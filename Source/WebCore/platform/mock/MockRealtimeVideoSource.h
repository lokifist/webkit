/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef MockRealtimeVideoSource_h
#define MockRealtimeVideoSource_h

#if ENABLE(MEDIA_STREAM)

#include "FontCascade.h"
#include "ImageBuffer.h"
#include "MockRealtimeMediaSource.h"
#include <wtf/RunLoop.h>

namespace WebCore {

class FloatRect;
class GraphicsContext;

class MockRealtimeVideoSource : public MockRealtimeMediaSource {
public:

    static Ref<MockRealtimeVideoSource> create();
    static Ref<MockRealtimeVideoSource> createMuted(const String& name);

    virtual ~MockRealtimeVideoSource() { }

    void setSize(const IntSize&);
    const IntSize& size() const { return m_size; }

    void setFrameRate(float);

protected:
    MockRealtimeVideoSource(const String& name = ASCIILiteral("Mock video device"));
    virtual void updatePlatformLayer() const { }

    ImageBuffer* imageBuffer() const;

private:
    void updateSettings(RealtimeMediaSourceSettings&) override;
    void initializeCapabilities(RealtimeMediaSourceCapabilities&) override;
    void initializeSupportedConstraints(RealtimeMediaSourceSupportedConstraints&) override;

    void startProducingData() override;
    void stopProducingData() override;

    void drawAnimation(GraphicsContext&);
    void drawText(GraphicsContext&);
    void drawBoxes(GraphicsContext&);

    PlatformLayer* platformLayer() const override { return nullptr; }
    RefPtr<Image> currentFrameImage() override;
    void paintCurrentFrameInContext(GraphicsContext&, const FloatRect&) override;

    void generateFrame();

    double elapsedTime();

    float m_baseFontSize { 0 };
    FontCascade m_timeFont;

    float m_bipBopFontSize { 0 };
    FontCascade m_bipBopFont;

    float m_statsFontSize { 0 };
    FontCascade m_statsFont;

    mutable std::unique_ptr<ImageBuffer> m_imageBuffer;

    IntSize m_size;
    Path m_path;
    DashArray m_dashWidths;

    double m_startTime { NAN };
    double m_elapsedTime { 0 };

    unsigned m_frameRate { 30 };
    unsigned m_frameNumber { 0 };

    RunLoop::Timer m_timer;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MockRealtimeVideoSource_h
