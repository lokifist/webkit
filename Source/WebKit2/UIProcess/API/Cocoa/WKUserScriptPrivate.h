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

#import <WebKit/WKUserScript.h>

#if WK_API_ENABLED

NS_ASSUME_NONNULL_BEGIN

@class _WKUserContentWorld;

@interface WKUserScript (WKPrivate)

- (instancetype)_initWithSource:(NSString *)source injectionTime:(WKUserScriptInjectionTime)injectionTime forMainFrameOnly:(BOOL)forMainFrameOnly legacyWhitelist:(WK_ARRAY(NSString *) *)legacyWhitelist legacyBlacklist:(WK_ARRAY(NSString *) *)legacyBlacklist userContentWorld:(_WKUserContentWorld *)userContentWorld WK_AVAILABLE(WK_MAC_TBA, WK_IOS_TBA);
- (instancetype)_initWithSource:(NSString *)source injectionTime:(WKUserScriptInjectionTime)injectionTime forMainFrameOnly:(BOOL)forMainFrameOnly legacyWhitelist:(WK_ARRAY(NSString *) *)legacyWhitelist legacyBlacklist:(WK_ARRAY(NSString *) *)legacyBlacklist associatedURL:(NSURL *)associatedURL userContentWorld:(_WKUserContentWorld *)userContentWorld WK_AVAILABLE(WK_MAC_TBA, WK_IOS_TBA);

@property (nonatomic, readonly) _WKUserContentWorld *_userContentWorld WK_AVAILABLE(WK_MAC_TBA, WK_IOS_TBA);

@end

NS_ASSUME_NONNULL_END

#endif
