/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentThreadableLoader.h"

#include "CachedRawResource.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedResourceRequestInitiators.h"
#include "ContentSecurityPolicy.h"
#include "CrossOriginAccessControl.h"
#include "CrossOriginPreflightChecker.h"
#include "CrossOriginPreflightResultCache.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "ProgressTracker.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "RuntimeEnabledFeatures.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SubresourceLoader.h"
#include "ThreadableLoaderClient.h"
#include <wtf/Assertions.h>
#include <wtf/Ref.h>

namespace WebCore {

void DocumentThreadableLoader::loadResourceSynchronously(Document& document, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options, std::unique_ptr<ContentSecurityPolicy>&& contentSecurityPolicy)
{
    // The loader will be deleted as soon as this function exits.
    RefPtr<DocumentThreadableLoader> loader = adoptRef(new DocumentThreadableLoader(document, client, LoadSynchronously, request, options, WTFMove(contentSecurityPolicy)));
    ASSERT(loader->hasOneRef());
}

void DocumentThreadableLoader::loadResourceSynchronously(Document& document, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options)
{
    loadResourceSynchronously(document, request, client, options, nullptr);
}

RefPtr<DocumentThreadableLoader> DocumentThreadableLoader::create(Document& document, ThreadableLoaderClient& client, const ResourceRequest& request, const ThreadableLoaderOptions& options, std::unique_ptr<ContentSecurityPolicy>&& contentSecurityPolicy)
{
    RefPtr<DocumentThreadableLoader> loader = adoptRef(new DocumentThreadableLoader(document, client, LoadAsynchronously, request, options, WTFMove(contentSecurityPolicy)));
    if (!loader->isLoading())
        loader = nullptr;
    return loader;
}

RefPtr<DocumentThreadableLoader> DocumentThreadableLoader::create(Document& document, ThreadableLoaderClient& client, const ResourceRequest& request, const ThreadableLoaderOptions& options)
{
    return create(document, client, request, options, nullptr);
}

DocumentThreadableLoader::DocumentThreadableLoader(Document& document, ThreadableLoaderClient& client, BlockingBehavior blockingBehavior, const ResourceRequest& request, const ThreadableLoaderOptions& options, std::unique_ptr<ContentSecurityPolicy>&& contentSecurityPolicy)
    : m_client(&client)
    , m_document(document)
    , m_options(options)
    , m_sameOriginRequest(securityOrigin()->canRequest(request.url()))
    , m_simpleRequest(true)
    , m_async(blockingBehavior == LoadAsynchronously)
    , m_contentSecurityPolicy(WTFMove(contentSecurityPolicy))
{
    // Setting an outgoing referer is only supported in the async code path.
    ASSERT(m_async || request.httpReferrer().isEmpty());

    ASSERT_WITH_SECURITY_IMPLICATION(isAllowedByContentSecurityPolicy(request.url()));

    if (m_sameOriginRequest || m_options.crossOriginRequestPolicy == AllowCrossOriginRequests) {
        loadRequest(request, DoSecurityCheck);
        return;
    }

    if (m_options.crossOriginRequestPolicy == DenyCrossOriginRequests) {
        m_client->didFail(ResourceError(errorDomainWebKitInternal, 0, request.url(), "Cross origin requests are not supported."));
        return;
    }

    makeCrossOriginAccessRequest(request);
}

void DocumentThreadableLoader::makeCrossOriginAccessRequest(const ResourceRequest& request)
{
    ASSERT(m_options.crossOriginRequestPolicy == UseAccessControl);

    auto crossOriginRequest = request;
    updateRequestForAccessControl(crossOriginRequest, securityOrigin(), m_options.allowCredentials());

    if ((m_options.preflightPolicy == ConsiderPreflight && isSimpleCrossOriginAccessRequest(crossOriginRequest.httpMethod(), crossOriginRequest.httpHeaderFields())) || m_options.preflightPolicy == PreventPreflight)
        makeSimpleCrossOriginAccessRequest(crossOriginRequest);
    else {
        m_simpleRequest = false;
        if (CrossOriginPreflightResultCache::singleton().canSkipPreflight(securityOrigin()->toString(), crossOriginRequest.url(), m_options.allowCredentials(), crossOriginRequest.httpMethod(), crossOriginRequest.httpHeaderFields()))
            preflightSuccess(WTFMove(crossOriginRequest));
        else
            makeCrossOriginAccessRequestWithPreflight(WTFMove(crossOriginRequest));
    }
}

void DocumentThreadableLoader::makeSimpleCrossOriginAccessRequest(const ResourceRequest& request)
{
    ASSERT(m_options.preflightPolicy != ForcePreflight);
    ASSERT(m_options.preflightPolicy == PreventPreflight || isSimpleCrossOriginAccessRequest(request.httpMethod(), request.httpHeaderFields()));

    // Cross-origin requests are only allowed for HTTP and registered schemes. We would catch this when checking response headers later, but there is no reason to send a request that's guaranteed to be denied.
    if (!SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(request.url().protocol())) {
        m_client->didFail(ResourceError(errorDomainWebKitInternal, 0, request.url(), "Cross origin requests are only supported for HTTP.", ResourceError::Type::AccessControl));
        return;
    }

    loadRequest(request, DoSecurityCheck);
}

void DocumentThreadableLoader::makeCrossOriginAccessRequestWithPreflight(ResourceRequest&& request)
{
    if (m_async) {
        m_preflightChecker = CrossOriginPreflightChecker(*this, WTFMove(request));
        m_preflightChecker->startPreflight();
        return;
    }
    CrossOriginPreflightChecker::doPreflight(*this, WTFMove(request));
}

DocumentThreadableLoader::~DocumentThreadableLoader()
{
    if (m_resource)
        m_resource->removeClient(this);
}

void DocumentThreadableLoader::cancel()
{
    Ref<DocumentThreadableLoader> protectedThis(*this);

    // Cancel can re-enter and m_resource might be null here as a result.
    if (m_client && m_resource) {
        // FIXME: This error is sent to the client in didFail(), so it should not be an internal one. Use FrameLoaderClient::cancelledError() instead.
        ResourceError error(errorDomainWebKitInternal, 0, m_resource->url(), "Load cancelled", ResourceError::Type::Cancellation);
        didFail(m_resource->identifier(), error);
    }
    clearResource();
    m_client = nullptr;
}

void DocumentThreadableLoader::setDefersLoading(bool value)
{
    if (m_resource)
        m_resource->setDefersLoading(value);
    if (m_preflightChecker)
        m_preflightChecker->setDefersLoading(value);
}

void DocumentThreadableLoader::clearResource()
{
    // Script can cancel and restart a request reentrantly within removeClient(),
    // which could lead to calling CachedResource::removeClient() multiple times for
    // this DocumentThreadableLoader. Save off a copy of m_resource and clear it to
    // prevent the reentrancy.
    if (CachedResourceHandle<CachedRawResource> resource = m_resource) {
        m_resource = nullptr;
        resource->removeClient(this);
    }
    if (m_preflightChecker)
        m_preflightChecker = Nullopt;
}

static inline void reportContentSecurityPolicyError(ThreadableLoaderClient& client, const URL& url)
{
    client.didFail(ResourceError(errorDomainWebKitInternal, 0, url, "Cross-origin redirection denied by Content Security Policy.", ResourceError::Type::AccessControl));
}

static inline void reportCrossOriginResourceSharingError(ThreadableLoaderClient& client, const URL& url)
{
    client.didFail(ResourceError(errorDomainWebKitInternal, 0, url, "Cross-origin redirection denied by Cross-Origin Resource Sharing policy.", ResourceError::Type::AccessControl));
}

void DocumentThreadableLoader::redirectReceived(CachedResource* resource, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);

    Ref<DocumentThreadableLoader> protectedThis(*this);
    if (!isAllowedByContentSecurityPolicy(request.url(), !redirectResponse.isNull())) {
        reportContentSecurityPolicyError(*m_client, redirectResponse.url());
        request = ResourceRequest();
        return;
    }

    // Allow same origin requests to continue after allowing clients to audit the redirect.
    if (isAllowedRedirect(request.url()))
        return;

    // When using access control, only simple cross origin requests are allowed to redirect. The new request URL must have a supported
    // scheme and not contain the userinfo production. In addition, the redirect response must pass the access control check if the
    // original request was not same-origin.
    if (m_options.crossOriginRequestPolicy == UseAccessControl) {
        bool allowRedirect = false;
        if (m_simpleRequest) {
            String accessControlErrorDescription;
            allowRedirect = isValidCrossOriginRedirectionURL(request.url())
                            && (m_sameOriginRequest || passesAccessControlCheck(redirectResponse, m_options.allowCredentials(), securityOrigin(), accessControlErrorDescription));
        }

        if (allowRedirect) {
            if (m_resource)
                clearResource();

            RefPtr<SecurityOrigin> originalOrigin = SecurityOrigin::createFromString(redirectResponse.url());
            RefPtr<SecurityOrigin> requestOrigin = SecurityOrigin::createFromString(request.url());
            // If the original request wasn't same-origin, then if the request URL origin is not same origin with the original URL origin,
            // set the source origin to a globally unique identifier. (If the original request was same-origin, the origin of the new request
            // should be the original URL origin.)
            if (!m_sameOriginRequest && !originalOrigin->isSameSchemeHostPort(requestOrigin.get()))
                m_options.securityOrigin = SecurityOrigin::createUnique();
            // Force any subsequent request to use these checks.
            m_sameOriginRequest = false;

            // Since the request is no longer same-origin, if the user didn't request credentials in
            // the first place, update our state so we neither request them nor expect they must be allowed.
            if (m_options.credentialRequest() == ClientDidNotRequestCredentials)
                m_options.setAllowCredentials(DoNotAllowStoredCredentials);

            cleanRedirectedRequestForAccessControl(request);

            makeCrossOriginAccessRequest(request);
            return;
        }
    }

    reportCrossOriginResourceSharingError(*m_client, redirectResponse.url());
    request = ResourceRequest();
}

void DocumentThreadableLoader::dataSent(CachedResource* resource, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);
    m_client->didSendData(bytesSent, totalBytesToBeSent);
}

void DocumentThreadableLoader::responseReceived(CachedResource* resource, const ResourceResponse& response)
{
    ASSERT_UNUSED(resource, resource == m_resource);
    didReceiveResponse(m_resource->identifier(), response);
}

void DocumentThreadableLoader::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    ASSERT(m_client);

    String accessControlErrorDescription;
    if (!m_sameOriginRequest && m_options.crossOriginRequestPolicy == UseAccessControl) {
        if (!passesAccessControlCheck(response, m_options.allowCredentials(), securityOrigin(), accessControlErrorDescription)) {
            m_client->didFail(ResourceError(errorDomainWebKitInternal, 0, response.url(), accessControlErrorDescription, ResourceError::Type::AccessControl));
            return;
        }
    }

    m_client->didReceiveResponse(identifier, response);
}

void DocumentThreadableLoader::dataReceived(CachedResource* resource, const char* data, int dataLength)
{
    ASSERT_UNUSED(resource, resource == m_resource);
    didReceiveData(m_resource->identifier(), data, dataLength);
}

void DocumentThreadableLoader::didReceiveData(unsigned long, const char* data, int dataLength)
{
    ASSERT(m_client);

    m_client->didReceiveData(data, dataLength);
}

void DocumentThreadableLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);

    if (m_resource->errorOccurred())
        didFail(m_resource->identifier(), m_resource->resourceError());
    else
        didFinishLoading(m_resource->identifier(), m_resource->loadFinishTime());
}

void DocumentThreadableLoader::didFinishLoading(unsigned long identifier, double finishTime)
{
    ASSERT(m_client);
    m_client->didFinishLoading(identifier, finishTime);
}

void DocumentThreadableLoader::didFail(unsigned long, const ResourceError& error)
{
    ASSERT(m_client);
    m_client->didFail(error);
}

void DocumentThreadableLoader::preflightSuccess(ResourceRequest&& request)
{
    ResourceRequest actualRequest(WTFMove(request));
    actualRequest.setHTTPOrigin(securityOrigin()->toString());

    m_preflightChecker = Nullopt;

    // It should be ok to skip the security check since we already asked about the preflight request.
    loadRequest(actualRequest, SkipSecurityCheck);
}

void DocumentThreadableLoader::preflightFailure(unsigned long identifier, const ResourceError& error)
{
    ASSERT(error.isAccessControl());
    m_preflightChecker = Nullopt;

    InspectorInstrumentation::didFailLoading(m_document.frame(), m_document.frame()->loader().documentLoader(), identifier, error);

    ASSERT(m_client);
    m_client->didFail(error);
}

void DocumentThreadableLoader::loadRequest(const ResourceRequest& request, SecurityCheckPolicy securityCheck)
{
    // Any credential should have been removed from the cross-site requests.
    const URL& requestURL = request.url();
    m_options.setSecurityCheck(securityCheck);
    ASSERT(m_sameOriginRequest || requestURL.user().isEmpty());
    ASSERT(m_sameOriginRequest || requestURL.pass().isEmpty());

    if (m_async) {
        ThreadableLoaderOptions options = m_options;
        options.setClientCredentialPolicy(DoNotAskClientForCrossOriginCredentials);

        CachedResourceRequest newRequest(request, options);
        if (RuntimeEnabledFeatures::sharedFeatures().resourceTimingEnabled())
            newRequest.setInitiator(m_options.initiator);
        ASSERT(!m_resource);
        m_resource = m_document.cachedResourceLoader().requestRawResource(newRequest);
        if (m_resource)
            m_resource->addClient(this);

        return;
    }

    // FIXME: ThreadableLoaderOptions.sniffContent is not supported for synchronous requests.
    RefPtr<SharedBuffer> data;
    ResourceError error;
    ResourceResponse response;
    unsigned long identifier = std::numeric_limits<unsigned long>::max();
    if (m_document.frame())
        identifier = m_document.frame()->loader().loadResourceSynchronously(request, m_options.allowCredentials(), m_options.clientCredentialPolicy(), error, response, data);

    if (!error.isNull() && response.httpStatusCode() <= 0) {
        if (requestURL.isLocalFile()) {
            // We don't want XMLHttpRequest to raise an exception for file:// resources, see <rdar://problem/4962298>.
            // FIXME: XMLHttpRequest quirks should be in XMLHttpRequest code, not in DocumentThreadableLoader.cpp.
            didReceiveResponse(identifier, response);
            didFinishLoading(identifier, 0.0);
            return;
        }
        m_client->didFail(error);
        return;
    }

    // FIXME: FrameLoader::loadSynchronously() does not tell us whether a redirect happened or not, so we guess by comparing the
    // request and response URLs. This isn't a perfect test though, since a server can serve a redirect to the same URL that was
    // requested. Also comparing the request and response URLs as strings will fail if the requestURL still has its credentials.
    bool didRedirect = requestURL != response.url();
    if (didRedirect) {
        if (!isAllowedByContentSecurityPolicy(response.url(), didRedirect)) {
            reportContentSecurityPolicyError(*m_client, requestURL);
            return;
        }
        if (!isAllowedRedirect(response.url())) {
            reportCrossOriginResourceSharingError(*m_client, requestURL);
            return;
        }
    }

    didReceiveResponse(identifier, response);

    if (data)
        didReceiveData(identifier, data->data(), data->size());
    didFinishLoading(identifier, 0.0);
}

bool DocumentThreadableLoader::isAllowedByContentSecurityPolicy(const URL& url, bool didRedirect)
{
    bool overrideContentSecurityPolicy = false;
    ContentSecurityPolicy::RedirectResponseReceived redirectResponseReceived = didRedirect ? ContentSecurityPolicy::RedirectResponseReceived::Yes : ContentSecurityPolicy::RedirectResponseReceived::No;

    switch (m_options.contentSecurityPolicyEnforcement) {
    case ContentSecurityPolicyEnforcement::DoNotEnforce:
        return true;
    case ContentSecurityPolicyEnforcement::EnforceChildSrcDirective:
        return contentSecurityPolicy().allowChildContextFromSource(url, overrideContentSecurityPolicy, redirectResponseReceived);
    case ContentSecurityPolicyEnforcement::EnforceConnectSrcDirective:
        return contentSecurityPolicy().allowConnectToSource(url, overrideContentSecurityPolicy, redirectResponseReceived);
    case ContentSecurityPolicyEnforcement::EnforceScriptSrcDirective:
        return contentSecurityPolicy().allowScriptFromSource(url, overrideContentSecurityPolicy, redirectResponseReceived);
    }
    ASSERT_NOT_REACHED();
    return false;
}

bool DocumentThreadableLoader::isAllowedRedirect(const URL& url)
{
    if (m_options.crossOriginRequestPolicy == AllowCrossOriginRequests)
        return true;

    return m_sameOriginRequest && securityOrigin()->canRequest(url);
}

bool DocumentThreadableLoader::isXMLHttpRequest() const
{
    return m_options.initiator == cachedResourceRequestInitiators().xmlhttprequest;
}

SecurityOrigin* DocumentThreadableLoader::securityOrigin() const
{
    return m_options.securityOrigin ? m_options.securityOrigin.get() : m_document.securityOrigin();
}

const ContentSecurityPolicy& DocumentThreadableLoader::contentSecurityPolicy() const
{
    if (m_contentSecurityPolicy)
        return *m_contentSecurityPolicy.get();
    ASSERT(m_document.contentSecurityPolicy());
    return *m_document.contentSecurityPolicy();
}

} // namespace WebCore
