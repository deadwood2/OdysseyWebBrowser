/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebsitePoliciesData.h"

#include "ArgumentCoders.h"
#include "WebProcess.h"
#include <WebCore/DocumentLoader.h>
#include <WebCore/Frame.h>
#include <WebCore/Page.h>

namespace WebKit {

void WebsitePoliciesData::encode(IPC::Encoder& encoder) const
{
    encoder << contentBlockersEnabled;
    encoder << deviceOrientationEventEnabled;
    encoder << autoplayPolicy;
    encoder << allowedAutoplayQuirks;
    encoder << customHeaderFields;
    encoder << popUpPolicy;
    encoder << websiteDataStoreParameters;
    encoder << customUserAgent;
    encoder << customJavaScriptUserAgentAsSiteSpecificQuirks;
    encoder << customNavigatorPlatform;
}

Optional<WebsitePoliciesData> WebsitePoliciesData::decode(IPC::Decoder& decoder)
{
    Optional<bool> contentBlockersEnabled;
    decoder >> contentBlockersEnabled;
    if (!contentBlockersEnabled)
        return WTF::nullopt;

    Optional<bool> deviceOrientationEventEnabled;
    decoder >> deviceOrientationEventEnabled;
    if (!deviceOrientationEventEnabled)
        return WTF::nullopt;
    
    Optional<WebsiteAutoplayPolicy> autoplayPolicy;
    decoder >> autoplayPolicy;
    if (!autoplayPolicy)
        return WTF::nullopt;
    
    Optional<OptionSet<WebsiteAutoplayQuirk>> allowedAutoplayQuirks;
    decoder >> allowedAutoplayQuirks;
    if (!allowedAutoplayQuirks)
        return WTF::nullopt;
    
    Optional<Vector<WebCore::HTTPHeaderField>> customHeaderFields;
    decoder >> customHeaderFields;
    if (!customHeaderFields)
        return WTF::nullopt;

    Optional<WebsitePopUpPolicy> popUpPolicy;
    decoder >> popUpPolicy;
    if (!popUpPolicy)
        return WTF::nullopt;

    Optional<Optional<WebsiteDataStoreParameters>> websiteDataStoreParameters;
    decoder >> websiteDataStoreParameters;
    if (!websiteDataStoreParameters)
        return WTF::nullopt;

    Optional<String> customUserAgent;
    decoder >> customUserAgent;
    if (!customUserAgent)
        return WTF::nullopt;

    Optional<String> customJavaScriptUserAgentAsSiteSpecificQuirks;
    decoder >> customJavaScriptUserAgentAsSiteSpecificQuirks;
    if (!customJavaScriptUserAgentAsSiteSpecificQuirks)
        return WTF::nullopt;

    Optional<String> customNavigatorPlatform;
    decoder >> customNavigatorPlatform;
    if (!customNavigatorPlatform)
        return WTF::nullopt;
    
    return { {
        WTFMove(*contentBlockersEnabled),
        WTFMove(*deviceOrientationEventEnabled),
        WTFMove(*allowedAutoplayQuirks),
        WTFMove(*autoplayPolicy),
        WTFMove(*customHeaderFields),
        WTFMove(*popUpPolicy),
        WTFMove(*websiteDataStoreParameters),
        WTFMove(*customUserAgent),
        WTFMove(*customJavaScriptUserAgentAsSiteSpecificQuirks),
        WTFMove(*customNavigatorPlatform),
    } };
}

void WebsitePoliciesData::applyToDocumentLoader(WebsitePoliciesData&& websitePolicies, WebCore::DocumentLoader& documentLoader)
{
    documentLoader.setCustomHeaderFields(WTFMove(websitePolicies.customHeaderFields));
    documentLoader.setCustomUserAgent(websitePolicies.customUserAgent);
    documentLoader.setCustomJavaScriptUserAgentAsSiteSpecificQuirks(websitePolicies.customJavaScriptUserAgentAsSiteSpecificQuirks);
    documentLoader.setCustomNavigatorPlatform(websitePolicies.customNavigatorPlatform);
    documentLoader.setDeviceOrientationEventEnabled(websitePolicies.deviceOrientationEventEnabled);
    
    // Only setUserContentExtensionsEnabled if it hasn't already been disabled by reloading without content blockers.
    if (documentLoader.userContentExtensionsEnabled())
        documentLoader.setUserContentExtensionsEnabled(websitePolicies.contentBlockersEnabled);

    OptionSet<WebCore::AutoplayQuirk> quirks;
    const auto& allowedQuirks = websitePolicies.allowedAutoplayQuirks;
    
    if (allowedQuirks.contains(WebsiteAutoplayQuirk::InheritedUserGestures))
        quirks.add(WebCore::AutoplayQuirk::InheritedUserGestures);
    
    if (allowedQuirks.contains(WebsiteAutoplayQuirk::SynthesizedPauseEvents))
        quirks.add(WebCore::AutoplayQuirk::SynthesizedPauseEvents);
    
    if (allowedQuirks.contains(WebsiteAutoplayQuirk::ArbitraryUserGestures))
        quirks.add(WebCore::AutoplayQuirk::ArbitraryUserGestures);

    if (allowedQuirks.contains(WebsiteAutoplayQuirk::PerDocumentAutoplayBehavior))
        quirks.add(WebCore::AutoplayQuirk::PerDocumentAutoplayBehavior);

    documentLoader.setAllowedAutoplayQuirks(quirks);

    switch (websitePolicies.autoplayPolicy) {
    case WebsiteAutoplayPolicy::Default:
        documentLoader.setAutoplayPolicy(WebCore::AutoplayPolicy::Default);
        break;
    case WebsiteAutoplayPolicy::Allow:
        documentLoader.setAutoplayPolicy(WebCore::AutoplayPolicy::Allow);
        break;
    case WebsiteAutoplayPolicy::AllowWithoutSound:
        documentLoader.setAutoplayPolicy(WebCore::AutoplayPolicy::AllowWithoutSound);
        break;
    case WebsiteAutoplayPolicy::Deny:
        documentLoader.setAutoplayPolicy(WebCore::AutoplayPolicy::Deny);
        break;
    }

    switch (websitePolicies.popUpPolicy) {
    case WebsitePopUpPolicy::Default:
        documentLoader.setPopUpPolicy(WebCore::PopUpPolicy::Default);
        break;
    case WebsitePopUpPolicy::Allow:
        documentLoader.setPopUpPolicy(WebCore::PopUpPolicy::Allow);
        break;
    case WebsitePopUpPolicy::Block:
        documentLoader.setPopUpPolicy(WebCore::PopUpPolicy::Block);
        break;
    }

    if (websitePolicies.websiteDataStoreParameters) {
        if (auto* frame = documentLoader.frame()) {
            if (auto* page = frame->page())
                page->setSessionID(websitePolicies.websiteDataStoreParameters->networkSessionParameters.sessionID);
        }
    }
}

}

