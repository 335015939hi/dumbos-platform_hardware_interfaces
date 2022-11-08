/*
 * Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <tinyxml2.h>

#include "effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

/** Default path of effect configuration file. Relative to DEFAULT_LOCATIONS. */
static const std::string DEFAULT_EFFECT_CONFIG_FILE = "audio_effects.xml";

/** Directories where the effect libraries will be search for. */
static const std::array<std::string, 3> LD_EFFECT_LIBRARY_PATH =
#ifdef __LP64__
        {"/odm/lib64/soundfx", "/vendor/lib64/soundfx", "/system/lib64/soundfx"};
#else
        {"/odm/lib/soundfx", "/vendor/lib/soundfx", "/system/lib/soundfx"};
#endif

static inline bool stringToUuid(const char* str,
                                ::aidl::android::media::audio::common::AudioUuid* uuid) {
    RETURN_VALUE_IF(!uuid || !str, false, "nullPtr");

    uint32_t tmp[10];
    if (sscanf(str, "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x", tmp, tmp + 1, tmp + 2, tmp + 3,
               tmp + 4, tmp + 5, tmp + 6, tmp + 7, tmp + 8, tmp + 9) < 10) {
        return false;
    }
    uuid->timeLow = (uint32_t)tmp[0];
    uuid->timeMid = (uint16_t)tmp[1];
    uuid->timeHiAndVersion = (uint16_t)tmp[2];
    uuid->clockSeq = (uint16_t)tmp[3];
    uuid->node[0] = (uint8_t)tmp[4];
    uuid->node[1] = (uint8_t)tmp[5];
    uuid->node[2] = (uint8_t)tmp[6];
    uuid->node[3] = (uint8_t)tmp[7];
    uuid->node[4] = (uint8_t)tmp[8];
    uuid->node[5] = (uint8_t)tmp[9];

    return true;
}

static inline bool resolveLibraryPath(const std::string& file, std::string* resolvedPath) {
    for (const auto& libraryDirectory : LD_EFFECT_LIBRARY_PATH) {
        std::string candidatePath = std::string(libraryDirectory) + '/' + file;
        if (access(candidatePath.c_str(), R_OK) == 0) {
            *resolvedPath = std::move(candidatePath);
            return true;
        }
    }
    return false;
}

/**
 *  Library contains a mapping from library name to path.
 *  Effect contains a mapping from effect name to Libraries and implementation UUID.
 *  Pre/post processor contains a mapping from processing name to effect names.
 */
class EffectConfig {
  public:
    explicit EffectConfig(const std::string& path = "") {
        tinyxml2::XMLDocument doc;
        doc.LoadFile(path.c_str());
        LOG(DEBUG) << __func__ << " " << path;
        // parse the xml file into maps
        if (doc.Error()) {
            LOG(ERROR) << __func__ << " Tinyxml2 failed to parse " << path << " error "
                       << doc.ErrorStr();
            return;
        }

        auto registerFailure = [&](bool result) { mSkippedElements += result ? 0 : 1; };
        for (auto& xmlConfig : getChildren(doc, "audio_effects_conf")) {
            // Parse library
            for (auto& xmlLibraries : getChildren(xmlConfig, "libraries")) {
                for (auto& xmlLibrary : getChildren(xmlLibraries, "library")) {
                    registerFailure(parseLibrary(xmlLibrary));
                }
            }

            // Parse effects
            for (auto& xmlEffects : getChildren(xmlConfig, "effects")) {
                for (auto& xmlEffect : getChildren(xmlEffects)) {
                    registerFailure(parseEffect(xmlEffect));
                }
            }

            // Parse pre processing chains
            for (auto& xmlPreprocess : getChildren(xmlConfig, "preprocess")) {
                for (auto& xmlStream : getChildren(xmlPreprocess, "stream")) {
                    registerFailure(parseStream(xmlStream));
                }
            }

            // Parse post processing chains
            for (auto& xmlPostprocess : getChildren(xmlConfig, "postprocess")) {
                for (auto& xmlStream : getChildren(xmlPostprocess, "stream")) {
                    registerFailure(parseStream(xmlStream));
                }
            }
        }
    }

    // <library>
    struct Library {
        std::string name;
        std::string path;
    };
    struct LibraryUuid {
        std::string name;  // library name
        ::aidl::android::media::audio::common::AudioUuid implUuid;
    };
    // <effects>
    struct EffectLibraries {
        std::optional<struct LibraryUuid> proxyLibrary;
        std::vector<struct LibraryUuid> libraries;
    };

    int getSkippedElements() const { return mSkippedElements; }
    const std::unordered_map<std::string, std::string> getLibraryMap() const { return mLibraryMap; }
    const std::unordered_map<std::string, struct EffectLibraries> getEffectsMap() const {
        return mEffectsMap;
    }
    const std::unordered_map<std::string, std::vector<std::string>> getProcessingMap() const {
        return mProcessingMap;
    }

  private:
    int mSkippedElements;
    /* Parsed Libraries result */
    std::unordered_map<std::string, std::string> mLibraryMap;
    /* Parsed Effects result */
    std::unordered_map<std::string, struct EffectLibraries> mEffectsMap;
    /* Parsed pre/post processing result */
    std::unordered_map<std::string, std::vector<std::string>> mProcessingMap;

    /** @return all `node`s children that are elements and match the tag if provided. */
    std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> getChildren(
            const tinyxml2::XMLNode& node, const char* childTag = nullptr) {
        std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> children;
        for (auto* child = node.FirstChildElement(childTag); child != nullptr;
             child = child->NextSiblingElement(childTag)) {
            children.emplace_back(*child);
        }
        return children;
    }

    /** Parse a library xml note and push the result in mLibraryMap or return false on failure. */
    bool parseLibrary(const tinyxml2::XMLElement& xmlLibrary) {
        const char* name = xmlLibrary.Attribute("name");
        RETURN_VALUE_IF(name == nullptr, false, "noNameAttribute");
        const char* path = xmlLibrary.Attribute("path");
        RETURN_VALUE_IF(path == nullptr, false, "noPathAttribute");

        mLibraryMap[name] = path;
        return true;
    }

    /** Parse an effect from an xml element describing it.
     * @return true and pushes the effect in mEffectsMap on success, false on failure.
     */
    bool parseEffect(const tinyxml2::XMLElement& xml) {
        struct EffectLibraries effectLibraries;
        std::vector<LibraryUuid> libraryUuids;
        std::string name = xml.Attribute("name");
        RETURN_VALUE_IF(name == "", false, "effectsNoName");

        struct LibraryUuid libraryUuid;
        if (std::strcmp(xml.Name(), "effectProxy") == 0) {
            // proxy lib and uuid
            RETURN_VALUE_IF(!parseLibraryUuid(xml, libraryUuid), false, "parseProxyLibFailed");
            effectLibraries.proxyLibrary = libraryUuid;
            // proxy effect libs and UUID
            auto xmlProxyLib = xml.FirstChildElement();
            RETURN_VALUE_IF(xmlProxyLib == nullptr, false, "noLibForProxy");
            while (xmlProxyLib) {
                RETURN_VALUE_IF(!parseLibraryUuid(*xmlProxyLib, libraryUuid), false,
                                "parseEffectLibFailed");
                libraryUuids.emplace_back(libraryUuid);
                xmlProxyLib = xmlProxyLib->NextSiblingElement();
            }
        } else {
            // expect only one library if not proxy
            RETURN_VALUE_IF(!parseLibraryUuid(xml, libraryUuid), false, "parseEffectLibFailed");
            libraryUuids.emplace_back(std::move(libraryUuid));
        }

        effectLibraries.libraries = std::move(libraryUuids);
        mEffectsMap[name] = std::move(effectLibraries);
        return true;
    }

    bool parseStream(const tinyxml2::XMLElement& xml) {
        const char* type = xml.Attribute("type");
        RETURN_VALUE_IF(!type, false, "noTypeInProcess");
        RETURN_VALUE_IF(0 != mProcessingMap.count(type), false, "duplicateType");

        for (auto& apply : getChildren(xml, "apply")) {
            const char* name = apply.get().Attribute("effect");
            RETURN_VALUE_IF(!name, false, "noEffectAttribute");
            mProcessingMap[type].push_back(name);
        }
        return true;
    }

    // Function to parse effect.library name and effect.uuid from xml
    bool parseLibraryUuid(const tinyxml2::XMLElement& xml, struct LibraryUuid& libraryUuid) {
        // Retrieve library name and uuid from xml
        libraryUuid.name = xml.Attribute("library");
        RETURN_VALUE_IF(libraryUuid.name == "", false, "noLibraryAttribute");

        const char* uuid = xml.Attribute("uuid");
        RETURN_VALUE_IF(!uuid, false, "noUuidAttribute");
        RETURN_VALUE_IF(!stringToUuid(uuid, &libraryUuid.implUuid), false, "invalidUuidAttribute");
        return true;
    }

    const char* dump(const tinyxml2::XMLElement& element, tinyxml2::XMLPrinter&& printer = {}) {
        element.Accept(&printer);
        return printer.CStr();
    }
};

}  // namespace aidl::android::hardware::audio::effect
