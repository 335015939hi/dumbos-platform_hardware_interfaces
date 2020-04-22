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

#define LOG_TAG "media_omx_hidl_master_test"
#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

#include <android-base/logging.h>

#include <android/hardware/media/omx/1.0/IOmx.h>
#include <android/hardware/media/omx/1.0/IOmxNode.h>
#include <android/hardware/media/omx/1.0/IOmxObserver.h>
#include <android/hardware/media/omx/1.0/IOmxStore.h>
#include <android/hardware/media/omx/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::hardware::media::omx::V1_0::IOmx;
using ::android::hardware::media::omx::V1_0::IOmxObserver;
using ::android::hardware::media::omx::V1_0::IOmxNode;
using ::android::hardware::media::omx::V1_0::IOmxStore;
using ::android::hardware::media::omx::V1_0::Message;
using ::android::hardware::media::omx::V1_0::CodecBuffer;
using ::android::hardware::media::omx::V1_0::PortMode;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::memory::V1_0::IMapper;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

#include <getopt.h>
#include <media_hidl_test_common.h>

class MasterHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        omxStore = IOmxStore::getService(GetParam());
        ASSERT_NE(omxStore, nullptr);
        omx = IOmx::getService(GetParam());
        ASSERT_NE(omx, nullptr);
    }

    sp<IOmxStore> omxStore;
    sp<IOmx> omx;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

void displayComponentInfo(hidl_vec<IOmx::ComponentInfo>& nodeList) {
    for (size_t i = 0; i < nodeList.size(); i++) {
        printf("%s | ", nodeList[i].mName.c_str());
        for (size_t j = 0; j < ((nodeList[i]).mRoles).size(); j++) {
            printf("%s ", nodeList[i].mRoles[j].c_str());
        }
        printf("\n");
    }
}

/*
 * Returns the role based on is_encoder and mime.
 *
 * The mapping from a pair (is_encoder, mime) to a role string is
 * defined in frameworks/av/media/libmedia/MediaDefs.cpp and
 * frameworks/av/media/libstagefright/omx/OMXUtils.cpp. This function
 * does essentially the same work as GetComponentRole() in
 * OMXUtils.cpp.
 *
 * Args:
 *   is_encoder: A boolean indicating whether the role is for an
 *       encoder or a decoder.
 *   mime: A string of the desired mime type.
 *
 * Returns:
 *   A const string for the requested role name, empty if mime is not
 *   recognized.
 */
const std::string getComponentRole(bool is_encoder, const std::string mime) {
    // Mapping from mime types to roles.
    // These values come from MediaDefs.cpp and OMXUtils.cpp
    const std::map<const std::string, const std::string> audio_mime_to_role = {
            {"3gpp", "amrnb"},         {"ac3", "ac3"},     {"amr-wb", "amrwb"},
            {"eac3", "eac3"},          {"flac", "flac"},   {"g711-alaw", "g711alaw"},
            {"g711-mlaw", "g711mlaw"}, {"gsm", "gsm"},     {"mp4a-latm", "aac"},
            {"mpeg", "mp3"},           {"mpeg-L1", "mp1"}, {"mpeg-L2", "mp2"},
            {"opus", "opus"},          {"raw", "raw"},     {"vorbis", "vorbis"},
    };
    const std::map<const std::string, const std::string> video_mime_to_role = {
            {"3gpp", "h263"},         {"avc", "avc"},           {"dolby-vision", "dolby-vision"},
            {"hevc", "hevc"},         {"mp4v-es", "mpeg4"},     {"mpeg2", "mpeg2"},
            {"x-vnd.on2.vp8", "vp8"}, {"x-vnd.on2.vp9", "vp9"},
    };
    const std::map<const std::string, const std::string> image_mime_to_role = {
            {"vnd.android.heic", "heic"},
    };

    const std::string mime_suffix = mime.substr(6, mime.size() - 1);
    const std::string middle = is_encoder ? "encoder." : "decoder.";
    std::string prefix;
    std::string suffix;
    if (mime.rfind("audio/", 0) != std::string::npos) {
        auto it = audio_mime_to_role.find(mime_suffix);
        if (it == audio_mime_to_role.end()) return "";
        prefix = "audio_";
        suffix = it->second;
    } else if (mime.rfind("video/", 0) != std::string::npos) {
        auto it = video_mime_to_role.find(mime_suffix);
        if (it == video_mime_to_role.end()) return "";
        prefix = "video_";
        suffix = it->second;
    } else if (mime.rfind("image/", 0) != std::string::npos) {
        auto it = image_mime_to_role.find(mime_suffix);
        if (it == image_mime_to_role.end()) return "";
        prefix = "image_";
        suffix = it->second;
    } else {
        return "";
    }
    return prefix + middle + suffix;
}

// Make sure IOmx and IOmxStore have the same set of instances.
TEST(MasterHidlTest, instanceMatchValidation) {
    auto omxInstances = android::hardware::getAllHalInstanceNames(IOmx::descriptor);
    auto omxStoreInstances = android::hardware::getAllHalInstanceNames(IOmxStore::descriptor);
    ASSERT_EQ(omxInstances.size(), omxInstances.size());
    for (const std::string& omxInstance : omxInstances) {
        EXPECT_TRUE(std::find(omxStoreInstances.begin(), omxStoreInstances.end(), omxInstance) !=
                    omxStoreInstances.end());
    }
}

// list service attributes and verify expected formats
TEST_P(MasterHidlTest, ListServiceAttr) {
    description("list service attributes");
    android::hardware::media::omx::V1_0::Status status;
    hidl_vec<IOmxStore::Attribute> attributes;
    EXPECT_TRUE(omxStore
                    ->listServiceAttributes([&status, &attributes](
                        android::hardware::media::omx::V1_0::Status _s,
                        hidl_vec<IOmxStore::Attribute> const& _nl) {
                        status = _s;
                        attributes = _nl;
                    })
                    .isOk());
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    if (attributes.size() == 0) {
        ALOGV("Warning, Attribute list empty");
    } else {
        /*
         * known is a map whose keys are the known "key" for a service
         * attribute pair (see IOmxStore::Attribute), and whose values are the
         * corresponding regular expressions that will have to match with the
         * "value" of the attribute pair. If listServiceAttributes() returns an
         * attribute that has a matching key but an unmatched value, the test
         * will fail.
         */
        const std::map<const std::string, const testing::internal::RE> known = {
                {"max-video-encoder-input-buffers", "0|[1-9][0-9]*"},
                {"supports-multiple-secure-codecs", "0|1"},
                {"supports-secure-with-non-secure-codec", "0|1"},
        };
        /*
         * unknown is a map of pairs of strings used for regular expressions.
         * For each attribute whose key is not known (i.e., does not match any
         * of the keys in the "known" variable defined above), that key will be
         * tried for a match with the first element of each pair of the variable
         * "unknown". If a match occurs, the value of that same attribute will be
         * tried for a match with the second element of the pair. If this second
         * match fails, the test will fail.
         */
        const std::map<std::string, std::string> unknown = {{"supports-[a-z0-9-]*", "0|1"}};

        std::set<const std::string> keySet;
        for (IOmxStore::Attribute attr : attributes) {
            // Make sure there are no duplicates
            const auto [keyIter, inserted] = keySet.insert(attr.key);
            EXPECT_EQ(inserted, true);

            const auto knownIter = known.find(attr.key);
            if (knownIter != known.end()) {
                // For known attributes, make sure their value is expected.
                EXPECT_EQ(testing::internal::RE::FullMatch(attr.value, knownIter->second), true);
            } else {
                bool matched = false;
                // For unknown attributes check the unknown keys. If there is a
                // match, make sure the value is expected.
                for (auto it : unknown) {
                    const testing::internal::RE unknownKey = it.first;
                    if (testing::internal::RE::PartialMatch(attr.key, unknownKey)) {
                        matched = true;
                        const testing::internal::RE unknownValue = it.second;
                        EXPECT_EQ(testing::internal::RE::FullMatch(attr.value, unknownValue), true);
                    }
                }
                if (!matched) {
                    ALOGV("Unrecognized service attribute \"%s\" with value "
                          "\"%s\".",
                          attr.key.c_str(), attr.value.c_str());
                }
            }
        }
    }
}

// get node prefix
TEST_P(MasterHidlTest, getNodePrefix) {
    description("get node prefix");
    hidl_string prefix;
    omxStore->getNodePrefix(
        [&prefix](hidl_string const& _nl) { prefix = _nl; });
    if (prefix.empty()) ALOGV("Warning, Node Prefix empty");
}

// list roles and validate all RoleInfo objects
TEST_P(MasterHidlTest, ListRoles) {
    description("list roles");
    hidl_vec<IOmxStore::RoleInfo> roleList;
    omxStore->listRoles([&roleList](hidl_vec<IOmxStore::RoleInfo> const& _nl) {
        roleList = _nl;
    });
    if (roleList.size() == 0) {
        ALOGV("Warning, RoleInfo list empty");
    } else {
        std::set<const std::string> roleSet;
        for (IOmxStore::RoleInfo role : roleList) {
            // Make sure there are no duplicates
            const auto [roleIter, inserted] = roleSet.insert(role.role);
            EXPECT_EQ(inserted, true);

            // Make sure role name follows expected format based on type and
            // isEncoder
            const std::string role_name = getComponentRole(role.isEncoder, role.type);
            EXPECT_EQ(role_name, role.role);

            // Check the nodes for this role
            std::set<const std::string> nodeSet;
            for (IOmxStore::NodeInfo node : role.nodes) {
                // Make sure there are no duplicates
                const auto [nodeIter, inserted] = nodeSet.insert(node.name);
                EXPECT_EQ(inserted, false);

                // TODO(devinmoore) all of these nodes' vectors are empty. Is that
                // a problem? Just due to the device I'm testing? Is it worth adding
                // the rest of the test logic here?
            }
        }
    }
}

// list components and roles.
TEST_P(MasterHidlTest, ListNodes) {
    description("enumerate component and roles");
    android::hardware::media::omx::V1_0::Status status;
    hidl_vec<IOmx::ComponentInfo> nodeList;
    bool isPass = true;
    EXPECT_TRUE(
        omx->listNodes([&status, &nodeList](
                           android::hardware::media::omx::V1_0::Status _s,
                           hidl_vec<IOmx::ComponentInfo> const& _nl) {
               status = _s;
               nodeList = _nl;
           })
            .isOk());
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    if (nodeList.size() == 0)
        ALOGV("Warning, ComponentInfo list empty");
    else {
        // displayComponentInfo(nodeList);
        for (size_t i = 0; i < nodeList.size(); i++) {
            sp<CodecObserver> observer = nullptr;
            sp<IOmxNode> omxNode = nullptr;
            observer = new CodecObserver(nullptr);
            ASSERT_NE(observer, nullptr);
            EXPECT_TRUE(
                omx->allocateNode(
                       nodeList[i].mName, observer,
                       [&](android::hardware::media::omx::V1_0::Status _s,
                           sp<IOmxNode> const& _nl) {
                           status = _s;
                           omxNode = _nl;
                       })
                    .isOk());
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            if (omxNode == nullptr) {
                isPass = false;
                std::cerr << "[    !OK   ] " << nodeList[i].mName.c_str()
                          << "\n";
            } else {
                EXPECT_TRUE((omxNode->freeNode()).isOk());
                omxNode = nullptr;
                // std::cout << "[     OK   ] " << nodeList[i].mName.c_str() <<
                // "\n";
            }
        }
    }
    EXPECT_TRUE(isPass);
}

INSTANTIATE_TEST_CASE_P(
        PerInstance, MasterHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IOmxStore::descriptor)),
        android::hardware::PrintInstanceNameToString);
