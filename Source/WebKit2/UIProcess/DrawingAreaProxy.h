/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#ifndef DrawingAreaProxy_h
#define DrawingAreaProxy_h

#include "DrawingAreaInfo.h"
#include "GenericCallback.h"
#include "MessageReceiver.h"
#include <WebCore/FloatRect.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <chrono>
#include <stdint.h>
#include <wtf/Noncopyable.h>
#include <wtf/RunLoop.h>
#include <wtf/TypeCasts.h>

#if PLATFORM(COCOA)
namespace WebCore {
class MachSendRight;
}
#endif

namespace WebKit {

class LayerTreeContext;
class CoordinatedLayerTreeHostProxy;
class UpdateInfo;
class WebPageProxy;

class DrawingAreaProxy : public IPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(DrawingAreaProxy);

public:
    virtual ~DrawingAreaProxy();

    DrawingAreaType type() const { return m_type; }

    virtual void deviceScaleFactorDidChange() = 0;

    // FIXME: These should be pure virtual.
    virtual void setBackingStoreIsDiscardable(bool) { }

    virtual void waitForBackingStoreUpdateOnNextPaint() { }

    const WebCore::IntSize& size() const { return m_size; }
    void setSize(const WebCore::IntSize&, const WebCore::IntSize&, const WebCore::IntSize& scrollOffset);

    // The timeout we use when waiting for a DidUpdateGeometry message.
    static constexpr std::chrono::milliseconds didUpdateBackingStoreStateTimeout() { return std::chrono::milliseconds(500); }

    virtual void waitForPossibleGeometryUpdate(std::chrono::milliseconds = didUpdateBackingStoreStateTimeout()) { }

    virtual void colorSpaceDidChange() { }
    virtual void minimumLayoutSizeDidChange() { }

    virtual void adjustTransientZoom(double, WebCore::FloatPoint) { }
    virtual void commitTransientZoom(double, WebCore::FloatPoint) { }

#if PLATFORM(MAC)
    virtual void setViewExposedRect(Optional<WebCore::FloatRect>);
    Optional<WebCore::FloatRect> viewExposedRect() const { return m_viewExposedRect; }
    void viewExposedRectChangedTimerFired();
#endif

    virtual void updateDebugIndicator() { }

    virtual void waitForDidUpdateViewState() { }
    
    virtual void dispatchAfterEnsuringDrawing(std::function<void (CallbackBase::Error)>) { ASSERT_NOT_REACHED(); }

    // Hide the content until the currently pending update arrives.
    virtual void hideContentUntilPendingUpdate() { ASSERT_NOT_REACHED(); }

    // Hide the content until any update arrives.
    virtual void hideContentUntilAnyUpdate() { ASSERT_NOT_REACHED(); }

    virtual bool hasVisibleContent() const { return true; }

    virtual void willSendUpdateGeometry() { }

#if PLATFORM(COCOA)
    virtual WebCore::MachSendRight createFence();
#endif

protected:
    explicit DrawingAreaProxy(DrawingAreaType, WebPageProxy&);

    DrawingAreaType m_type;
    WebPageProxy& m_webPageProxy;

    WebCore::IntSize m_size;
    WebCore::IntSize m_layerPosition;
    WebCore::IntSize m_scrollOffset;

    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;

private:
    virtual void sizeDidChange() = 0;

    // Message handlers.
    // FIXME: These should be pure virtual.
    virtual void update(uint64_t /* backingStoreStateID */, const UpdateInfo&) { }
    virtual void didUpdateBackingStoreState(uint64_t /* backingStoreStateID */, const UpdateInfo&, const LayerTreeContext&) { }
    virtual void enterAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const LayerTreeContext&) { }
    virtual void exitAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const UpdateInfo&) { }
    virtual void updateAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const LayerTreeContext&) { }
    virtual void willEnterAcceleratedCompositingMode(uint64_t /* backingStoreStateID */) { }
#if PLATFORM(COCOA)
    virtual void didUpdateGeometry() { }
    virtual void intrinsicContentSizeDidChange(const WebCore::IntSize&) { }

#if PLATFORM(MAC)
    RunLoop::Timer m_viewExposedRectChangedTimer;
    Optional<WebCore::FloatRect> m_viewExposedRect;
    Optional<WebCore::FloatRect> m_lastSentViewExposedRect;
#endif // PLATFORM(MAC)
#endif
};

} // namespace WebKit

#define SPECIALIZE_TYPE_TRAITS_DRAWING_AREA_PROXY(ToValueTypeName, ProxyType) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebKit::ToValueTypeName) \
    static bool isType(const WebKit::DrawingAreaProxy& proxy) { return proxy.type() == WebKit::ProxyType; } \
SPECIALIZE_TYPE_TRAITS_END()

#endif // DrawingAreaProxy_h
