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

#include <keystore/IKeystoreService.h>
#include <binder/IServiceManager.h>

#ifndef HIDL_RETURN_UTIL_H_
#define HIDL_RETURN_UTIL_H_

namespace android {
namespace hardware {
namespace wifi {
namespace keystore {
namespace V1_0 {
namespace implementation {
namespace hidl_return_util {

/**
 * This utility function is used to invoke a method on the provided
 * HIDL interface object.
 * This function checks if the provided HIDL interface object is valid.
 * a) if valid, it invokes the corresponding internal implementation
 *    function of the HIDL method. It then invokes the HIDL continuation
      callback with the status and any returned values.
 * b) if invalid, invokes the HIDL continuation callback with the
 *    provided error status and default values.
 */

// Use for HIDL methods which return a KeystoreStatusCode and a single return
// value.
template <typename ObjT, typename WorkFuncT, typename ReturnT, typename... Args>
Return<void> validateAndCall(
        ObjT* obj,
        IKeystore::KeystoreStatusCode status_code_if_invalid,
        WorkFuncT&& work,
        const std::function<void(IKeystore::KeystoreStatusCode, ReturnT)>&
                hidl_cb,
        Args&&... args) {
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(
            defaultServiceManager()->getService(
                    String16("android.security.keystore")));

    if (service != nullptr) {
        const auto& ret_pair = (obj->*work)(
                service, std::forward<Args>(args)...);
        const IKeystore::KeystoreStatusCode status = std::get<0>(ret_pair);
        const auto& ret_value = std::get<1>(ret_pair);
        hidl_cb(status, ret_value);
    } else {
        hidl_cb(status_code_if_invalid,
                typename std::remove_reference<ReturnT>::type());
    }
    return Void();
}

}  // namespace hidl_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace keystore
}  // namespace wifi
}  // namespace hardware
}  // namespace android
#endif  // HIDL_RETURN_UTIL_H_
