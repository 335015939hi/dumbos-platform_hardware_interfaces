/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/cas/BnCas.h>
#include <aidl/android/hardware/cas/ICasListener.h>
#include <aidl/android/hardware/cas/Status.h>
#include <media/stagefright/foundation/ABase.h>
#include <utils/RefBase.h>

namespace android {
struct CasPlugin;

namespace hardware {
namespace cas {
struct ICasListener;
namespace implementation {

using aidl::android::hardware::cas::BnCas;
using aidl::android::hardware::cas::ICasListener;
using aidl::android::hardware::cas::ScramblingMode;
using aidl::android::hardware::cas::SessionIntent;
using aidl::android::hardware::cas::Status;
using aidl::android::hardware::cas::StatusEvent;

class SharedLibrary;

class CasImpl : public BnCas {
  public:
    CasImpl(const std::shared_ptr<ICasListener>& listener);
    virtual ~CasImpl();

    static void OnEvent(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size);

    static void CallBackExt(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size,
                            const CasSessionId* sessionId);

    static void StatusUpdate(void* appData, int32_t event, int32_t arg);

    void init(CasPlugin* plugin);
    void onEvent(int32_t event, int32_t arg, uint8_t* data, size_t size);

    void onEvent(const CasSessionId* sessionId, int32_t event, int32_t arg, uint8_t* data,
                 size_t size);

    void onStatusUpdate(int32_t event, int32_t arg);

    // ICas inherits

    ::ndk::ScopedAStatus setPluginStatusUpdateCallback();

    virtual ::ndk::ScopedAStatus setPrivateData(const std::vector<uint8_t>& pvtData) override;

    virtual ::ndk::ScopedAStatus openSession(SessionIntent intent, ScramblingMode mode,
                                             std::vector<uint8_t>* _aidl_return) override;

    virtual ::ndk::ScopedAStatus closeSession(const std::vector<uint8_t>& sessionId) override;

    virtual ::ndk::ScopedAStatus setSessionPrivateData(
            const std::vector<uint8_t>& sessionId, const std::vector<uint8_t>& pvtData) override;

    virtual ::ndk::ScopedAStatus processEcm(const std::vector<uint8_t>& sessionId,
                                            const std::vector<uint8_t>& ecm) override;

    virtual ::ndk::ScopedAStatus processEmm(const std::vector<uint8_t>& emm) override;

    virtual ::ndk::ScopedAStatus sendEvent(int32_t event, int32_t arg,
                                           const std::vector<uint8_t>& eventData) override;

    virtual ::ndk::ScopedAStatus sendSessionEvent(const std::vector<uint8_t>& sessionId,
                                                  int32_t event, int32_t arg,
                                                  const std::vector<uint8_t>& eventData) override;

    virtual ::ndk::ScopedAStatus provision(const std::string& provisionString) override;

    virtual ::ndk::ScopedAStatus refreshEntitlements(
            int32_t refreshType, const std::vector<uint8_t>& refreshData) override;

    virtual ::ndk::ScopedAStatus release() override;

  private:
    struct PluginHolder;
    std::shared_ptr<CasPlugin> mPluginHolder;
    std::shared_ptr<ICasListener> mListener;

    DISALLOW_EVIL_CONSTRUCTORS(CasImpl);
};

}  // namespace implementation
}  // namespace cas
}  // namespace hardware
}  // namespace android
