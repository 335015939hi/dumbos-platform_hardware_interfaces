/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "keymaster_4_1_hidl_hal_test"

#include <android-base/logging.h>

#include <android/hidl/manager/1.0/IServiceManager.h>

#include <android/hardware/keymaster/4.0/types.h>
#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.1/types.h>
#include <keymaster/keymaster_configuration.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_1 {

namespace test {

using ::android::sp;
using ::std::string;
using V4_0::ErrorCode;
using V4_0::KeyCharacteristics;
using V4_0::OperationHandle;
using V4_0::SecurityLevel;

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

class KeymasterHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static KeymasterHidlEnvironment* Instance() {
        static KeymasterHidlEnvironment* instance = new KeymasterHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<IKeymasterDevice>(); }

   private:
    KeymasterHidlEnvironment(){};

    GTEST_DISALLOW_COPY_AND_ASSIGN_(KeymasterHidlEnvironment);
};

class KeymasterHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    void TearDown() override { AbortIfNeeded(); }

    // SetUpTestCase runs only once per test case, not once per test.
    static void SetUpTestCase();
    static void TearDownTestCase() {
        keymaster_.clear();
        all_keymasters_.clear();
    }

    ErrorCode Abort(OperationHandle op_handle);
    void AbortIfNeeded();

    static IKeymasterDevice& keymaster() { return *keymaster_; }
    static const std::vector<sp<IKeymasterDevice>>& all_keymasters() { return all_keymasters_; }
    static uint32_t os_version() { return os_version_; }
    static uint32_t os_patch_level() { return os_patch_level_; }

    KeyCharacteristics key_characteristics_;
    OperationHandle op_handle_ = kOpHandleSentinel;

   private:
    static sp<IKeymasterDevice> keymaster_;
    static std::vector<sp<IKeymasterDevice>> all_keymasters_;
    static uint32_t os_version_;
    static uint32_t os_patch_level_;

    static CertificationLevel certificationLevel_;
    static SecurityLevel securityLevel_;
    static hidl_string name_;
    static hidl_string author_;
};

void KeymasterHidlTest::SetUpTestCase() {
    string service_name = KeymasterHidlEnvironment::Instance()->getServiceName<IKeymasterDevice>();
    keymaster_ = ::testing::VtsHalHidlTargetTestBase::getService<IKeymasterDevice>(service_name);
    ASSERT_NE(keymaster_, nullptr);

    ASSERT_TRUE(keymaster_
                    ->getHardwareInfo_4_1([&](V4_0::SecurityLevel securityLevel,
                                              const hidl_string& name, const hidl_string& author,
                                              CertificationLevel certificationLevel) {
                        securityLevel_ = securityLevel;
                        name_ = name;
                        author_ = author;
                        certificationLevel_ = certificationLevel;
                    })
                    .isOk());

    os_version_ = ::keymaster::GetOsVersion();
    os_patch_level_ = ::keymaster::GetOsPatchlevel();

    auto service_manager = android::hidl::manager::V1_0::IServiceManager::getService();
    ASSERT_NE(nullptr, service_manager.get());

    all_keymasters_.push_back(keymaster_);
    service_manager->listByInterface(
        IKeymasterDevice::descriptor, [&](const hidl_vec<hidl_string>& names) {
            for (auto& name : names) {
                if (name == service_name) continue;
                auto keymaster =
                    ::testing::VtsHalHidlTargetTestBase::getService<IKeymasterDevice>(name);
                ASSERT_NE(keymaster, nullptr);
                all_keymasters_.push_back(keymaster);
            }
        });
}

ErrorCode KeymasterHidlTest::Abort(OperationHandle op_handle) {
    SCOPED_TRACE("Abort");
    auto retval = keymaster_->abort(op_handle);
    EXPECT_TRUE(retval.isOk());
    return retval;
}

void KeymasterHidlTest::AbortIfNeeded() {
    SCOPED_TRACE("AbortIfNeeded");
    if (op_handle_ != kOpHandleSentinel) {
        EXPECT_EQ(ErrorCode::OK, Abort(op_handle_));
        op_handle_ = kOpHandleSentinel;
    }
}

}  // namespace test
}  // namespace V4_1
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

// // Test transmit() for the min and max frequency of every available range
// TEST_F(ConsumerIrHidlTest, TransmitTest) {
//   bool success;
//   hidl_vec<ConsumerIrFreqRange> ranges;
//   auto cb = [&](bool s, hidl_vec<ConsumerIrFreqRange> v) {
//     ranges = v;
//     success = s;
//   };
//   Return<void> ret = ir->getCarrierFreqs(cb);
//   ASSERT_TRUE(ret.isOk());
//   ASSERT_TRUE(success);

//   if (ranges.size() > 0) {
//     uint32_t len = 16;
//     hidl_vec<int32_t> vec;
//     vec.resize(len);
//     std::fill(vec.begin(), vec.end(), 1000);
//     for (auto range = ranges.begin(); range != ranges.end(); range++) {
//       EXPECT_TRUE(ir->transmit(range->min, vec));
//       EXPECT_TRUE(ir->transmit(range->max, vec));
//     }
//   }
// }

// // Test transmit() when called with invalid frequencies
// TEST_F(ConsumerIrHidlTest, BadFreqTest) {
//   uint32_t len = 16;
//   hidl_vec<int32_t> vec;
//   vec.resize(len);
//   std::fill(vec.begin(), vec.end(), 1);
//   EXPECT_FALSE(ir->transmit(-1, vec));
// }

// int main(int argc, char **argv) {
//   ::testing::AddGlobalTestEnvironment(ConsumerIrHidlEnvironment::Instance());
//   ::testing::InitGoogleTest(&argc, argv);
//   ConsumerIrHidlEnvironment::Instance()->init(&argc, argv);
//   int status = RUN_ALL_TESTS();
//   LOG(INFO) << "Test result = " << status;
//   return status;
// }
