/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef ResourceLoaderOptions_h
#define ResourceLoaderOptions_h

#include "FetchOptions.h"
#include "ResourceHandleTypes.h"

namespace WebCore {
    
enum SendCallbackPolicy {
    SendCallbacks,
    DoNotSendCallbacks
};

enum ContentSniffingPolicy {
    SniffContent,
    DoNotSniffContent
};

enum DataBufferingPolicy {
    BufferData,
    DoNotBufferData
};

enum SecurityCheckPolicy {
    SkipSecurityCheck,
    DoSecurityCheck
};

enum RequestOriginPolicy {
    UseDefaultOriginRestrictionsForType,
    RestrictToSameOrigin,
    PotentiallyCrossOriginEnabled // Indicates "potentially CORS-enabled fetch" in HTML standard.
};

enum CertificateInfoPolicy {
    IncludeCertificateInfo,
    DoNotIncludeCertificateInfo
};

enum class ContentSecurityPolicyImposition : uint8_t {
    SkipPolicyCheck,
    DoPolicyCheck
};

enum class DefersLoadingPolicy : uint8_t {
    AllowDefersLoading,
    DisallowDefersLoading
};

enum class CachingPolicy : uint8_t {
    AllowCaching,
    DisallowCaching
};

struct ResourceLoaderOptions {
    ResourceLoaderOptions()
        : m_sendLoadCallbacks(DoNotSendCallbacks)
        , m_sniffContent(DoNotSniffContent)
        , m_dataBufferingPolicy(BufferData)
        , m_allowCredentials(DoNotAllowStoredCredentials)
        , m_clientCredentialPolicy(DoNotAskClientForAnyCredentials)
        , m_credentialRequest(ClientDidNotRequestCredentials)
        , m_securityCheck(DoSecurityCheck)
        , m_requestOriginPolicy(UseDefaultOriginRestrictionsForType)
        , m_certificateInfoPolicy(DoNotIncludeCertificateInfo)
    {
    }

    ResourceLoaderOptions(SendCallbackPolicy sendLoadCallbacks, ContentSniffingPolicy sniffContent, DataBufferingPolicy dataBufferingPolicy, StoredCredentials allowCredentials, ClientCredentialPolicy credentialPolicy, CredentialRequest credentialRequest, SecurityCheckPolicy securityCheck, RequestOriginPolicy requestOriginPolicy, CertificateInfoPolicy certificateInfoPolicy, ContentSecurityPolicyImposition contentSecurityPolicyImposition, DefersLoadingPolicy defersLoadingPolicy, CachingPolicy cachingPolicy)
        : m_sendLoadCallbacks(sendLoadCallbacks)
        , m_sniffContent(sniffContent)
        , m_dataBufferingPolicy(dataBufferingPolicy)
        , m_allowCredentials(allowCredentials)
        , m_clientCredentialPolicy(credentialPolicy)
        , m_credentialRequest(credentialRequest)
        , m_securityCheck(securityCheck)
        , m_requestOriginPolicy(requestOriginPolicy)
        , m_certificateInfoPolicy(certificateInfoPolicy)
        , m_contentSecurityPolicyImposition(contentSecurityPolicyImposition)
        , m_defersLoadingPolicy(defersLoadingPolicy)
        , m_cachingPolicy(cachingPolicy)
    {
    }

    SendCallbackPolicy sendLoadCallbacks() const { return static_cast<SendCallbackPolicy>(m_sendLoadCallbacks); }
    void setSendLoadCallbacks(SendCallbackPolicy allow) { m_sendLoadCallbacks = allow; }
    ContentSniffingPolicy sniffContent() const { return static_cast<ContentSniffingPolicy>(m_sniffContent); }
    void setSniffContent(ContentSniffingPolicy policy) { m_sniffContent = policy; }
    DataBufferingPolicy dataBufferingPolicy() const { return static_cast<DataBufferingPolicy>(m_dataBufferingPolicy); }
    void setDataBufferingPolicy(DataBufferingPolicy policy) { m_dataBufferingPolicy = policy; }
    StoredCredentials allowCredentials() const { return static_cast<StoredCredentials>(m_allowCredentials); }
    void setAllowCredentials(StoredCredentials allow) { m_allowCredentials = allow; }
    ClientCredentialPolicy clientCredentialPolicy() const { return static_cast<ClientCredentialPolicy>(m_clientCredentialPolicy); }
    void setClientCredentialPolicy(ClientCredentialPolicy policy) { m_clientCredentialPolicy = policy; }
    CredentialRequest credentialRequest() { return static_cast<CredentialRequest>(m_credentialRequest); }
    void setCredentialRequest(CredentialRequest credentialRequest) { m_credentialRequest = credentialRequest; }
    SecurityCheckPolicy securityCheck() const { return static_cast<SecurityCheckPolicy>(m_securityCheck); }
    void setSecurityCheck(SecurityCheckPolicy check) { m_securityCheck = check; }
    RequestOriginPolicy requestOriginPolicy() const { return static_cast<RequestOriginPolicy>(m_requestOriginPolicy); }
    void setRequestOriginPolicy(RequestOriginPolicy policy) { m_requestOriginPolicy = policy; }
    CertificateInfoPolicy certificateInfoPolicy() const { return static_cast<CertificateInfoPolicy>(m_certificateInfoPolicy); }
    void setCertificateInfoPolicy(CertificateInfoPolicy policy) { m_certificateInfoPolicy = policy; }
    ContentSecurityPolicyImposition contentSecurityPolicyImposition() const { return m_contentSecurityPolicyImposition; }
    void setContentSecurityPolicyImposition(ContentSecurityPolicyImposition imposition) { m_contentSecurityPolicyImposition = imposition; }
    DefersLoadingPolicy defersLoadingPolicy() const { return m_defersLoadingPolicy; }
    void setDefersLoadingPolicy(DefersLoadingPolicy defersLoadingPolicy) { m_defersLoadingPolicy = defersLoadingPolicy; }
    CachingPolicy cachingPolicy() const { return m_cachingPolicy; }
    void setCachingPolicy(CachingPolicy cachingPolicy) { m_cachingPolicy = cachingPolicy; }
    const FetchOptions& fetchOptions() const { return m_fetchOptions; }
    FetchOptions& fetchOptions() { return m_fetchOptions; }
    void setFetchOptions(const FetchOptions& fetchOptions) { m_fetchOptions = fetchOptions; }

    unsigned m_sendLoadCallbacks : 1;
    unsigned m_sniffContent : 1;
    unsigned m_dataBufferingPolicy : 1;
    unsigned m_allowCredentials : 1; // Whether HTTP credentials and cookies are sent with the request.
    unsigned m_clientCredentialPolicy : 2; // When we should ask the client for credentials (if we allow credentials at all).
    unsigned m_credentialRequest: 1; // Whether the client (e.g. XHR) wanted credentials in the first place.
    unsigned m_securityCheck : 1;
    unsigned m_requestOriginPolicy : 2;
    unsigned m_certificateInfoPolicy : 1; // Whether the response should include certificate info.
    ContentSecurityPolicyImposition m_contentSecurityPolicyImposition { ContentSecurityPolicyImposition::DoPolicyCheck };
    DefersLoadingPolicy m_defersLoadingPolicy { DefersLoadingPolicy::AllowDefersLoading };
    CachingPolicy m_cachingPolicy { CachingPolicy::AllowCaching };
    FetchOptions m_fetchOptions;
};

} // namespace WebCore    

#endif // ResourceLoaderOptions_h
