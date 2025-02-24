/*
 * Copyright (C) 2014 Frédéric Wang (fred.wang@free.fr). All rights reserved.
 * Copyright (C) 2016 Igalia S.L.
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

#ifndef RenderMathMLToken_h
#define RenderMathMLToken_h

#if ENABLE(MATHML)

#include "MathMLTextElement.h"
#include "RenderMathMLBlock.h"
#include "RenderText.h"

namespace WebCore {
    
class RenderMathMLToken : public RenderMathMLBlock {
public:
    RenderMathMLToken(Element&, RenderStyle&&);
    RenderMathMLToken(Document&, RenderStyle&&);

    MathMLTextElement& element() { return static_cast<MathMLTextElement&>(nodeForNonAnonymous()); }

    bool isChildAllowed(const RenderObject&, const RenderStyle&) const override { return true; };
    virtual void updateTokenContent();
    void updateFromElement() override;

    void paint(PaintInfo&, const LayoutPoint&) override;
    void paintChildren(PaintInfo& forSelf, const LayoutPoint&, PaintInfo& forChild, bool usePrintRect) override;
    Optional<int> firstLineBaseline() const override;
    void layoutBlock(bool relayoutChildren, LayoutUnit pageLogicalHeight = 0) override;
    void computePreferredLogicalWidths() override;

private:
    bool isRenderMathMLToken() const final { return true; }
    const char* renderName() const override { return "RenderMathMLToken"; }
    void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;
    void updateMathVariantGlyph();
    void setMathVariantGlyphDirty()
    {
        m_mathVariantGlyphDirty = true;
        setNeedsLayoutAndPrefWidthsRecalc();
    }
    GlyphData m_mathVariantGlyph;
    bool m_mathVariantGlyphDirty;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderMathMLToken, isRenderMathMLToken())

#endif // ENABLE(MATHML)
#endif // RenderMathMLToken_h
