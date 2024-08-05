/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "WebPreferences.h"

#include "WebPageGroup.h"
#include "WebPreferencesKeys.h"
#include "WebProcessPool.h"
#include <WebCore/LibWebRTCProvider.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ThreadingPrimitives.h>

namespace WebKit {

// FIXME: Manipulating this variable is not thread safe.
// Instead of tracking private browsing state as a boolean preference, we should let the client provide storage sessions explicitly.
static unsigned privateBrowsingPageCount;

Ref<WebPreferences> WebPreferences::create(const String& identifier, const String& keyPrefix, const String& globalDebugKeyPrefix)
{
    return adoptRef(*new WebPreferences(identifier, keyPrefix, globalDebugKeyPrefix));
}

Ref<WebPreferences> WebPreferences::createWithLegacyDefaults(const String& identifier, const String& keyPrefix, const String& globalDebugKeyPrefix)
{
    auto preferences = WebPreferences::create(identifier, keyPrefix, globalDebugKeyPrefix);
    // FIXME: The registerDefault...ValueForKey machinery is unnecessarily heavyweight and complicated.
    // We can just compute different defaults for modern and legacy APIs in WebPreferencesDefinitions.h macros.
    preferences->registerDefaultBoolValueForKey(WebPreferencesKey::javaEnabledKey(), true);
    preferences->registerDefaultBoolValueForKey(WebPreferencesKey::javaEnabledForLocalFilesKey(), true);
    preferences->registerDefaultBoolValueForKey(WebPreferencesKey::pluginsEnabledKey(), true);
    preferences->registerDefaultUInt32ValueForKey(WebPreferencesKey::storageBlockingPolicyKey(), WebCore::SecurityOrigin::AllowAllStorage);
    return preferences;
}

WebPreferences::WebPreferences(const String& identifier, const String& keyPrefix, const String& globalDebugKeyPrefix)
    : m_identifier(identifier)
    , m_keyPrefix(keyPrefix)
    , m_globalDebugKeyPrefix(globalDebugKeyPrefix)
{
    platformInitializeStore();
}

WebPreferences::WebPreferences(const WebPreferences& other)
    : m_keyPrefix(other.m_keyPrefix)
    , m_globalDebugKeyPrefix(other.m_globalDebugKeyPrefix)
    , m_store(other.m_store)
{
    platformInitializeStore();
}

WebPreferences::~WebPreferences()
{
    ASSERT(m_pages.isEmpty());
}

Ref<WebPreferences> WebPreferences::copy() const
{
    return adoptRef(*new WebPreferences(*this));
}

void WebPreferences::addPage(WebPageProxy& webPageProxy)
{
    ASSERT(!m_pages.contains(&webPageProxy));
    m_pages.add(&webPageProxy);

    if (privateBrowsingEnabled()) {
        if (!privateBrowsingPageCount)
            WebProcessPool::willStartUsingPrivateBrowsing();

        ++privateBrowsingPageCount;
    }
}

void WebPreferences::removePage(WebPageProxy& webPageProxy)
{
    ASSERT(m_pages.contains(&webPageProxy));
    m_pages.remove(&webPageProxy);

    if (privateBrowsingEnabled()) {
        --privateBrowsingPageCount;
        if (!privateBrowsingPageCount)
            WebProcessPool::willStopUsingPrivateBrowsing();
    }
}

void WebPreferences::update()
{
    for (auto& webPageProxy : m_pages)
        webPageProxy->preferencesDidChange();
}

void WebPreferences::updateStringValueForKey(const String& key, const String& value)
{
    platformUpdateStringValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateBoolValueForKey(const String& key, bool value)
{
    if (key == WebPreferencesKey::privateBrowsingEnabledKey()) {
        updatePrivateBrowsingValue(value);
        return;
    }

    platformUpdateBoolValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateBoolValueForExperimentalFeatureKey(const String& key, bool value)
{
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateUInt32ValueForKey(const String& key, uint32_t value)
{
    platformUpdateUInt32ValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateDoubleValueForKey(const String& key, double value)
{
    platformUpdateDoubleValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateFloatValueForKey(const String& key, float value)
{
    platformUpdateFloatValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updatePrivateBrowsingValue(bool value)
{
    platformUpdateBoolValueForKey(WebPreferencesKey::privateBrowsingEnabledKey(), value);

    unsigned pagesChanged = m_pages.size();
    if (!pagesChanged)
        return;

    if (value) {
        if (!privateBrowsingPageCount)
            WebProcessPool::willStartUsingPrivateBrowsing();
        privateBrowsingPageCount += pagesChanged;
    }

    update(); // FIXME: Only send over the changed key and value.

    if (!value) {
        ASSERT(privateBrowsingPageCount >= pagesChanged);
        privateBrowsingPageCount -= pagesChanged;
        if (!privateBrowsingPageCount)
            WebProcessPool::willStopUsingPrivateBrowsing();
    }
}

#define DEFINE_PREFERENCE_GETTER_AND_SETTERS(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    void WebPreferences::set##KeyUpper(const Type& value) \
    { \
        if (!m_store.set##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key(), value)) \
            return; \
        update##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key(), value); \
        \
    } \
    \
    Type WebPreferences::KeyLower() const \
    { \
        return m_store.get##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key()); \
    } \

FOR_EACH_WEBKIT_PREFERENCE(DEFINE_PREFERENCE_GETTER_AND_SETTERS)
FOR_EACH_WEBKIT_DEBUG_PREFERENCE(DEFINE_PREFERENCE_GETTER_AND_SETTERS)

#undef DEFINE_PREFERENCE_GETTER_AND_SETTERS

#define DEFINE_EXPERIMENTAL_PREFERENCE_GETTER_AND_SETTERS(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    void WebPreferences::set##KeyUpper(const Type& value) \
    { \
        if (!m_store.set##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key(), value)) \
            return; \
        update##TypeName##ValueForExperimentalFeatureKey(WebPreferencesKey::KeyLower##Key(), value); \
    \
    } \
    \
    Type WebPreferences::KeyLower() const \
    { \
        return m_store.get##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key()); \
    } \

FOR_EACH_WEBKIT_EXPERIMENTAL_FEATURE_PREFERENCE(DEFINE_EXPERIMENTAL_PREFERENCE_GETTER_AND_SETTERS)

#undef DEFINE_EXPERIMENTAL_PREFERENCE_GETTER_AND_SETTERS

static Vector<RefPtr<API::Object>> createExperimentalFeaturesVector()
{
    Vector<RefPtr<API::Object>> features;

#define ADD_EXPERIMENTAL_PREFERENCE_DESCRIPTION(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    features.append(API::ExperimentalFeature::create(HumanReadableName, #KeyUpper, HumanReadableDescription, DefaultValue)); \

    FOR_EACH_WEBKIT_EXPERIMENTAL_FEATURE_PREFERENCE(ADD_EXPERIMENTAL_PREFERENCE_DESCRIPTION)

#undef ADD_EXPERIMENTAL_PREFERENCE_DESCRIPTION

    return features;
}

const Vector<RefPtr<API::Object>>& WebPreferences::experimentalFeatures()
{
    static NeverDestroyed<Vector<RefPtr<API::Object>>> features = createExperimentalFeaturesVector();
    return features;
}

bool WebPreferences::isEnabledForFeature(const API::ExperimentalFeature& feature) const
{
    struct FeatureGetterMapping {
        const char* name;
        bool (WebPreferences::*function) () const;
    };

#define MAKE_FEATURE_GETTER(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    { #KeyUpper, &WebPreferences::KeyLower }, \

    static FeatureGetterMapping getters[] = {
        FOR_EACH_WEBKIT_EXPERIMENTAL_FEATURE_PREFERENCE(MAKE_FEATURE_GETTER)
    };

#undef MAKE_FEATURE_GETTER

    const String& key = feature.key();

    for (auto& getter : getters) {
        if (key == getter.name)
            return (this->*getter.function)();
    }

    return false;
}

void WebPreferences::setEnabledForFeature(bool value, const API::ExperimentalFeature& feature)
{
    struct FeatureSetterMapping {
        const char* name;
        void (WebPreferences::*function) (const bool&);
    };

#define MAKE_FEATURE_SETTER(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    { #KeyUpper, &WebPreferences::set##KeyUpper }, \

    static FeatureSetterMapping setters[] = {
        FOR_EACH_WEBKIT_EXPERIMENTAL_FEATURE_PREFERENCE(MAKE_FEATURE_SETTER)
    };

#undef MAKE_FEATURE_SETTER

    const String& key = feature.key();
    
    for (auto& setter : setters) {
        if (key == setter.name) {
            (this->*setter.function)(value);
            return;
        }
    }
}

void WebPreferences::enableAllExperimentalFeatures()
{
#define SET_FEATURE_ENABLED(KeyUpper, KeyLower, TypeName, Type, DefaultValue, HumanReadableName, HumanReadableDescription) \
    set##KeyUpper(true); \

    FOR_EACH_WEBKIT_EXPERIMENTAL_FEATURE_PREFERENCE(SET_FEATURE_ENABLED)

#undef SET_FEATURE_ENABLED
}

bool WebPreferences::anyPagesAreUsingPrivateBrowsing()
{
    return privateBrowsingPageCount;
}

void WebPreferences::registerDefaultBoolValueForKey(const String& key, bool value)
{
    m_store.setOverrideDefaultsBoolValueForKey(key, value);
    bool userValue;
    if (platformGetBoolUserValueForKey(key, userValue))
        m_store.setBoolValueForKey(key, userValue);
}

void WebPreferences::registerDefaultUInt32ValueForKey(const String& key, uint32_t value)
{
    m_store.setOverrideDefaultsUInt32ValueForKey(key, value);
    uint32_t userValue;
    if (platformGetUInt32UserValueForKey(key, userValue))
        m_store.setUInt32ValueForKey(key, userValue);
}

} // namespace WebKit
