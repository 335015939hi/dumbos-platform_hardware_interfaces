/*
* Copyright (C) 2024 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>

#define LOG_TAG "AHAL_Config"
#include <aidl/android/media/audio/common/AudioFlag.h>
#include <aidl/android/media/audio/common/AudioProductStrategyType.h>
#include <android-base/logging.h>

#include "core-impl/CapEngineConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioHalCapConfiguration;
using aidl::android::media::audio::common::AudioHalCapDomain;
using aidl::android::media::audio::common::AudioHalCapDomains;
using aidl::android::media::audio::common::AudioHalCapSetting;

using ::android::BAD_VALUE;
using ::android::base::unexpected;

namespace eng_xsd = android::audio::policy::engine::configuration;

namespace aidl::android::hardware::audio::core::internal {

static constexpr const char *gSystemClassNameAttribute = "SystemClassName";

template<typename E, typename C>
struct BaseSerializerTraits {
    typedef E Element;
    typedef C Collection;
    typedef void* PtrSerializingCtx;
};

struct CapConfigurableDomainTraits : public BaseSerializerTraits<AudioHalCapDomain,
        std::vector<AudioHalCapDomain>> {
    static constexpr const char *tag = "ConfigurableDomain";
    static constexpr const char *collectionTag = "ConfigurableDomains";

    struct Attributes {
        static constexpr const char *sequenceAware = "SequenceAware";
        static constexpr const char *name = "Name";
    };
    static ::android::status_t deserialize(_xmlDoc *doc, const _xmlNode *root, Collection &ps);
};

struct ConfigurationTraits : public BaseSerializerTraits<AudioHalCapConfiguration,
        std::vector<AudioHalCapConfiguration>> {
    static constexpr const char *tag = "Configuration";
    static constexpr const char *collectionTag = "Configurations";

    struct Attributes {
        static constexpr const char *name = "Name";
    };
    static ::android::status_t deserialize(_xmlDoc *doc, const _xmlNode *root, Collection &ps);
};

struct ConfigurableElementTraits :
        public BaseSerializerTraits<std::string, std::vector<std::string>> {
    static constexpr const char *tag = "ConfigurableElement";
    static constexpr const char *collectionTag = "ConfigurableElements";

    struct Attributes {
        static constexpr const char *path = "Path";
    };
    static ::android::status_t deserialize(_xmlDoc *doc, const _xmlNode *root, Collection &ps);
};

struct CapSettingTraits :
        public BaseSerializerTraits<AudioHalCapSetting, std::vector<AudioHalCapSetting>> {
    static constexpr const char *tag = "Configuration";
    static constexpr const char *collectionTag = "Settings";
    struct Attributes {
        static constexpr const char *name = "Name";
    };
    static ::android::status_t deserialize(_xmlDoc *doc, const _xmlNode *root, Collection &ps);
};

template <class T>
constexpr void (*xmlDeleter)(T* t);
template <>
constexpr auto xmlDeleter<xmlDoc> = xmlFreeDoc;
template <>
constexpr auto xmlDeleter<xmlChar> = [](xmlChar *s) { xmlFree(s); };

/** @return a unique_ptr with the correct deleter for the libxml2 object. */
template <class T>
constexpr auto make_xmlUnique(T *t) {
    // Wrap deleter in lambda to enable empty base optimization
    auto deleter = [](T *t) { xmlDeleter<T>(t); };
    return std::unique_ptr<T, decltype(deleter)>{t, deleter};
}

std::string getXmlAttribute(const xmlNode *cur, const char *attribute)
{
    auto charPtr = make_xmlUnique(xmlGetProp(cur, reinterpret_cast<const xmlChar *>(attribute)));
    if (charPtr == NULL) {
        return "";
    }
    std::string value(reinterpret_cast<const char*>(charPtr.get()));
    return value;
}

template <class Trait>
static ::android::status_t deserializeCollection(_xmlDoc *doc, const _xmlNode *cur,
                                      typename Trait::Collection &collection,
                                      size_t &nbSkippedElement)
{
    for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
        if (xmlStrcmp(cur->name, (const xmlChar *)Trait::collectionTag) &&
            xmlStrcmp(cur->name, (const xmlChar *)Trait::tag)) {
            continue;
        }
        const xmlNode *child = cur;
        if (!xmlStrcmp(child->name, (const xmlChar *)Trait::collectionTag)) {
            child = child->xmlChildrenNode;
        }
        for (; child != NULL; child = child->next) {
            if (!xmlStrcmp(child->name, (const xmlChar *)Trait::tag)) {
                ::android::status_t status = Trait::deserialize(doc, child, collection);
                if (status != ::android::NO_ERROR) {
                    nbSkippedElement += 1;
                }
            }
        }
        if (!xmlStrcmp(cur->name, (const xmlChar *)Trait::tag)) {
            return ::android::NO_ERROR;
        }
    }
    return ::android::NO_ERROR;
}

static constexpr const char *compoundRuleTag = "CompoundRule";
static constexpr const char *selectionCriterionRuleTag = "SelectionCriterionRule";
static constexpr const char *typeAttribute = "Type";

static constexpr const char *selectionCriterionAttribute = "SelectionCriterion";
static constexpr const char *matchesWhenAttribute = "MatchesWhen";
static constexpr const char *valueAttribute = "Value";

::android::status_t deserializeRule(_xmlDoc *doc, const _xmlNode *cur, std::string &rule)
{
    bool isPreviousCompoundRule = true;
    for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
        if (xmlStrcmp(cur->name, (const xmlChar *) compoundRuleTag) &&
            xmlStrcmp(cur->name, (const xmlChar *) selectionCriterionRuleTag)) {
            continue;
        }
        const xmlNode *child = cur;
        if (!xmlStrcmp(child->name, (const xmlChar *) compoundRuleTag)) {
            std::string type = getXmlAttribute(child, typeAttribute);
            if (type.empty()) {
                ALOGE("%s No attribute %s found", __func__, typeAttribute);
                return BAD_VALUE;
            }
            rule += (isPreviousCompoundRule? "" : " , " ) + type + "{";
            deserializeRule(doc, child, rule);
            rule += "}";
        }
        if (!xmlStrcmp(child->name, (const xmlChar *) selectionCriterionRuleTag)) {
            if (!isPreviousCompoundRule) {
                rule += " , ";
            }
            isPreviousCompoundRule = false;
            std::string selectionCriterion = getXmlAttribute(child, selectionCriterionAttribute);
            if (selectionCriterion.empty()) {
                ALOGE("%s No attribute %s found", __func__, selectionCriterionAttribute);
                return BAD_VALUE;
            }
            std::string matchesWhen = getXmlAttribute(child, matchesWhenAttribute);
            if (matchesWhen.empty()) {
                ALOGE("%s No attribute %s found", __func__, matchesWhenAttribute);
                return BAD_VALUE;
            }
            std::string value = getXmlAttribute(child, valueAttribute);
            if (value.empty()) {
                ALOGE("%s No attribute %s found", __func__, valueAttribute);
                return BAD_VALUE;
            }
            rule += " " + selectionCriterion + " " + matchesWhen + " " + value + " ";
        }
    }
    return ::android::NO_ERROR;
}

::android::status_t ConfigurationTraits::deserialize(_xmlDoc *doc, const _xmlNode *child,
        Collection &configurations)
{
    std::string name = getXmlAttribute(child, Attributes::name);
    if (name.empty()) {
        ALOGE("%s No attribute %s found", __func__, Attributes::name);
        return BAD_VALUE;
    }
    std::string rule;
    deserializeRule(doc, child, rule);

    configurations.push_back({name, rule});
    return ::android::NO_ERROR;
}

static constexpr const char *configurableElementTag = "ConfigurableElement";
static constexpr const char *configurableElementPathAttribute = "Path";
static constexpr const char *stringParameterTag = "StringParameter";
static constexpr const char *enumParameterTag = "EnumParameter";
static constexpr const char *bitParameterTag = "BitParameter";
static constexpr const char *fixedPointParameterTag = "FixedPointParameter";
static constexpr const char *booleanParameterTag = "BooleanParameter";
static constexpr const char *integerParameterTag = "IntegerParameter";
static constexpr const char *floatingPointParameterTag = "FloatingPointParameter";

::android::status_t CapSettingTraits::deserialize(_xmlDoc *doc __unused, const _xmlNode *cur,
        Collection &settings)
{
    std::string configurationName = getXmlAttribute(cur, Attributes::name);
    if (configurationName.empty()) {
        ALOGE("%s No attribute %s found", __func__, Attributes::name);
        return BAD_VALUE;
    }
    std::vector<AudioHalCapSetting::ParameterSetting> configurableElementValues;
    for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
        if (xmlStrcmp(cur->name, (const xmlChar *) configurableElementTag)) {
            continue;
        }
        std::string name = getXmlAttribute(cur, configurableElementPathAttribute);
        if (name.empty()) {
            ALOGE("%s No attribute %s found", __func__, configurableElementPathAttribute);
            return BAD_VALUE;
        }
        const xmlNode *child = cur->xmlChildrenNode;
        for (; child != NULL; child = child->next) {
            if (!xmlStrcmp(child->name, (const xmlChar *)stringParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)bitParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)fixedPointParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)booleanParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)integerParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)floatingPointParameterTag) ||
                !xmlStrcmp(child->name, (const xmlChar *)enumParameterTag)) {
                auto valXml = make_xmlUnique(xmlNodeListGetString(doc, child->xmlChildrenNode, 1));
                if (valXml == NULL) {
                    return BAD_VALUE;
                }
                std::string value = reinterpret_cast<const char*>(valXml.get());
                configurableElementValues.push_back({name, value});
                break;
            }
        }
    }
    settings.push_back({configurationName, configurableElementValues});
    return ::android::NO_ERROR;
}

::android::status_t ConfigurableElementTraits::deserialize(_xmlDoc *doc __unused,
        const _xmlNode *child, Collection &configurableElements)
{
    std::string path = getXmlAttribute(child, Attributes::path);
    if (path.empty()) {
        ALOGE("%s No attribute %s found", __func__, Attributes::path);
        return BAD_VALUE;
    }
    configurableElements.push_back(path);
    return ::android::NO_ERROR;
}

::android::status_t CapConfigurableDomainTraits::deserialize(_xmlDoc *doc, const _xmlNode *child,
        Collection &domains)
{
    std::string name = getXmlAttribute(child, Attributes::name);
    if (name.empty()) {
        ALOGE("%s No attribute %s found", __func__, Attributes::name);
        return BAD_VALUE;
    }
    bool sequenceAware = false;
    std::string sequenceAwareLiteral = getXmlAttribute(child, Attributes::sequenceAware);
    if (!sequenceAwareLiteral.empty()) {
        sequenceAware = (sequenceAwareLiteral == "true");
    }
    if (sequenceAware) {
        ALOGE("%s sequence aware not supported", __func__);
        return BAD_VALUE;
    }
    size_t skipped = 0;
    std::vector<AudioHalCapConfiguration> configurations{};
    deserializeCollection<ConfigurationTraits>(doc, child, configurations, skipped);

    std::vector<std::string> configurableElements;
    deserializeCollection<ConfigurableElementTraits>(doc, child, configurableElements, skipped);

    std::vector<AudioHalCapSetting> settings;
    deserializeCollection<CapSettingTraits>(doc, child, settings, skipped);

    domains.push_back({name, configurableElements, configurations, settings});
    return ::android::NO_ERROR;
}


namespace {

class XmlErrorHandler {
public:
    XmlErrorHandler() {
        xmlSetGenericErrorFunc(this, &xmlErrorHandler);
    }
    XmlErrorHandler(const XmlErrorHandler&) = delete;
    XmlErrorHandler(XmlErrorHandler&&) = delete;
    XmlErrorHandler& operator=(const XmlErrorHandler&) = delete;
    XmlErrorHandler& operator=(XmlErrorHandler&&) = delete;
    ~XmlErrorHandler() {
        xmlSetGenericErrorFunc(NULL, NULL);
        if (!mErrorMessage.empty()) {
            ALOG(LOG_ERROR, "libxml2", "%s", mErrorMessage.c_str());
        }
    }
    static void xmlErrorHandler(void* ctx, const char* msg, ...) {
        char buffer[256];
        va_list args;
        va_start(args, msg);
        vsnprintf(buffer, sizeof(buffer), msg, args);
        va_end(args);
        static_cast<XmlErrorHandler*>(ctx)->mErrorMessage += buffer;
    }
private:
    std::string mErrorMessage;
};

}  // namespace

AudioHalCapDomains CapEngineConfigXmlConverter::getAidlCapEngineConfig() {
    XmlErrorHandler errorHandler;
    auto doc = make_xmlUnique(xmlParseFile(kCapEngineConfigFileName));
    if (doc == NULL) {
        // It is OK not to find an engine config file at the default location
        // as the caller will default to hardcoded default config
        ALOGW("%s: Could not parse document %s", __func__, kCapEngineConfigFileName);
        return {};
    }
    xmlNodePtr cur = xmlDocGetRootElement(doc.get());
    if (cur == NULL) {
        ALOGE("%s: Could not parse: empty document %s", __func__, kCapEngineConfigFileName);
        return {};
    }
    if (xmlXIncludeProcess(doc.get()) < 0) {
        ALOGE("%s: libxml failed to resolve XIncludes on document %s", __func__,
              kCapEngineConfigFileName);
        return {};
    }
    std::string systemClass = getXmlAttribute(cur, gSystemClassNameAttribute);
    if (systemClass.empty() || systemClass != "Policy") {
        ALOGE("%s: No systemClass found", __func__);
        return {};
    }
    size_t nbSkippedElements = 0;
    std::vector<AudioHalCapDomain> capConfigurableDomains {};
    deserializeCollection<CapConfigurableDomainTraits>(
            doc.get(), cur, capConfigurableDomains, nbSkippedElements);
    return {capConfigurableDomains};
}

} // namespace android
