/*
 * Copyright (C) 2010 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2012 David Barton (dbarton@mathscribe.com). All rights reserved.
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

#ifndef RenderMathMLBlock_h
#define RenderMathMLBlock_h

#if ENABLE(MATHML)

#include "RenderFlexibleBox.h"
#include "RenderTable.h"
#include "StyleInheritedData.h"

#define ENABLE_DEBUG_MATH_LAYOUT 0

namespace WebCore {
    
class RenderMathMLOperator;

class RenderMathMLBlock : public RenderFlexibleBox {
public:
    RenderMathMLBlock(Element&, RenderStyle&&);
    RenderMathMLBlock(Document&, RenderStyle&&);

    bool isChildAllowed(const RenderObject&, const RenderStyle&) const override;
    
    // MathML defines an "embellished operator" as roughly an <mo> that may have subscripts,
    // superscripts, underscripts, overscripts, or a denominator (as in d/dx, where "d" is some
    // differential operator). The padding, precedence, and stretchiness of the base <mo> should
    // apply to the embellished operator as a whole. unembellishedOperator() checks for being an
    // embellished operator, and omits any embellishments.
    // FIXME: We don't yet handle all the cases in the MathML spec. See
    // https://bugs.webkit.org/show_bug.cgi?id=78617.
    virtual RenderMathMLOperator* unembellishedOperator() { return 0; }
    
    int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override;
    
#if ENABLE(DEBUG_MATH_LAYOUT)
    virtual void paint(PaintInfo&, const LayoutPoint&);
#endif
    
    LayoutUnit mathAxisHeight() const;
    LayoutUnit mirrorIfNeeded(LayoutUnit horizontalOffset, LayoutUnit boxWidth = 0) const;
    LayoutUnit mirrorIfNeeded(LayoutUnit horizontalOffset, const RenderBox& child) const { return mirrorIfNeeded(horizontalOffset, child.logicalWidth()); }
    
protected:
    static LayoutUnit ascentForChild(const RenderBox& child)
    {
        return child.firstLineBaseline().valueOr(child.logicalHeight());
    }

private:
    bool isRenderMathMLBlock() const final { return true; }
    const char* renderName() const override;
    bool isFlexibleBoxImpl() const override { return true; }
};

class RenderMathMLTable final : public RenderTable {
public:
    explicit RenderMathMLTable(Element& element, RenderStyle&& style)
        : RenderTable(element, WTFMove(style))
    {
    }
    
    Optional<int> firstLineBaseline() const override;
    
private:
    bool isRenderMathMLTable() const override { return true; }
    const char* renderName() const override { return "RenderMathMLTable"; }
};

// Parsing functions for MathML Length values
bool parseMathMLLength(const String&, LayoutUnit&, const RenderStyle*, bool allowNegative = true);
bool parseMathMLNamedSpace(const String&, LayoutUnit&, const RenderStyle*, bool allowNegative = true);

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderMathMLBlock, isRenderMathMLBlock())
SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderMathMLTable, isRenderMathMLTable())

#endif // ENABLE(MATHML)
#endif // RenderMathMLBlock_h
