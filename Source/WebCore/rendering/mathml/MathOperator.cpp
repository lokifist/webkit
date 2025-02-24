/*
 * Copyright (C) 2016 Igalia S.L. All rights reserved.
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

#include "config.h"

#if ENABLE(MATHML)
#include "MathOperator.h"

#include "RenderStyle.h"
#include "StyleInheritedData.h"

static const unsigned kRadicalOperator = 0x221A;
static const unsigned kMaximumExtensionCount = 128;

namespace WebCore {

static inline FloatRect boundsForGlyph(const GlyphData& data)
{
    return data.isValid() ? data.font->boundsForGlyph(data.glyph) : FloatRect();
}

static inline float heightForGlyph(const GlyphData& data)
{
    return boundsForGlyph(data).height();
}

static inline void getAscentAndDescentForGlyph(const GlyphData& data, LayoutUnit& ascent, LayoutUnit& descent)
{
    FloatRect bounds = boundsForGlyph(data);
    ascent = -bounds.y();
    descent = bounds.maxY();
}

static inline float advanceWidthForGlyph(const GlyphData& data)
{
    return data.isValid() ? data.font->widthForGlyph(data.glyph) : 0;
}

// FIXME: This hardcoded data can be removed when OpenType MATH font are widely available (http://wkbug/156837).
struct StretchyCharacter {
    UChar character;
    UChar topChar;
    UChar extensionChar;
    UChar bottomChar;
    UChar middleChar;
};
// The first leftRightPairsCount pairs correspond to left/right fences that can easily be mirrored in RTL.
static const short leftRightPairsCount = 5;
static const StretchyCharacter stretchyCharacters[14] = {
    { 0x28  , 0x239b, 0x239c, 0x239d, 0x0    }, // left parenthesis
    { 0x29  , 0x239e, 0x239f, 0x23a0, 0x0    }, // right parenthesis
    { 0x5b  , 0x23a1, 0x23a2, 0x23a3, 0x0    }, // left square bracket
    { 0x5d  , 0x23a4, 0x23a5, 0x23a6, 0x0    }, // right square bracket
    { 0x7b  , 0x23a7, 0x23aa, 0x23a9, 0x23a8 }, // left curly bracket
    { 0x7d  , 0x23ab, 0x23aa, 0x23ad, 0x23ac }, // right curly bracket
    { 0x2308, 0x23a1, 0x23a2, 0x23a2, 0x0    }, // left ceiling
    { 0x2309, 0x23a4, 0x23a5, 0x23a5, 0x0    }, // right ceiling
    { 0x230a, 0x23a2, 0x23a2, 0x23a3, 0x0    }, // left floor
    { 0x230b, 0x23a5, 0x23a5, 0x23a6, 0x0    }, // right floor
    { 0x7c  , 0x7c,   0x7c,   0x7c,   0x0    }, // vertical bar
    { 0x2016, 0x2016, 0x2016, 0x2016, 0x0    }, // double vertical line
    { 0x2225, 0x2225, 0x2225, 0x2225, 0x0    }, // parallel to
    { 0x222b, 0x2320, 0x23ae, 0x2321, 0x0    } // integral sign
};

void MathOperator::setOperator(const RenderStyle& style, UChar baseCharacter, Type operatorType)
{
    m_baseCharacter = baseCharacter;
    m_operatorType = operatorType;
    reset(style);
}

void MathOperator::reset(const RenderStyle& style)
{
    m_stretchType = StretchType::Unstretched;
    m_maxPreferredWidth = 0;
    m_width = 0;
    m_ascent = 0;
    m_descent = 0;
    m_italicCorrection = 0;
    m_radicalVerticalScale = 1;

    // We use the base size for the calculation of the preferred width.
    GlyphData baseGlyph;
    if (!getBaseGlyph(style, baseGlyph))
        return;
    m_maxPreferredWidth = m_width = advanceWidthForGlyph(baseGlyph);
    getAscentAndDescentForGlyph(baseGlyph, m_ascent, m_descent);

    if (m_operatorType == Type::VerticalOperator)
        calculateStretchyData(style, true); // We also take into account the width of larger sizes for the calculation of the preferred width.
    else if (m_operatorType == Type::DisplayOperator)
        calculateDisplayStyleLargeOperator(style); // We can directly select the size variant and determine the final metrics.
}

LayoutUnit MathOperator::stretchSize() const
{
    ASSERT(m_operatorType == Type::VerticalOperator || m_operatorType == Type::HorizontalOperator);
    return m_operatorType == Type::VerticalOperator ? m_ascent + m_descent : m_width;
}

bool MathOperator::getBaseGlyph(const RenderStyle& style, GlyphData& baseGlyph) const
{
    baseGlyph = style.fontCascade().glyphDataForCharacter(m_baseCharacter, !style.isLeftToRightDirection());
    return baseGlyph.isValid() && baseGlyph.font == &style.fontCascade().primaryFont();
}

void MathOperator::setSizeVariant(const GlyphData& sizeVariant)
{
    ASSERT(sizeVariant.isValid());
    ASSERT(sizeVariant.font->mathData());
    m_stretchType = StretchType::SizeVariant;
    m_variant = sizeVariant;
    m_width = advanceWidthForGlyph(sizeVariant);
    getAscentAndDescentForGlyph(sizeVariant, m_ascent, m_descent);
}

void MathOperator::setGlyphAssembly(const GlyphAssemblyData& assemblyData)
{
    ASSERT(m_operatorType == Type::VerticalOperator || m_operatorType == Type::HorizontalOperator);
    m_stretchType = StretchType::GlyphAssembly;
    m_assembly = assemblyData;
    if (m_operatorType == Type::VerticalOperator) {
        m_width = 0;
        m_width = std::max<LayoutUnit>(m_width, advanceWidthForGlyph(m_assembly.topOrRight));
        m_width = std::max<LayoutUnit>(m_width, advanceWidthForGlyph(m_assembly.extension));
        m_width = std::max<LayoutUnit>(m_width, advanceWidthForGlyph(m_assembly.bottomOrLeft));
        m_width = std::max<LayoutUnit>(m_width, advanceWidthForGlyph(m_assembly.middle));
    } else {
        m_ascent = 0;
        m_descent = 0;
        LayoutUnit ascent, descent;
        getAscentAndDescentForGlyph(m_assembly.bottomOrLeft, ascent, descent);
        m_ascent = std::max(m_ascent, ascent);
        m_descent = std::max(m_descent, descent);
        getAscentAndDescentForGlyph(m_assembly.extension, ascent, descent);
        m_ascent = std::max(m_ascent, ascent);
        m_descent = std::max(m_descent, descent);
        getAscentAndDescentForGlyph(m_assembly.topOrRight, ascent, descent);
        m_ascent = std::max(m_ascent, ascent);
        m_descent = std::max(m_descent, descent);
        getAscentAndDescentForGlyph(m_assembly.middle, ascent, descent);
        m_ascent = std::max(m_ascent, ascent);
        m_descent = std::max(m_descent, descent);
    }
}

void MathOperator::calculateDisplayStyleLargeOperator(const RenderStyle& style)
{
    ASSERT(m_operatorType == Type::DisplayOperator);

    GlyphData baseGlyph;
    if (!getBaseGlyph(style, baseGlyph) || !baseGlyph.font->mathData())
        return;

    // The value of displayOperatorMinHeight is sometimes too small, so we ensure that it is at least \sqrt{2} times the size of the base glyph.
    float displayOperatorMinHeight = std::max(heightForGlyph(baseGlyph) * sqrtOfTwoFloat, baseGlyph.font->mathData()->getMathConstant(*baseGlyph.font, OpenTypeMathData::DisplayOperatorMinHeight));

    Vector<Glyph> sizeVariants;
    Vector<OpenTypeMathData::AssemblyPart> assemblyParts;
    baseGlyph.font->mathData()->getMathVariants(baseGlyph.glyph, true, sizeVariants, assemblyParts);

    // We choose the first size variant that is larger than the expected displayOperatorMinHeight and otherwise fallback to the largest variant.
    for (auto& sizeVariant : sizeVariants) {
        GlyphData glyphData(sizeVariant, baseGlyph.font);
        setSizeVariant(glyphData);
        m_maxPreferredWidth = m_width;
        m_italicCorrection = glyphData.font->mathData()->getItalicCorrection(*glyphData.font, glyphData.glyph);
        if (heightForGlyph(glyphData) >= displayOperatorMinHeight)
            break;
    }
}

bool MathOperator::calculateGlyphAssemblyFallback(const RenderStyle& style, const Vector<OpenTypeMathData::AssemblyPart>& assemblyParts, GlyphAssemblyData& assemblyData) const
{
    // The structure of the Open Type Math table is a bit more general than the one currently used by the MathOperator code, so we try to fallback in a reasonable way.
    // FIXME: MathOperator should support the most general format (https://bugs.webkit.org/show_bug.cgi?id=130327).
    // We use the approach of the copyComponents function in github.com/mathjax/MathJax-dev/blob/master/fonts/OpenTypeMath/fontUtil.py

    // We count the number of non extender pieces.
    int nonExtenderCount = 0;
    for (auto& part : assemblyParts) {
        if (!part.isExtender)
            nonExtenderCount++;
    }
    if (nonExtenderCount > 3)
        return false; // This is not supported: there are too many pieces.

    // We now browse the list of pieces from left to right for horizontal operators and from bottom to top for vertical operators.
    enum PartType {
        Start,
        ExtenderBetweenStartAndMiddle,
        Middle,
        ExtenderBetweenMiddleAndEnd,
        End,
        None
    };
    PartType expectedPartType = Start;
    assemblyData.extension.glyph = 0;
    assemblyData.middle.glyph = 0;
    for (auto& part : assemblyParts) {
        if (nonExtenderCount < 3) {
            // If we only have at most two non-extenders then we skip the middle glyph.
            if (expectedPartType == ExtenderBetweenStartAndMiddle)
                expectedPartType = ExtenderBetweenMiddleAndEnd;
            else if (expectedPartType == Middle)
                expectedPartType = End;
        }
        if (part.isExtender) {
            if (!assemblyData.extension.glyph)
                assemblyData.extension.glyph = part.glyph; // We copy the extender part.
            else if (assemblyData.extension.glyph != part.glyph)
                return false; // This is not supported: the assembly has different extenders.

            switch (expectedPartType) {
            case Start:
                // We ignore the left/bottom part.
                expectedPartType = ExtenderBetweenStartAndMiddle;
                continue;
            case Middle:
                // We ignore the middle part.
                expectedPartType = ExtenderBetweenMiddleAndEnd;
                continue;
            case End:
            case None:
                // This is not supported: we got an unexpected extender.
                return false;
            case ExtenderBetweenStartAndMiddle:
            case ExtenderBetweenMiddleAndEnd:
                // We ignore multiple consecutive extenders.
                continue;
            }
        }

        switch (expectedPartType) {
        case Start:
            // We copy the left/bottom part.
            assemblyData.bottomOrLeft.glyph = part.glyph;
            expectedPartType = ExtenderBetweenStartAndMiddle;
            continue;
        case ExtenderBetweenStartAndMiddle:
        case Middle:
            // We copy the middle part.
            assemblyData.middle.glyph = part.glyph;
            expectedPartType = ExtenderBetweenMiddleAndEnd;
            continue;
        case ExtenderBetweenMiddleAndEnd:
        case End:
            // We copy the right/top part.
            assemblyData.topOrRight.glyph = part.glyph;
            expectedPartType = None;
            continue;
        case None:
            // This is not supported: we got an unexpected non-extender part.
            return false;
        }
    }

    if (!assemblyData.extension.glyph)
        return false; // This is not supported: we always assume that we have an extension glyph.

    // If we don't have top/bottom glyphs, we use the extension glyph.
    if (!assemblyData.topOrRight.glyph)
        assemblyData.topOrRight.glyph = assemblyData.extension.glyph;
    if (!assemblyData.bottomOrLeft.glyph)
        assemblyData.bottomOrLeft.glyph = assemblyData.extension.glyph;

    assemblyData.topOrRight.font = &style.fontCascade().primaryFont();
    assemblyData.extension.font = assemblyData.topOrRight.font;
    assemblyData.bottomOrLeft.font = assemblyData.topOrRight.font;
    assemblyData.middle.font = assemblyData.middle.glyph ? assemblyData.topOrRight.font : nullptr;

    return true;
}

void MathOperator::calculateStretchyData(const RenderStyle& style, bool calculateMaxPreferredWidth, LayoutUnit targetSize)
{
    ASSERT(m_operatorType == Type::VerticalOperator || m_operatorType == Type::HorizontalOperator);
    ASSERT(!calculateMaxPreferredWidth || m_operatorType == Type::VerticalOperator);
    bool isVertical = m_operatorType == Type::VerticalOperator;

    GlyphData baseGlyph;
    if (!getBaseGlyph(style, baseGlyph))
        return;

    if (!calculateMaxPreferredWidth) {
        // We do not stretch if the base glyph is large enough.
        float baseSize = isVertical ? heightForGlyph(baseGlyph) : advanceWidthForGlyph(baseGlyph);
        if (targetSize <= baseSize)
            return;
    }

    GlyphAssemblyData assemblyData;
    if (baseGlyph.font->mathData()) {
        Vector<Glyph> sizeVariants;
        Vector<OpenTypeMathData::AssemblyPart> assemblyParts;
        baseGlyph.font->mathData()->getMathVariants(baseGlyph.glyph, isVertical, sizeVariants, assemblyParts);
        // We verify the size variants.
        for (auto& sizeVariant : sizeVariants) {
            GlyphData glyphData(sizeVariant, baseGlyph.font);
            if (calculateMaxPreferredWidth)
                m_maxPreferredWidth = std::max<LayoutUnit>(m_maxPreferredWidth, advanceWidthForGlyph(glyphData));
            else {
                setSizeVariant(glyphData);
                LayoutUnit size = isVertical ? heightForGlyph(glyphData) : advanceWidthForGlyph(glyphData);
                if (size >= targetSize)
                    return;
            }
        }

        // We verify if there is a construction.
        if (!calculateGlyphAssemblyFallback(style, assemblyParts, assemblyData))
            return;
    } else {
        if (!isVertical)
            return;

        // If the font does not have a MATH table, we fallback to the Unicode-only constructions.
        const StretchyCharacter* stretchyCharacter = nullptr;
        const unsigned maxIndex = WTF_ARRAY_LENGTH(stretchyCharacters);
        for (unsigned index = 0; index < maxIndex; ++index) {
            if (stretchyCharacters[index].character == m_baseCharacter) {
                stretchyCharacter = &stretchyCharacters[index];
                if (!style.isLeftToRightDirection() && index < leftRightPairsCount * 2) {
                    // If we are in right-to-left direction we select the mirrored form by adding -1 or +1 according to the parity of index.
                    index += index % 2 ? -1 : 1;
                }
                break;
            }
        }

        // Unicode contains U+23B7 RADICAL SYMBOL BOTTOM but it is generally not provided by fonts without a MATH table.
        // Moreover, it's not clear what the proper vertical extender or top hook would be.
        // Hence we fallback to scaling the base glyph vertically.
        if (!calculateMaxPreferredWidth && m_baseCharacter == kRadicalOperator) {
            LayoutUnit height = m_ascent + m_descent;
            if (height > 0 && height < targetSize) {
                m_radicalVerticalScale = targetSize.toFloat() / height;
                m_ascent *= m_radicalVerticalScale;
                m_descent *= m_radicalVerticalScale;
            }
            return;
        }

        // If we didn't find a stretchy character set for this character, we don't know how to stretch it.
        if (!stretchyCharacter)
            return;

        // We convert the list of Unicode characters into a list of glyph data.
        assemblyData.topOrRight = style.fontCascade().glyphDataForCharacter(stretchyCharacter->topChar, false);
        assemblyData.extension = style.fontCascade().glyphDataForCharacter(stretchyCharacter->extensionChar, false);
        assemblyData.bottomOrLeft = style.fontCascade().glyphDataForCharacter(stretchyCharacter->bottomChar, false);
        assemblyData.middle = stretchyCharacter->middleChar ? style.fontCascade().glyphDataForCharacter(stretchyCharacter->middleChar, false) : GlyphData();
    }

    // If we are measuring the maximum width, verify each component.
    if (calculateMaxPreferredWidth) {
        m_maxPreferredWidth = std::max<LayoutUnit>(m_maxPreferredWidth, advanceWidthForGlyph(assemblyData.topOrRight));
        m_maxPreferredWidth = std::max<LayoutUnit>(m_maxPreferredWidth, advanceWidthForGlyph(assemblyData.extension));
        m_maxPreferredWidth = std::max<LayoutUnit>(m_maxPreferredWidth, advanceWidthForGlyph(assemblyData.middle));
        m_maxPreferredWidth = std::max<LayoutUnit>(m_maxPreferredWidth, advanceWidthForGlyph(assemblyData.bottomOrLeft));
        return;
    }

    // We ensure that the size is large enough to avoid glyph overlaps.
    float minSize = isVertical ?
        heightForGlyph(assemblyData.topOrRight) + heightForGlyph(assemblyData.middle) + heightForGlyph(assemblyData.bottomOrLeft)
        : advanceWidthForGlyph(assemblyData.bottomOrLeft) + advanceWidthForGlyph(assemblyData.middle) + advanceWidthForGlyph(assemblyData.topOrRight);
    if (minSize > targetSize)
        return;

    setGlyphAssembly(assemblyData);
}

void MathOperator::stretchTo(const RenderStyle& style, LayoutUnit ascent, LayoutUnit descent)
{
    ASSERT(m_operatorType == Type::VerticalOperator);
    calculateStretchyData(style, false, ascent + descent);
    if (m_stretchType == StretchType::GlyphAssembly) {
        m_ascent = ascent;
        m_descent = descent;
    }
}

void MathOperator::stretchTo(const RenderStyle& style, LayoutUnit width)
{
    ASSERT(m_operatorType == Type::HorizontalOperator);
    calculateStretchyData(style, false, width);
    if (m_stretchType == StretchType::GlyphAssembly)
        m_width = width;
}

LayoutRect MathOperator::paintGlyph(const RenderStyle& style, PaintInfo& info, const GlyphData& data, const LayoutPoint& origin, GlyphPaintTrimming trim)
{
    FloatRect glyphBounds = boundsForGlyph(data);

    LayoutRect glyphPaintRect(origin, LayoutSize(glyphBounds.x() + glyphBounds.width(), glyphBounds.height()));
    glyphPaintRect.setY(origin.y() + glyphBounds.y());

    // In order to have glyphs fit snugly with one another we snap the connecting edges to pixel boundaries
    // and trim off one pixel. The pixel trim is to account for fonts that have edge pixels that have less
    // than full coverage. These edge pixels can introduce small seams between connected glyphs.
    FloatRect clipBounds = info.rect;
    switch (trim) {
    case TrimTop:
        glyphPaintRect.shiftYEdgeTo(glyphPaintRect.y().ceil() + 1);
        clipBounds.shiftYEdgeTo(glyphPaintRect.y());
        break;
    case TrimBottom:
        glyphPaintRect.shiftMaxYEdgeTo(glyphPaintRect.maxY().floor() - 1);
        clipBounds.shiftMaxYEdgeTo(glyphPaintRect.maxY());
        break;
    case TrimTopAndBottom:
        glyphPaintRect.shiftYEdgeTo(glyphPaintRect.y().ceil() + 1);
        glyphPaintRect.shiftMaxYEdgeTo(glyphPaintRect.maxY().floor() - 1);
        clipBounds.shiftYEdgeTo(glyphPaintRect.y());
        clipBounds.shiftMaxYEdgeTo(glyphPaintRect.maxY());
        break;
    case TrimLeft:
        glyphPaintRect.shiftXEdgeTo(glyphPaintRect.x().ceil() + 1);
        clipBounds.shiftXEdgeTo(glyphPaintRect.x());
        break;
    case TrimRight:
        glyphPaintRect.shiftMaxXEdgeTo(glyphPaintRect.maxX().floor() - 1);
        clipBounds.shiftMaxXEdgeTo(glyphPaintRect.maxX());
        break;
    case TrimLeftAndRight:
        glyphPaintRect.shiftXEdgeTo(glyphPaintRect.x().ceil() + 1);
        glyphPaintRect.shiftMaxXEdgeTo(glyphPaintRect.maxX().floor() - 1);
        clipBounds.shiftXEdgeTo(glyphPaintRect.x());
        clipBounds.shiftMaxXEdgeTo(glyphPaintRect.maxX());
    }

    // Clipping the enclosing IntRect avoids any potential issues at joined edges.
    GraphicsContextStateSaver stateSaver(info.context());
    info.context().clip(clipBounds);

    GlyphBuffer buffer;
    buffer.add(data.glyph, data.font, advanceWidthForGlyph(data));
    info.context().drawGlyphs(style.fontCascade(), *data.font, buffer, 0, 1, origin);

    return glyphPaintRect;
}

void MathOperator::fillWithVerticalExtensionGlyph(const RenderStyle& style, PaintInfo& info, const LayoutPoint& from, const LayoutPoint& to)
{
    ASSERT(m_operatorType == Type::VerticalOperator);
    ASSERT(m_stretchType == StretchType::GlyphAssembly);
    ASSERT(m_assembly.extension.isValid());
    ASSERT(from.y() <= to.y());

    // If there is no space for the extension glyph, we don't need to do anything.
    if (from.y() == to.y())
        return;

    GraphicsContextStateSaver stateSaver(info.context());

    FloatRect glyphBounds = boundsForGlyph(m_assembly.extension);

    // Clipping the extender region here allows us to draw the bottom extender glyph into the
    // regions of the bottom glyph without worrying about overdraw (hairy pixels) and simplifies later clipping.
    LayoutRect clipBounds = info.rect;
    clipBounds.shiftYEdgeTo(from.y());
    clipBounds.shiftMaxYEdgeTo(to.y());
    info.context().clip(clipBounds);

    // Trimming may remove up to two pixels from the top of the extender glyph, so we move it up by two pixels.
    float offsetToGlyphTop = glyphBounds.y() + 2;
    LayoutPoint glyphOrigin = LayoutPoint(from.x(), from.y() - offsetToGlyphTop);
    FloatRect lastPaintedGlyphRect(from, FloatSize());

    // In practice, only small stretch sizes are requested but we limit the number of glyphs to avoid hangs.
    for (unsigned extensionCount = 0; lastPaintedGlyphRect.maxY() < to.y() && extensionCount < kMaximumExtensionCount; extensionCount++) {
        lastPaintedGlyphRect = paintGlyph(style, info, m_assembly.extension, glyphOrigin, TrimTopAndBottom);
        glyphOrigin.setY(glyphOrigin.y() + lastPaintedGlyphRect.height());

        // There's a chance that if the font size is small enough the glue glyph has been reduced to an empty rectangle
        // with trimming. In that case we just draw nothing.
        if (lastPaintedGlyphRect.isEmpty())
            break;
    }
}

void MathOperator::fillWithHorizontalExtensionGlyph(const RenderStyle& style, PaintInfo& info, const LayoutPoint& from, const LayoutPoint& to)
{
    ASSERT(m_operatorType == Type::HorizontalOperator);
    ASSERT(m_stretchType == StretchType::GlyphAssembly);
    ASSERT(m_assembly.extension.isValid());
    ASSERT(from.x() <= to.x());
    ASSERT(from.y() == to.y());

    // If there is no space for the extension glyph, we don't need to do anything.
    if (from.x() == to.x())
        return;

    GraphicsContextStateSaver stateSaver(info.context());

    // Clipping the extender region here allows us to draw the bottom extender glyph into the
    // regions of the bottom glyph without worrying about overdraw (hairy pixels) and simplifies later clipping.
    LayoutRect clipBounds = info.rect;
    clipBounds.shiftXEdgeTo(from.x());
    clipBounds.shiftMaxXEdgeTo(to.x());
    info.context().clip(clipBounds);

    // Trimming may remove up to two pixels from the left of the extender glyph, so we move it left by two pixels.
    float offsetToGlyphLeft = -2;
    LayoutPoint glyphOrigin = LayoutPoint(from.x() + offsetToGlyphLeft, from.y());
    FloatRect lastPaintedGlyphRect(from, FloatSize());

    // In practice, only small stretch sizes are requested but we limit the number of glyphs to avoid hangs.
    for (unsigned extensionCount = 0; lastPaintedGlyphRect.maxX() < to.x() && extensionCount < kMaximumExtensionCount; extensionCount++) {
        lastPaintedGlyphRect = paintGlyph(style, info, m_assembly.extension, glyphOrigin, TrimLeftAndRight);
        glyphOrigin.setX(glyphOrigin.x() + lastPaintedGlyphRect.width());

        // There's a chance that if the font size is small enough the glue glyph has been reduced to an empty rectangle
        // with trimming. In that case we just draw nothing.
        if (lastPaintedGlyphRect.isEmpty())
            break;
    }
}

void MathOperator::paintVerticalGlyphAssembly(const RenderStyle& style, PaintInfo& info, const LayoutPoint& paintOffset)
{
    ASSERT(m_operatorType == Type::VerticalOperator);
    ASSERT(m_stretchType == StretchType::GlyphAssembly);
    ASSERT(m_assembly.topOrRight.isValid());
    ASSERT(m_assembly.bottomOrLeft.isValid());

    // We are positioning the glyphs so that the edge of the tight glyph bounds line up exactly with the edges of our paint box.
    LayoutPoint operatorTopLeft = paintOffset;
    FloatRect topGlyphBounds = boundsForGlyph(m_assembly.topOrRight);
    LayoutPoint topGlyphOrigin(operatorTopLeft.x(), operatorTopLeft.y() - topGlyphBounds.y());
    LayoutRect topGlyphPaintRect = paintGlyph(style, info, m_assembly.topOrRight, topGlyphOrigin, TrimBottom);

    FloatRect bottomGlyphBounds = boundsForGlyph(m_assembly.bottomOrLeft);
    LayoutPoint bottomGlyphOrigin(operatorTopLeft.x(), operatorTopLeft.y() + stretchSize() - (bottomGlyphBounds.height() + bottomGlyphBounds.y()));
    LayoutRect bottomGlyphPaintRect = paintGlyph(style, info, m_assembly.bottomOrLeft, bottomGlyphOrigin, TrimTop);

    if (m_assembly.middle.isValid()) {
        // Center the glyph origin between the start and end glyph paint extents. Then shift it half the paint height toward the bottom glyph.
        FloatRect middleGlyphBounds = boundsForGlyph(m_assembly.middle);
        LayoutPoint middleGlyphOrigin(operatorTopLeft.x(), topGlyphOrigin.y());
        middleGlyphOrigin.moveBy(LayoutPoint(0, (bottomGlyphPaintRect.y() - topGlyphPaintRect.maxY()) / 2.0));
        middleGlyphOrigin.moveBy(LayoutPoint(0, middleGlyphBounds.height() / 2.0));

        LayoutRect middleGlyphPaintRect = paintGlyph(style, info, m_assembly.middle, middleGlyphOrigin, TrimTopAndBottom);
        fillWithVerticalExtensionGlyph(style, info, topGlyphPaintRect.minXMaxYCorner(), middleGlyphPaintRect.minXMinYCorner());
        fillWithVerticalExtensionGlyph(style, info, middleGlyphPaintRect.minXMaxYCorner(), bottomGlyphPaintRect.minXMinYCorner());
    } else
        fillWithVerticalExtensionGlyph(style, info, topGlyphPaintRect.minXMaxYCorner(), bottomGlyphPaintRect.minXMinYCorner());
}

void MathOperator::paintHorizontalGlyphAssembly(const RenderStyle& style, PaintInfo& info, const LayoutPoint& paintOffset)
{
    ASSERT(m_operatorType == Type::HorizontalOperator);
    ASSERT(m_stretchType == StretchType::GlyphAssembly);
    ASSERT(m_assembly.bottomOrLeft.isValid());
    ASSERT(m_assembly.topOrRight.isValid());

    // We are positioning the glyphs so that the edge of the tight glyph bounds line up exactly with the edges of our paint box.
    LayoutPoint operatorTopLeft = paintOffset;
    LayoutUnit baselineY = operatorTopLeft.y() + m_ascent;
    LayoutPoint leftGlyphOrigin(operatorTopLeft.x(), baselineY);
    LayoutRect leftGlyphPaintRect = paintGlyph(style, info, m_assembly.bottomOrLeft, leftGlyphOrigin, TrimRight);

    FloatRect rightGlyphBounds = boundsForGlyph(m_assembly.topOrRight);
    LayoutPoint rightGlyphOrigin(operatorTopLeft.x() + stretchSize() - rightGlyphBounds.width(), baselineY);
    LayoutRect rightGlyphPaintRect = paintGlyph(style, info, m_assembly.topOrRight, rightGlyphOrigin, TrimLeft);

    if (m_assembly.middle.isValid()) {
        // Center the glyph origin between the start and end glyph paint extents.
        LayoutPoint middleGlyphOrigin(operatorTopLeft.x(), baselineY);
        middleGlyphOrigin.moveBy(LayoutPoint((rightGlyphPaintRect.x() - leftGlyphPaintRect.maxX()) / 2.0, 0));
        LayoutRect middleGlyphPaintRect = paintGlyph(style, info, m_assembly.middle, middleGlyphOrigin, TrimLeftAndRight);
        fillWithHorizontalExtensionGlyph(style, info, LayoutPoint(leftGlyphPaintRect.maxX(), baselineY), LayoutPoint(middleGlyphPaintRect.x(), baselineY));
        fillWithHorizontalExtensionGlyph(style, info, LayoutPoint(middleGlyphPaintRect.maxX(), baselineY), LayoutPoint(rightGlyphPaintRect.x(), baselineY));
    } else
        fillWithHorizontalExtensionGlyph(style, info, LayoutPoint(leftGlyphPaintRect.maxX(), baselineY), LayoutPoint(rightGlyphPaintRect.x(), baselineY));
}

void MathOperator::paint(const RenderStyle& style, PaintInfo& info, const LayoutPoint& paintOffset)
{
    if (info.context().paintingDisabled() || info.phase != PaintPhaseForeground || style.visibility() != VISIBLE)
        return;

    GraphicsContextStateSaver stateSaver(info.context());
    info.context().setFillColor(style.visitedDependentColor(CSSPropertyColor));

    // For a radical character, we may need some scale transform to stretch it vertically or mirror it.
    if (m_baseCharacter == kRadicalOperator) {
        float radicalHorizontalScale = style.isLeftToRightDirection() ? 1 : -1;
        if (radicalHorizontalScale == -1 || m_radicalVerticalScale > 1) {
            LayoutPoint scaleOrigin = paintOffset;
            scaleOrigin.move(m_width / 2, 0);
            info.applyTransform(AffineTransform().translate(scaleOrigin).scale(radicalHorizontalScale, m_radicalVerticalScale).translate(-scaleOrigin));
        }
    }

    if (m_stretchType == StretchType::GlyphAssembly) {
        if (m_operatorType == Type::VerticalOperator)
            paintVerticalGlyphAssembly(style, info, paintOffset);
        else
            paintHorizontalGlyphAssembly(style, info, paintOffset);
        return;
    }

    GlyphData glyphData;
    ASSERT(m_stretchType == StretchType::Unstretched || m_stretchType == StretchType::SizeVariant);
    if (m_stretchType == StretchType::Unstretched) {
        if (!getBaseGlyph(style, glyphData))
            return;
    } else if (m_stretchType == StretchType::SizeVariant) {
        ASSERT(m_variant.isValid());
        glyphData = m_variant;
    }
    GlyphBuffer buffer;
    buffer.add(glyphData.glyph, glyphData.font, advanceWidthForGlyph(glyphData));
    LayoutPoint operatorTopLeft = paintOffset;
    FloatRect glyphBounds = boundsForGlyph(glyphData);
    LayoutPoint operatorOrigin(operatorTopLeft.x(), operatorTopLeft.y() - glyphBounds.y());
    info.context().drawGlyphs(style.fontCascade(), *glyphData.font, buffer, 0, 1, operatorOrigin);
}

}

#endif
