/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBKey.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBKeyData.h"

namespace WebCore {

IDBKey::~IDBKey()
{
}

bool IDBKey::isValid() const
{
    if (m_type == KeyType::Invalid)
        return false;

    if (m_type == KeyType::Array) {
        for (auto& key : m_array) {
            if (!key->isValid())
                return false;
        }
    }

    return true;
}

int IDBKey::compare(const IDBKey& other) const
{
    if (m_type != other.m_type)
        return m_type > other.m_type ? -1 : 1;

    switch (m_type) {
    case KeyType::Array:
        for (size_t i = 0; i < m_array.size() && i < other.m_array.size(); ++i) {
            if (int result = m_array[i]->compare(*other.m_array[i]))
                return result;
        }
        if (m_array.size() < other.m_array.size())
            return -1;
        if (m_array.size() > other.m_array.size())
            return 1;
        return 0;
    case KeyType::String:
        return -codePointCompare(other.m_string, m_string);
    case KeyType::Date:
    case KeyType::Number:
        return (m_number < other.m_number) ? -1 : ((m_number > other. m_number) ? 1 : 0);
    case KeyType::Invalid:
    case KeyType::Min:
    case KeyType::Max:
        ASSERT_NOT_REACHED();
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

bool IDBKey::isLessThan(const IDBKey& other) const
{
    return compare(other) == -1;
}

bool IDBKey::isEqual(const IDBKey& other) const
{
    return !compare(other);
}

#ifndef NDEBUG
String IDBKey::loggingString() const
{
    return IDBKeyData(this).loggingString();
}
#endif

} // namespace WebCore

#endif
