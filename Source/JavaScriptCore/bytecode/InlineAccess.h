/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#ifndef InlineAccess_h
#define InlineAccess_h

#if ENABLE(JIT)

#include "CodeLocation.h"
#include "PropertyOffset.h"

namespace JSC {

class JSArray;
class Structure;
class StructureStubInfo;
class VM;

class InlineAccess {
public:

    static constexpr size_t sizeForPropertyAccess()
    {
#if CPU(X86_64)
        return 23;
#elif CPU(X86)
        return 27;
#elif CPU(ARM64)
        return 40;
#elif CPU(ARM)
#if CPU(ARM_THUMB2)
        return 48;
#else
        return 50;
#endif
#else
#error "unsupported platform"
#endif
    }

    static constexpr size_t sizeForPropertyReplace()
    {
#if CPU(X86_64)
        return 23;
#elif CPU(X86)
        return 27;
#elif CPU(ARM64)
        return 40;
#elif CPU(ARM)
#if CPU(ARM_THUMB2)
        return 48;
#else
        return 50;
#endif
#else
#error "unsupported platform"
#endif
    }

    static constexpr size_t sizeForLengthAccess()
    {
#if CPU(X86_64)
        return 26;
#elif CPU(X86)
        return 27;
#elif CPU(ARM64)
        return 32;
#elif CPU(ARM)
#if CPU(ARM_THUMB2)
        return 30;
#else
        return 50;
#endif
#else
#error "unsupported platform"
#endif
    }

    static bool generateSelfPropertyAccess(VM&, StructureStubInfo&, Structure*, PropertyOffset);
    static bool canGenerateSelfPropertyReplace(StructureStubInfo&, PropertyOffset);
    static bool generateSelfPropertyReplace(VM&, StructureStubInfo&, Structure*, PropertyOffset);
    static bool isCacheableArrayLength(StructureStubInfo&, JSArray*);
    static bool generateArrayLength(VM&, StructureStubInfo&, JSArray*);
    static void rewireStubAsJump(VM&, StructureStubInfo&, CodeLocationLabel);

    // This is helpful when determining the size of an IC on
    // various platforms. When adding a new type of IC, implement
    // its placeholder code here, and log the size. That way we
    // can intelligently choose sizes on various platforms.
    NO_RETURN_DUE_TO_CRASH void dumpCacheSizesAndCrash(VM&);
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // InlineAccess_h
