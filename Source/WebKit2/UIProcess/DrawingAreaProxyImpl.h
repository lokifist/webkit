/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DrawingAreaProxyImpl_h
#define DrawingAreaProxyImpl_h

#include "BackingStore.h"
#include "DrawingAreaProxy.h"
#include "LayerTreeContext.h"
#include <wtf/RunLoop.h>

namespace WebCore {
class Region;
}

namespace WebKit {

class DrawingAreaProxyImpl : public DrawingAreaProxy {
public:
    explicit DrawingAreaProxyImpl(WebPageProxy&);
    virtual ~DrawingAreaProxyImpl();

    void paint(BackingStore::PlatformGraphicsContext, const WebCore::IntRect&, WebCore::Region& unpaintedRegion);

    bool isInAcceleratedCompositingMode() const { return alwaysUseCompositing() || !m_layerTreeContext.isEmpty(); }

    bool hasReceivedFirstUpdate() const { return m_hasReceivedFirstUpdate; }

#if USE(TEXTURE_MAPPER) && PLATFORM(GTK)
    void setNativeSurfaceHandleForCompositing(uint64_t);
    void destroyNativeSurfaceHandleForCompositing();
#endif

    void dispatchAfterEnsuringDrawing(std::function<void (CallbackBase::Error)>) override;

private:
    // DrawingAreaProxy
    void sizeDidChange() override;
    void deviceScaleFactorDidChange() override;

    void setBackingStoreIsDiscardable(bool) override;
    void waitForBackingStoreUpdateOnNextPaint() override;

    // IPC message handlers
    void update(uint64_t backingStoreStateID, const UpdateInfo&) override;
    void didUpdateBackingStoreState(uint64_t backingStoreStateID, const UpdateInfo&, const LayerTreeContext&) override;
    void enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) override;
    void exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo&) override;
    void updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) override;
    void willEnterAcceleratedCompositingMode(uint64_t backingStoreStateID) override;

    void incorporateUpdate(const UpdateInfo&);

    enum RespondImmediatelyOrNot { DoNotRespondImmediately, RespondImmediately };
    void backingStoreStateDidChange(RespondImmediatelyOrNot);
    void sendUpdateBackingStoreState(RespondImmediatelyOrNot);
    void waitForAndDispatchDidUpdateBackingStoreState();

    void enterAcceleratedCompositingMode(const LayerTreeContext&);
    void exitAcceleratedCompositingMode();
    void updateAcceleratedCompositingMode(const LayerTreeContext&);
    bool alwaysUseCompositing() const;

    void discardBackingStoreSoon();
    void discardBackingStore();

    // The state ID corresponding to our current backing store. Updated whenever we allocate
    // a new backing store. Any messages received that correspond to an earlier state are ignored,
    // as they don't apply to our current backing store.
    uint64_t m_currentBackingStoreStateID;

    // The next backing store state ID we will request the web process update to. Incremented
    // whenever our state changes in a way that will require a new backing store to be allocated.
    uint64_t m_nextBackingStoreStateID;

    // The current layer tree context.
    LayerTreeContext m_layerTreeContext;

    // Whether we've sent a UpdateBackingStoreState message and are now waiting for a DidUpdateBackingStoreState message.
    // Used to throttle UpdateBackingStoreState messages so we don't send them faster than the Web process can handle.
    bool m_isWaitingForDidUpdateBackingStoreState;
    
    // For a new Drawing Area don't draw anything until the WebProcess has sent over the first content.
    bool m_hasReceivedFirstUpdate;

    bool m_isBackingStoreDiscardable;
    std::unique_ptr<BackingStore> m_backingStore;

    RunLoop::Timer m_discardBackingStoreTimer;
};

} // namespace WebKit

#endif // DrawingAreaProxyImpl_h
