/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2015, 2016 Apple Inc. All rights reserved.
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

#ifndef JSCustomElementInterface_h
#define JSCustomElementInterface_h

#if ENABLE(CUSTOM_ELEMENTS)

#include "ActiveDOMCallback.h"
#include "QualifiedName.h"
#include <heap/Weak.h>
#include <heap/WeakInlines.h>
#include <runtime/JSObject.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace JSC {

class JSObject;
class PrivateName;

}

namespace WebCore {

class DOMWrapperWorld;
class Element;
class JSDOMGlobalObject;
class MathMLElement;
class SVGElement;

class JSCustomElementInterface : public RefCounted<JSCustomElementInterface>, public ActiveDOMCallback {
public:
    static Ref<JSCustomElementInterface> create(const QualifiedName& name, JSC::JSObject* callback, JSDOMGlobalObject* globalObject)
    {
        return adoptRef(*new JSCustomElementInterface(name, callback, globalObject));
    }

    enum class ShouldClearException { Clear, DoNotClear };
    RefPtr<Element> constructElement(const AtomicString&, ShouldClearException);

    void upgradeElement(Element&);

    void attributeChanged(Element&, const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);

    ScriptExecutionContext* scriptExecutionContext() const { return ContextDestructionObserver::scriptExecutionContext(); }
    JSC::JSObject* constructor() { return m_constructor.get(); }

    const QualifiedName& name() const { return m_name; }

    bool isUpgradingElement() const { return !m_constructionStack.isEmpty(); }
    Element* lastElementInConstructionStack() const { return m_constructionStack.last().get(); }
    void didUpgradeLastElementInConstructionStack();

    virtual ~JSCustomElementInterface();

private:
    JSCustomElementInterface(const QualifiedName&, JSC::JSObject* callback, JSDOMGlobalObject*);

    QualifiedName m_name;
    mutable JSC::Weak<JSC::JSObject> m_constructor;
    RefPtr<DOMWrapperWorld> m_isolatedWorld;
    Vector<RefPtr<Element>, 1> m_constructionStack;
};

} // namespace WebCore

#endif

#endif
