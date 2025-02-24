/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBServer.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBRequestData.h"
#include "IDBResultData.h"
#include "Logging.h"
#include "MemoryIDBBackingStore.h"
#include "SQLiteFileSystem.h"
#include "SQLiteIDBBackingStore.h"
#include "SecurityOrigin.h"
#include <wtf/CrossThreadCopier.h>
#include <wtf/Locker.h>
#include <wtf/MainThread.h>

namespace WebCore {
namespace IDBServer {

Ref<IDBServer> IDBServer::create(IDBBackingStoreTemporaryFileHandler& fileHandler)
{
    return adoptRef(*new IDBServer(fileHandler));
}

Ref<IDBServer> IDBServer::create(const String& databaseDirectoryPath, IDBBackingStoreTemporaryFileHandler& fileHandler)
{
    return adoptRef(*new IDBServer(databaseDirectoryPath, fileHandler));
}

IDBServer::IDBServer(IDBBackingStoreTemporaryFileHandler& fileHandler)
    : m_backingStoreTemporaryFileHandler(fileHandler)
{
    Locker<Lock> locker(m_databaseThreadCreationLock);
    m_threadID = createThread(IDBServer::databaseThreadEntry, this, "IndexedDatabase Server");
}

IDBServer::IDBServer(const String& databaseDirectoryPath, IDBBackingStoreTemporaryFileHandler& fileHandler)
    : m_databaseDirectoryPath(databaseDirectoryPath)
    , m_backingStoreTemporaryFileHandler(fileHandler)
{
    LOG(IndexedDB, "IDBServer created at path %s", databaseDirectoryPath.utf8().data());

    Locker<Lock> locker(m_databaseThreadCreationLock);
    m_threadID = createThread(IDBServer::databaseThreadEntry, this, "IndexedDatabase Server");
}

void IDBServer::registerConnection(IDBConnectionToClient& connection)
{
    ASSERT(!m_connectionMap.contains(connection.identifier()));
    m_connectionMap.set(connection.identifier(), &connection);
}

void IDBServer::unregisterConnection(IDBConnectionToClient& connection)
{
    ASSERT(m_connectionMap.contains(connection.identifier()));
    ASSERT(m_connectionMap.get(connection.identifier()) == &connection);

    connection.connectionToClientClosed();

    m_connectionMap.remove(connection.identifier());
}

void IDBServer::registerTransaction(UniqueIDBDatabaseTransaction& transaction)
{
    ASSERT(!m_transactions.contains(transaction.info().identifier()));
    m_transactions.set(transaction.info().identifier(), &transaction);
}

void IDBServer::unregisterTransaction(UniqueIDBDatabaseTransaction& transaction)
{
    ASSERT(m_transactions.contains(transaction.info().identifier()));
    ASSERT(m_transactions.get(transaction.info().identifier()) == &transaction);

    m_transactions.remove(transaction.info().identifier());
}

void IDBServer::registerDatabaseConnection(UniqueIDBDatabaseConnection& connection)
{
    ASSERT(!m_databaseConnections.contains(connection.identifier()));
    m_databaseConnections.set(connection.identifier(), &connection);
}

void IDBServer::unregisterDatabaseConnection(UniqueIDBDatabaseConnection& connection)
{
    ASSERT(m_databaseConnections.contains(connection.identifier()));
    m_databaseConnections.remove(connection.identifier());
}

UniqueIDBDatabase& IDBServer::getOrCreateUniqueIDBDatabase(const IDBDatabaseIdentifier& identifier)
{
    ASSERT(isMainThread());

    auto uniqueIDBDatabase = m_uniqueIDBDatabaseMap.add(identifier, nullptr);
    if (uniqueIDBDatabase.isNewEntry)
        uniqueIDBDatabase.iterator->value = UniqueIDBDatabase::create(*this, identifier);

    return *uniqueIDBDatabase.iterator->value;
}

std::unique_ptr<IDBBackingStore> IDBServer::createBackingStore(const IDBDatabaseIdentifier& identifier)
{
    ASSERT(!isMainThread());

    if (m_databaseDirectoryPath.isEmpty())
        return MemoryIDBBackingStore::create(identifier);

    return std::make_unique<SQLiteIDBBackingStore>(identifier, m_databaseDirectoryPath, m_backingStoreTemporaryFileHandler);
}

void IDBServer::openDatabase(const IDBRequestData& requestData)
{
    LOG(IndexedDB, "IDBServer::openDatabase");

    auto& uniqueIDBDatabase = getOrCreateUniqueIDBDatabase(requestData.databaseIdentifier());

    auto connection = m_connectionMap.get(requestData.requestIdentifier().connectionIdentifier());
    if (!connection) {
        // If the connection back to the client is gone, there's no way to open the database as
        // well as no way to message back failure.
        return;
    }

    uniqueIDBDatabase.openDatabaseConnection(*connection, requestData);
}

void IDBServer::deleteDatabase(const IDBRequestData& requestData)
{
    LOG(IndexedDB, "IDBServer::deleteDatabase - %s", requestData.databaseIdentifier().debugString().utf8().data());
    ASSERT(isMainThread());

    auto connection = m_connectionMap.get(requestData.requestIdentifier().connectionIdentifier());
    if (!connection) {
        // If the connection back to the client is gone, there's no way to delete the database as
        // well as no way to message back failure.
        return;
    }

    auto* database = m_uniqueIDBDatabaseMap.get(requestData.databaseIdentifier());
    if (!database)
        database = &getOrCreateUniqueIDBDatabase(requestData.databaseIdentifier());

    database->handleDelete(*connection, requestData);
}

void IDBServer::closeUniqueIDBDatabase(UniqueIDBDatabase& database)
{
    LOG(IndexedDB, "IDBServer::closeUniqueIDBDatabase");
    ASSERT(isMainThread());

    m_uniqueIDBDatabaseMap.remove(database.identifier());
}

void IDBServer::abortTransaction(const IDBResourceIdentifier& transactionIdentifier)
{
    LOG(IndexedDB, "IDBServer::abortTransaction");

    auto transaction = m_transactions.get(transactionIdentifier);
    if (!transaction) {
        // If there is no transaction there is nothing to abort.
        // We also have no access to a connection over which to message failure-to-abort.
        return;
    }

    transaction->abort();
}

void IDBServer::createObjectStore(const IDBRequestData& requestData, const IDBObjectStoreInfo& info)
{
    LOG(IndexedDB, "IDBServer::createObjectStore");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    ASSERT(transaction->isVersionChange());
    transaction->createObjectStore(requestData, info);
}

void IDBServer::deleteObjectStore(const IDBRequestData& requestData, const String& objectStoreName)
{
    LOG(IndexedDB, "IDBServer::deleteObjectStore");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    ASSERT(transaction->isVersionChange());
    transaction->deleteObjectStore(requestData, objectStoreName);
}

void IDBServer::clearObjectStore(const IDBRequestData& requestData, uint64_t objectStoreIdentifier)
{
    LOG(IndexedDB, "IDBServer::clearObjectStore");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->clearObjectStore(requestData, objectStoreIdentifier);
}

void IDBServer::createIndex(const IDBRequestData& requestData, const IDBIndexInfo& info)
{
    LOG(IndexedDB, "IDBServer::createIndex");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    ASSERT(transaction->isVersionChange());
    transaction->createIndex(requestData, info);
}

void IDBServer::deleteIndex(const IDBRequestData& requestData, uint64_t objectStoreIdentifier, const String& indexName)
{
    LOG(IndexedDB, "IDBServer::deleteIndex");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    ASSERT(transaction->isVersionChange());
    transaction->deleteIndex(requestData, objectStoreIdentifier, indexName);
}

void IDBServer::putOrAdd(const IDBRequestData& requestData, const IDBKeyData& keyData, const IDBValue& value, IndexedDB::ObjectStoreOverwriteMode overwriteMode)
{
    LOG(IndexedDB, "IDBServer::putOrAdd");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->putOrAdd(requestData, keyData, value, overwriteMode);
}

void IDBServer::getRecord(const IDBRequestData& requestData, const IDBKeyRangeData& keyRangeData)
{
    LOG(IndexedDB, "IDBServer::getRecord");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->getRecord(requestData, keyRangeData);
}

void IDBServer::getCount(const IDBRequestData& requestData, const IDBKeyRangeData& keyRangeData)
{
    LOG(IndexedDB, "IDBServer::getCount");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->getCount(requestData, keyRangeData);
}

void IDBServer::deleteRecord(const IDBRequestData& requestData, const IDBKeyRangeData& keyRangeData)
{
    LOG(IndexedDB, "IDBServer::deleteRecord");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->deleteRecord(requestData, keyRangeData);
}

void IDBServer::openCursor(const IDBRequestData& requestData, const IDBCursorInfo& info)
{
    LOG(IndexedDB, "IDBServer::openCursor");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->openCursor(requestData, info);
}

void IDBServer::iterateCursor(const IDBRequestData& requestData, const IDBKeyData& key, unsigned long count)
{
    LOG(IndexedDB, "IDBServer::iterateCursor");

    auto transaction = m_transactions.get(requestData.transactionIdentifier());
    if (!transaction)
        return;

    transaction->iterateCursor(requestData, key, count);
}

void IDBServer::establishTransaction(uint64_t databaseConnectionIdentifier, const IDBTransactionInfo& info)
{
    LOG(IndexedDB, "IDBServer::establishTransaction");

    auto databaseConnection = m_databaseConnections.get(databaseConnectionIdentifier);
    if (!databaseConnection)
        return;

    databaseConnection->establishTransaction(info);
}

void IDBServer::commitTransaction(const IDBResourceIdentifier& transactionIdentifier)
{
    LOG(IndexedDB, "IDBServer::commitTransaction");

    auto transaction = m_transactions.get(transactionIdentifier);
    if (!transaction) {
        // If there is no transaction there is nothing to commit.
        // We also have no access to a connection over which to message failure-to-commit.
        return;
    }

    transaction->commit();
}

void IDBServer::didFinishHandlingVersionChangeTransaction(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& transactionIdentifier)
{
    LOG(IndexedDB, "IDBServer::didFinishHandlingVersionChangeTransaction - %s", transactionIdentifier.loggingString().utf8().data());

    auto* connection = m_databaseConnections.get(databaseConnectionIdentifier);
    if (!connection)
        return;

    connection->didFinishHandlingVersionChange(transactionIdentifier);
}

void IDBServer::databaseConnectionClosed(uint64_t databaseConnectionIdentifier)
{
    LOG(IndexedDB, "IDBServer::databaseConnectionClosed - %" PRIu64, databaseConnectionIdentifier);

    auto databaseConnection = m_databaseConnections.get(databaseConnectionIdentifier);
    if (!databaseConnection)
        return;

    databaseConnection->connectionClosedFromClient();
}

void IDBServer::abortOpenAndUpgradeNeeded(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& transactionIdentifier)
{
    LOG(IndexedDB, "IDBServer::abortOpenAndUpgradeNeeded");

    auto transaction = m_transactions.get(transactionIdentifier);
    if (transaction)
        transaction->abortWithoutCallback();

    auto databaseConnection = m_databaseConnections.get(databaseConnectionIdentifier);
    if (!databaseConnection)
        return;

    databaseConnection->connectionClosedFromClient();
}

void IDBServer::didFireVersionChangeEvent(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& requestIdentifier)
{
    LOG(IndexedDB, "IDBServer::didFireVersionChangeEvent");

    if (auto databaseConnection = m_databaseConnections.get(databaseConnectionIdentifier))
        databaseConnection->didFireVersionChangeEvent(requestIdentifier);
}

void IDBServer::openDBRequestCancelled(const IDBRequestData& requestData)
{
    LOG(IndexedDB, "IDBServer::openDBRequestCancelled");
    ASSERT(isMainThread());

    auto* uniqueIDBDatabase = m_uniqueIDBDatabaseMap.get(requestData.databaseIdentifier());
    if (!uniqueIDBDatabase)
        return;

    uniqueIDBDatabase->openDBRequestCancelled(requestData.requestIdentifier());
}

void IDBServer::confirmDidCloseFromServer(uint64_t databaseConnectionIdentifier)
{
    LOG(IndexedDB, "IDBServer::confirmDidCloseFromServer");

    if (auto databaseConnection = m_databaseConnections.get(databaseConnectionIdentifier))
        databaseConnection->confirmDidCloseFromServer();
}

void IDBServer::getAllDatabaseNames(uint64_t serverConnectionIdentifier, const SecurityOriginData& mainFrameOrigin, const SecurityOriginData& openingOrigin, uint64_t callbackID)
{
    postDatabaseTask(createCrossThreadTask(*this, &IDBServer::performGetAllDatabaseNames, serverConnectionIdentifier, mainFrameOrigin, openingOrigin, callbackID));
}

void IDBServer::performGetAllDatabaseNames(uint64_t serverConnectionIdentifier, const SecurityOriginData& mainFrameOrigin, const SecurityOriginData& openingOrigin, uint64_t callbackID)
{
    String directory = IDBDatabaseIdentifier::databaseDirectoryRelativeToRoot(mainFrameOrigin, openingOrigin, m_databaseDirectoryPath);

    Vector<String> entries = listDirectory(directory, ASCIILiteral("*"));
    Vector<String> databases;
    databases.reserveInitialCapacity(entries.size());
    for (auto& entry : entries) {
        String encodedName = lastComponentOfPathIgnoringTrailingSlash(entry);
        databases.uncheckedAppend(SQLiteIDBBackingStore::databaseNameFromEncodedFilename(encodedName));
    }

    postDatabaseTaskReply(createCrossThreadTask(*this, &IDBServer::didGetAllDatabaseNames, serverConnectionIdentifier, callbackID, databases));
}

void IDBServer::didGetAllDatabaseNames(uint64_t serverConnectionIdentifier, uint64_t callbackID, const Vector<String>& databaseNames)
{
    auto connection = m_connectionMap.get(serverConnectionIdentifier);
    if (!connection)
        return;

    connection->didGetAllDatabaseNames(callbackID, databaseNames);
}

void IDBServer::postDatabaseTask(CrossThreadTask&& task)
{
    ASSERT(isMainThread());
    m_databaseQueue.append(WTFMove(task));
}

void IDBServer::postDatabaseTaskReply(CrossThreadTask&& task)
{
    ASSERT(!isMainThread());
    m_databaseReplyQueue.append(WTFMove(task));


    Locker<Lock> locker(m_mainThreadReplyLock);
    if (m_mainThreadReplyScheduled)
        return;

    m_mainThreadReplyScheduled = true;
    callOnMainThread([this] {
        handleTaskRepliesOnMainThread();
    });
}

void IDBServer::databaseThreadEntry(void* threadData)
{
    ASSERT(threadData);
    IDBServer* server = reinterpret_cast<IDBServer*>(threadData);
    server->databaseRunLoop();
}

void IDBServer::databaseRunLoop()
{
    ASSERT(!isMainThread());
    {
        Locker<Lock> locker(m_databaseThreadCreationLock);
    }

    while (!m_databaseQueue.isKilled())
        m_databaseQueue.waitForMessage().performTask();
}

void IDBServer::handleTaskRepliesOnMainThread()
{
    {
        Locker<Lock> locker(m_mainThreadReplyLock);
        m_mainThreadReplyScheduled = false;
    }

    while (auto task = m_databaseReplyQueue.tryGetMessage())
        task->performTask();
}

static uint64_t generateDeleteCallbackID()
{
    ASSERT(isMainThread());
    static uint64_t currentID = 0;
    return ++currentID;
}

void IDBServer::closeAndDeleteDatabasesModifiedSince(std::chrono::system_clock::time_point modificationTime, std::function<void ()> completionHandler)
{
    uint64_t callbackID = generateDeleteCallbackID();
    auto addResult = m_deleteDatabaseCompletionHandlers.add(callbackID, WTFMove(completionHandler));
    ASSERT_UNUSED(addResult, addResult.isNewEntry);

    // If the modification time is in the future, don't both doing anything.
    if (modificationTime > std::chrono::system_clock::now()) {
        postDatabaseTaskReply(createCrossThreadTask(*this, &IDBServer::didPerformCloseAndDeleteDatabases, callbackID));
        return;
    }

    HashSet<UniqueIDBDatabase*> openDatabases;
    for (auto* connection : m_databaseConnections.values())
        openDatabases.add(&connection->database());

    for (auto* database : openDatabases)
        database->immediateCloseForUserDelete();

    postDatabaseTask(createCrossThreadTask(*this, &IDBServer::performCloseAndDeleteDatabasesModifiedSince, modificationTime, callbackID));
}

void IDBServer::closeAndDeleteDatabasesForOrigins(const Vector<SecurityOriginData>& origins, std::function<void ()> completionHandler)
{
    uint64_t callbackID = generateDeleteCallbackID();
    auto addResult = m_deleteDatabaseCompletionHandlers.add(callbackID, WTFMove(completionHandler));
    ASSERT_UNUSED(addResult, addResult.isNewEntry);

    HashSet<UniqueIDBDatabase*> openDatabases;
    for (auto* connection : m_databaseConnections.values()) {
        const auto& identifier = connection->database().identifier();
        for (auto& origin : origins) {
            if (identifier.isRelatedToOrigin(origin)) {
                openDatabases.add(&connection->database());
                break;
            }
        }
    }

    for (auto* database : openDatabases)
        database->immediateCloseForUserDelete();

    postDatabaseTask(createCrossThreadTask(*this, &IDBServer::performCloseAndDeleteDatabasesForOrigins, origins, callbackID));
}

static void removeAllDatabasesForOriginPath(const String& originPath, std::chrono::system_clock::time_point modifiedSince)
{
    Vector<String> databasePaths = listDirectory(originPath, "*");

    for (auto& databasePath : databasePaths) {
        String databaseFile = pathByAppendingComponent(databasePath, "IndexedDB.sqlite3");

        if (!fileExists(databaseFile))
            continue;

        if (modifiedSince > std::chrono::system_clock::time_point::min()) {
            time_t modificationTime;
            if (!getFileModificationTime(databaseFile, modificationTime))
                continue;

            if (std::chrono::system_clock::from_time_t(modificationTime) < modifiedSince)
                continue;
        }

        SQLiteFileSystem::deleteDatabaseFile(databaseFile);
        deleteEmptyDirectory(databasePath);
    }

    deleteEmptyDirectory(originPath);
}

void IDBServer::performCloseAndDeleteDatabasesModifiedSince(std::chrono::system_clock::time_point modifiedSince, uint64_t callbackID)
{
    if (!m_databaseDirectoryPath.isEmpty()) {
        Vector<String> originPaths = listDirectory(m_databaseDirectoryPath, "*");
        for (auto& originPath : originPaths)
            removeAllDatabasesForOriginPath(originPath, modifiedSince);
    }

    postDatabaseTaskReply(createCrossThreadTask(*this, &IDBServer::didPerformCloseAndDeleteDatabases, callbackID));
}

void IDBServer::performCloseAndDeleteDatabasesForOrigins(const Vector<SecurityOriginData>& origins, uint64_t callbackID)
{
    if (!m_databaseDirectoryPath.isEmpty()) {
        for (const auto& origin : origins) {
            String originPath = pathByAppendingComponent(m_databaseDirectoryPath, origin.securityOrigin()->databaseIdentifier());
            removeAllDatabasesForOriginPath(originPath, std::chrono::system_clock::time_point::min());
        }
    }

    postDatabaseTaskReply(createCrossThreadTask(*this, &IDBServer::didPerformCloseAndDeleteDatabases, callbackID));
}

void IDBServer::didPerformCloseAndDeleteDatabases(uint64_t callbackID)
{
    auto callback = m_deleteDatabaseCompletionHandlers.take(callbackID);
    ASSERT(callback);
    callback();
}

} // namespace IDBServer
} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
