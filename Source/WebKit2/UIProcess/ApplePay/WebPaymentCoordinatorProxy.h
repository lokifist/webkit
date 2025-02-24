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

#pragma once

#if ENABLE(APPLE_PAY)

#include "MessageReceiver.h"
#include <WebCore/PaymentHeaders.h>
#include <wtf/Forward.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>

namespace IPC {
class DataReference;
}

namespace WebCore {
enum class PaymentAuthorizationStatus;
class Payment;
class PaymentContact;
class PaymentMerchantSession;
class PaymentMethod;
class URL;
}

OBJC_CLASS NSWindow;
OBJC_CLASS PKPaymentAuthorizationViewController;
OBJC_CLASS WKPaymentAuthorizationViewControllerDelegate;

namespace WebKit {

class WebPageProxy;

class WebPaymentCoordinatorProxy : private IPC::MessageReceiver {
public:
    explicit WebPaymentCoordinatorProxy(WebPageProxy&);
    ~WebPaymentCoordinatorProxy();

    void didCancelPayment();
    void validateMerchant(const WebCore::URL&);
    void didAuthorizePayment(const WebCore::Payment&);
    void didSelectShippingMethod(const WebCore::PaymentRequest::ShippingMethod&);
    void didSelectShippingContact(const WebCore::PaymentContact&);
    void didSelectPaymentMethod(const WebCore::PaymentMethod&);

    void hidePaymentUI();

private:
    // IPC::MessageReceiver.
    void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::MessageDecoder&, std::unique_ptr<IPC::MessageEncoder>&) override;

    // Message handlers.
    void canMakePayments(bool& reply);
    void canMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, uint64_t requestID);
    void showPaymentUI(const String& originatingURLString, const Vector<String>& linkIconURLStrings, const WebCore::PaymentRequest&);
    void completeMerchantValidation(const WebCore::PaymentMerchantSession&);
    void completeShippingMethodSelection(uint32_t opaqueStatus, const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void completeShippingContactSelection(uint32_t opaqueStatus, const Vector<WebCore::PaymentRequest::ShippingMethod>& newShippingMethods, const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void completePaymentMethodSelection(const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void completePaymentSession(uint32_t opaqueStatus);
    void abortPaymentSession();

    bool canBegin() const;
    bool canCancel() const;
    bool canCompletePayment() const;
    bool canAbort() const;

    void didReachFinalState();

    bool platformCanMakePayments();
    void platformCanMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, std::function<void (bool)> completionHandler);
    void platformShowPaymentUI(const WebCore::URL& originatingURL, const Vector<WebCore::URL>& linkIconURLs, const WebCore::PaymentRequest&, std::function<void (bool)> completionHandler);
    void platformCompleteMerchantValidation(const WebCore::PaymentMerchantSession&);
    void platformCompleteShippingMethodSelection(WebCore::PaymentAuthorizationStatus, const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void platformCompleteShippingContactSelection(WebCore::PaymentAuthorizationStatus, const Vector<WebCore::PaymentRequest::ShippingMethod>& newShippingMethods, const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void platformCompletePaymentMethodSelection(const Optional<WebCore::PaymentRequest::TotalAndLineItems>&);
    void platformCompletePaymentSession(WebCore::PaymentAuthorizationStatus);

    WebPageProxy& m_webPageProxy;
    WeakPtrFactory<WebPaymentCoordinatorProxy> m_weakPtrFactory;

    enum class State {
        // Idle - Nothing's happening.
        Idle,

        // Activating - Waiting to show the payment UI.
        Activating,

        // Active - Showing payment UI.
        Active,

        // Authorized - Dispatching the authorized event and waiting for the paymentSessionCompleted message.
        Authorized,

        // ShippingMethodSelected - Dispatching the shippingmethodselected event and waiting for a reply.
        ShippingMethodSelected,

        // ShippingContactSelected - Dispatching the shippingcontactselected event and waiting for a reply.
        ShippingContactSelected,

        // PaymentMethodSelected - Dispatching the paymentmethodselected event and waiting for a reply.
        PaymentMethodSelected,
    } m_state;

    enum class MerchantValidationState {
        // Idle - Nothing's happening.
        Idle,

        // Validating - Dispatching the validatemerchant event and waiting for a reply.
        Validating,

        // ValidationComplete - A merchant session has been sent along to PassKit.
        ValidationComplete
    } m_merchantValidationState;

    RetainPtr<PKPaymentAuthorizationViewController> m_paymentAuthorizationViewController;
    RetainPtr<WKPaymentAuthorizationViewControllerDelegate> m_paymentAuthorizationViewControllerDelegate;

#if PLATFORM(MAC)
    uint64_t m_showPaymentUIRequestSeed { 0 };
    RetainPtr<NSWindow> m_sheetWindow;
#endif
};

}

#endif
