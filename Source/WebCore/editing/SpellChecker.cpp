/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SpellChecker.h"

#include "Document.h"
#include "DocumentMarkerController.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLTextAreaElement.h"
#include "Page.h"
#include "PositionIterator.h"
#include "RenderObject.h"
#include "Settings.h"
#include "TextCheckerClient.h"
#include "TextCheckingHelper.h"
#include "htmlediting.h"

namespace WebCore {

SpellCheckRequest::SpellCheckRequest(PassRefPtr<Range> checkingRange, PassRefPtr<Range> paragraphRange, const String& text, TextCheckingTypeMask mask, TextCheckingProcessType processType)
    : m_checkingRange(checkingRange)
    , m_paragraphRange(paragraphRange)
    , m_rootEditableElement(m_checkingRange->startContainer().rootEditableElement())
    , m_requestData(unrequestedTextCheckingSequence, text, mask, processType)
{
}

SpellCheckRequest::~SpellCheckRequest()
{
}

// static
RefPtr<SpellCheckRequest> SpellCheckRequest::create(TextCheckingTypeMask textCheckingOptions, TextCheckingProcessType processType, PassRefPtr<Range> checkingRange, PassRefPtr<Range> paragraphRange)
{
    ASSERT(checkingRange);
    ASSERT(paragraphRange);

    String text = checkingRange->text();
    if (!text.length())
        return nullptr;

    return adoptRef(*new SpellCheckRequest(checkingRange, paragraphRange, text, textCheckingOptions, processType));
}

const TextCheckingRequestData& SpellCheckRequest::data() const
{
    return m_requestData;
}

void SpellCheckRequest::didSucceed(const Vector<TextCheckingResult>& results)
{
    if (!m_checker)
        return;
    m_checker->didCheckSucceed(m_requestData.sequence(), results);
    m_checker = nullptr;
}

void SpellCheckRequest::didCancel()
{
    if (!m_checker)
        return;
    m_checker->didCheckCancel(m_requestData.sequence());
    m_checker = nullptr;
}

void SpellCheckRequest::setCheckerAndSequence(SpellChecker* requester, int sequence)
{
    ASSERT(!m_checker);
    ASSERT(m_requestData.sequence() == unrequestedTextCheckingSequence);
    m_checker = requester;
    m_requestData.m_sequence = sequence;
}

void SpellCheckRequest::requesterDestroyed()
{
    m_checker = nullptr;
}

SpellChecker::SpellChecker(Frame& frame)
    : m_frame(frame)
    , m_lastRequestSequence(0)
    , m_lastProcessedSequence(0)
    , m_timerToProcessQueuedRequest(*this, &SpellChecker::timerFiredToProcessQueuedRequest)
{
}

SpellChecker::~SpellChecker()
{
    if (m_processingRequest)
        m_processingRequest->requesterDestroyed();
    for (auto& queue : m_requestQueue)
        queue->requesterDestroyed();
}

TextCheckerClient* SpellChecker::client() const
{
    Page* page = m_frame.page();
    if (!page)
        return nullptr;
    return page->editorClient().textChecker();
}

void SpellChecker::timerFiredToProcessQueuedRequest()
{
    ASSERT(!m_requestQueue.isEmpty());
    if (m_requestQueue.isEmpty())
        return;

    invokeRequest(m_requestQueue.takeFirst());
}

bool SpellChecker::isAsynchronousEnabled() const
{
    return m_frame.settings().asynchronousSpellCheckingEnabled();
}

bool SpellChecker::canCheckAsynchronously(Range* range) const
{
    return client() && isCheckable(range) && isAsynchronousEnabled();
}

bool SpellChecker::isCheckable(Range* range) const
{
    if (!range || !range->firstNode() || !range->firstNode()->renderer())
        return false;
    const Node& node = range->startContainer();
    if (is<Element>(node) && !downcast<Element>(node).isSpellCheckingEnabled())
        return false;
    return true;
}

void SpellChecker::requestCheckingFor(PassRefPtr<SpellCheckRequest> request)
{
    if (!request || !canCheckAsynchronously(request->paragraphRange().get()))
        return;

    ASSERT(request->data().sequence() == unrequestedTextCheckingSequence);
    int sequence = ++m_lastRequestSequence;
    if (sequence == unrequestedTextCheckingSequence)
        sequence = ++m_lastRequestSequence;

    request->setCheckerAndSequence(this, sequence);

    if (m_timerToProcessQueuedRequest.isActive() || m_processingRequest) {
        enqueueRequest(request);
        return;
    }

    invokeRequest(request);
}

void SpellChecker::invokeRequest(PassRefPtr<SpellCheckRequest> request)
{
    ASSERT(!m_processingRequest);
    if (!client())
        return;
    m_processingRequest = request;
    client()->requestCheckingOfString(m_processingRequest, m_frame.selection().selection());
}

void SpellChecker::enqueueRequest(PassRefPtr<SpellCheckRequest> request)
{
    ASSERT(request);

    for (auto& queue : m_requestQueue) {
        if (request->rootEditableElement() != queue->rootEditableElement())
            continue;

        queue = request;
        return;
    }

    m_requestQueue.append(request);
}

void SpellChecker::didCheck(int sequence, const Vector<TextCheckingResult>& results)
{
    ASSERT(m_processingRequest);
    ASSERT(m_processingRequest->data().sequence() == sequence);
    if (m_processingRequest->data().sequence() != sequence) {
        m_requestQueue.clear();
        return;
    }

    m_frame.editor().markAndReplaceFor(m_processingRequest, results);

    if (m_lastProcessedSequence < sequence)
        m_lastProcessedSequence = sequence;

    m_processingRequest = nullptr;
    if (!m_requestQueue.isEmpty())
        m_timerToProcessQueuedRequest.startOneShot(0);
}

void SpellChecker::didCheckSucceed(int sequence, const Vector<TextCheckingResult>& results)
{
    TextCheckingRequestData requestData = m_processingRequest->data();
    if (requestData.sequence() == sequence) {
        unsigned markers = 0;
        if (requestData.mask() & TextCheckingTypeSpelling)
            markers |= DocumentMarker::Spelling;
        if (requestData.mask() & TextCheckingTypeGrammar)
            markers |= DocumentMarker::Grammar;
        if (markers)
            m_frame.document()->markers().removeMarkers(m_processingRequest->checkingRange().get(), markers);
    }
    didCheck(sequence, results);
}

void SpellChecker::didCheckCancel(int sequence)
{
    didCheck(sequence, Vector<TextCheckingResult>());
}

} // namespace WebCore
