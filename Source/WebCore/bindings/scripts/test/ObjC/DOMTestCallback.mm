/*
 * This file is part of the WebKit open source project.
 * This file has been generated by generate-bindings.pl. DO NOT MODIFY!
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

#import "config.h"

#if ENABLE(SPEECH_SYNTHESIS)

#import "DOMInternal.h"

#import "DOMTestCallback.h"

#import "Class5.h"
#import "Class6.h"
#import "DOMClass5Internal.h"
#import "DOMClass6Internal.h"
#import "DOMDOMStringListInternal.h"
#import "DOMFloat32ArrayInternal.h"
#import "DOMNodeInternal.h"
#import "DOMStringList.h"
#import "DOMTestCallbackInternal.h"
#import "DOMTestNodeInternal.h"
#import "ExceptionHandlers.h"
#import "JSMainThreadExecState.h"
#import "SerializedScriptValue.h"
#import "TestCallback.h"
#import "TestNode.h"
#import "ThreadCheck.h"
#import "URL.h"
#import "WebCoreObjCExtras.h"
#import "WebScriptObjectPrivate.h"
#import <wtf/GetPtr.h>

#define IMPL reinterpret_cast<WebCore::TestCallback*>(_internal)

@implementation DOMTestCallback

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([DOMTestCallback class], self))
        return;

    if (_internal)
        IMPL->deref();
    [super dealloc];
}

- (BOOL)callbackWithNoParam
{
    WebCore::JSMainThreadNullState state;
    return IMPL->callbackWithNoParam();
}

- (BOOL)callbackWithArrayParam:(DOMFloat32Array *)arrayParam
{
    WebCore::JSMainThreadNullState state;
    if (!arrayParam)
        WebCore::raiseTypeErrorException();
    return IMPL->callbackWithArrayParam(*core(arrayParam));
}

- (BOOL)callbackWithSerializedScriptValueParam:(NSString *)srzParam strArg:(NSString *)strArg
{
    WebCore::JSMainThreadNullState state;
    return IMPL->callbackWithSerializedScriptValueParam(WebCore::SerializedScriptValue::create(WTF::String(srzParam)), strArg);
}

- (int)callbackWithNonBoolReturnType:(NSString *)strArg
{
    WebCore::JSMainThreadNullState state;
    return IMPL->callbackWithNonBoolReturnType(strArg);
}

- (int)customCallback:(DOMClass5 *)class5Param class6Param:(DOMClass6 *)class6Param
{
    WebCore::JSMainThreadNullState state;
    if (!class5Param)
        WebCore::raiseTypeErrorException();
    if (!class6Param)
        WebCore::raiseTypeErrorException();
    return IMPL->customCallback(*core(class5Param), *core(class6Param));
}

- (BOOL)callbackWithStringList:(DOMDOMStringList *)listParam
{
    WebCore::JSMainThreadNullState state;
    if (!listParam)
        WebCore::raiseTypeErrorException();
    return IMPL->callbackWithStringList(*core(listParam));
}

- (BOOL)callbackWithBoolean:(BOOL)boolParam
{
    WebCore::JSMainThreadNullState state;
    return IMPL->callbackWithBoolean(boolParam);
}

- (BOOL)callbackRequiresThisToPass:(int)longParam testNodeParam:(DOMTestNode *)testNodeParam
{
    WebCore::JSMainThreadNullState state;
    if (!testNodeParam)
        WebCore::raiseTypeErrorException();
    return IMPL->callbackRequiresThisToPass(longParam, *core(testNodeParam));
}

@end

WebCore::TestCallback* core(DOMTestCallback *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::TestCallback*>(wrapper->_internal) : 0;
}

DOMTestCallback *kit(WebCore::TestCallback* value)
{
    WebCoreThreadViolationCheckRoundOne();
    if (!value)
        return nil;
    if (DOMTestCallback *wrapper = getDOMWrapper(value))
        return [[wrapper retain] autorelease];
    DOMTestCallback *wrapper = [[DOMTestCallback alloc] _init];
    wrapper->_internal = reinterpret_cast<DOMObjectInternal*>(value);
    value->ref();
    addDOMWrapper(wrapper, value);
    return [wrapper autorelease];
}

#endif // ENABLE(SPEECH_SYNTHESIS)
