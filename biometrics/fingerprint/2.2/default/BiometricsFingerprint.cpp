/*Transsion Top Secret*/
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
#define LOG_TAG "android.hardware.biometrics.fingerprint@2.2-service"
#define LOG_VERBOSE "android.hardware.biometrics.fingerprint@2.2-service"

#include <hardware/hw_auth_token.h>

#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include "BiometricsFingerprint.h"

#include <inttypes.h>
#include <unistd.h>
//add XLLSHLSS-1724 by qiaoqiao.pan 20181015 start
#include <cutils/properties.h>
//add XLLSHLSS-1724 by qiaoqiao.pan 20181015 end
#include <dlfcn.h>

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_2 {
namespace implementation {

// Supported fingerprint HAL version
static const uint16_t kVersion = HARDWARE_MODULE_API_VERSION(2, 1);

using RequestStatus =
        android::hardware::biometrics::fingerprint::V2_1::RequestStatus;

BiometricsFingerprint *BiometricsFingerprint::sInstance = nullptr;

BiometricsFingerprint::BiometricsFingerprint() : mClientCallback(nullptr), mDevice(nullptr) {
    sInstance = this; // keep track of the most recent instance
    mDevice = openHal();
    if (!mDevice) {
        ALOGE("Can't open HAL module");
    }
}

BiometricsFingerprint::~BiometricsFingerprint() {
    ALOGV("~BiometricsFingerprint()");
    if (mDevice == nullptr) {
        ALOGE("No valid device");
        return;
    }
    int err;
    if (0 != (err = mDevice->common.close(
            reinterpret_cast<hw_device_t*>(mDevice)))) {
        ALOGE("Can't close fingerprint module, error: %d", err);
        return;
    }
    mDevice = nullptr;
}

Return<RequestStatus> BiometricsFingerprint::ErrorFilter(int32_t error) {
    switch(error) {
        case 0: return RequestStatus::SYS_OK;
        case -2: return RequestStatus::SYS_ENOENT;
        case -4: return RequestStatus::SYS_EINTR;
        case -5: return RequestStatus::SYS_EIO;
        case -11: return RequestStatus::SYS_EAGAIN;
        case -12: return RequestStatus::SYS_ENOMEM;
        case -13: return RequestStatus::SYS_EACCES;
        case -14: return RequestStatus::SYS_EFAULT;
        case -16: return RequestStatus::SYS_EBUSY;
        case -22: return RequestStatus::SYS_EINVAL;
        case -28: return RequestStatus::SYS_ENOSPC;
        case -110: return RequestStatus::SYS_ETIMEDOUT;
        default:
            ALOGE("An unknown error returned from fingerprint vendor library: %d", error);
            return RequestStatus::SYS_UNKNOWN;
    }
}

// Translate from errors returned by traditional HAL (see fingerprint.h) to
// HIDL-compliant FingerprintError.
FingerprintError BiometricsFingerprint::VendorErrorFilter(int32_t error,
            int32_t* vendorCode) {
    *vendorCode = 0;
    switch(error) {
        case FINGERPRINT_ERROR_HW_UNAVAILABLE:
            return FingerprintError::ERROR_HW_UNAVAILABLE;
        case FINGERPRINT_ERROR_UNABLE_TO_PROCESS:
            return FingerprintError::ERROR_UNABLE_TO_PROCESS;
        case FINGERPRINT_ERROR_TIMEOUT:
            return FingerprintError::ERROR_TIMEOUT;
        case FINGERPRINT_ERROR_NO_SPACE:
            return FingerprintError::ERROR_NO_SPACE;
        case FINGERPRINT_ERROR_CANCELED:
            return FingerprintError::ERROR_CANCELED;
        case FINGERPRINT_ERROR_UNABLE_TO_REMOVE:
            return FingerprintError::ERROR_UNABLE_TO_REMOVE;
        case FINGERPRINT_ERROR_LOCKOUT:
            return FingerprintError::ERROR_LOCKOUT;
        default:
            if (error >= FINGERPRINT_ERROR_VENDOR_BASE) {
                // vendor specific code.
                *vendorCode = error - FINGERPRINT_ERROR_VENDOR_BASE;
                return FingerprintError::ERROR_VENDOR;
            }
    }
    ALOGE("Unknown error from fingerprint vendor library: %d", error);
    return FingerprintError::ERROR_UNABLE_TO_PROCESS;
}

// Translate acquired messages returned by traditional HAL (see fingerprint.h)
// to HIDL-compliant FingerprintAcquiredInfo.
FingerprintAcquiredInfo BiometricsFingerprint::VendorAcquiredFilter(
        int32_t info, int32_t* vendorCode) {
    *vendorCode = 0;
    switch(info) {
        case FINGERPRINT_ACQUIRED_GOOD:
            return FingerprintAcquiredInfo::ACQUIRED_GOOD;
        case FINGERPRINT_ACQUIRED_PARTIAL:
            return FingerprintAcquiredInfo::ACQUIRED_PARTIAL;
        case FINGERPRINT_ACQUIRED_INSUFFICIENT:
            return FingerprintAcquiredInfo::ACQUIRED_INSUFFICIENT;
        case FINGERPRINT_ACQUIRED_IMAGER_DIRTY:
            return FingerprintAcquiredInfo::ACQUIRED_IMAGER_DIRTY;
        case FINGERPRINT_ACQUIRED_TOO_SLOW:
            return FingerprintAcquiredInfo::ACQUIRED_TOO_SLOW;
        case FINGERPRINT_ACQUIRED_TOO_FAST:
            return FingerprintAcquiredInfo::ACQUIRED_TOO_FAST;
        default:
            if (info >= FINGERPRINT_ACQUIRED_VENDOR_BASE) {
                // vendor specific code.
                *vendorCode = info - FINGERPRINT_ACQUIRED_VENDOR_BASE;
                return FingerprintAcquiredInfo::ACQUIRED_VENDOR;
            }
    }
    ALOGE("Unknown acquiredmsg from fingerprint vendor library: %d", info);
    return FingerprintAcquiredInfo::ACQUIRED_INSUFFICIENT;
}

Return<uint64_t> BiometricsFingerprint::setNotify(
        const sp<IBiometricsFingerprintClientCallback>& clientCallback) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    mClientCallback = clientCallback;
    // This is here because HAL 2.1 doesn't have a way to propagate a
    // unique token for its driver. Subsequent versions should send a unique
    // token for each call to setNotify(). This is fine as long as there's only
    // one fingerprint device on the platform.
    return reinterpret_cast<uint64_t>(mDevice);
}

Return<uint64_t> BiometricsFingerprint::preEnroll()  {
    return mDevice->pre_enroll(mDevice);
}

Return<RequestStatus> BiometricsFingerprint::enroll(const hidl_array<uint8_t, 69>& hat,
        uint32_t gid, uint32_t timeoutSec) {
    const hw_auth_token_t* authToken =
        reinterpret_cast<const hw_auth_token_t*>(hat.data());
    return ErrorFilter(mDevice->enroll(mDevice, authToken, gid, timeoutSec));
}

Return<RequestStatus> BiometricsFingerprint::postEnroll() {
    return ErrorFilter(mDevice->post_enroll(mDevice));
}

Return<uint64_t> BiometricsFingerprint::getAuthenticatorId() {
    return mDevice->get_authenticator_id(mDevice);
}

Return<RequestStatus> BiometricsFingerprint::cancel() {
    return ErrorFilter(mDevice->cancel(mDevice));
}

Return<RequestStatus> BiometricsFingerprint::enumerate()  {
    return ErrorFilter(mDevice->enumerate(mDevice));
}

Return<RequestStatus> BiometricsFingerprint::remove(uint32_t gid, uint32_t fid) {
    return ErrorFilter(mDevice->remove(mDevice, gid, fid));
}

Return<RequestStatus> BiometricsFingerprint::setActiveGroup(uint32_t gid,
        const hidl_string& storePath) {
    if (storePath.size() >= PATH_MAX || storePath.size() <= 0) {
        ALOGE("Bad path length: %zd", storePath.size());
        return RequestStatus::SYS_EINVAL;
    }
    if (access(storePath.c_str(), W_OK)) {
        return RequestStatus::SYS_EINVAL;
    }

    return ErrorFilter(mDevice->set_active_group(mDevice, gid,
                                                    storePath.c_str()));
}

Return<RequestStatus> BiometricsFingerprint::authenticate(uint64_t operationId,
        uint32_t gid) {
    return ErrorFilter(mDevice->authenticate(mDevice, operationId, gid));
}

IBiometricsFingerprint* BiometricsFingerprint::getInstance() {
    if (!sInstance) {
      sInstance = new BiometricsFingerprint();
    }
    return sInstance;
}

//modify XLLSHLSS-819 by qiaoqiao.pan 20181012 start
static int get_fingerprint_vendor(void)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;
    /* must match vendor_list in openHal() */
    const char *vendor[] = {"sunwave", "microarray", "goodix", "chipone"};
    const int VENDOR_COUNT = (sizeof(vendor)/sizeof(vendor[0]));

    fp = fopen("/proc/rlkdevinfo", "r");
    if (fp == NULL) {
        ALOGE("get_fingerprint_vendor fopen fail\n");
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, "FINGERPRINT")) {
            ALOGD("FINGERPRINT line is %s", line);
            break;
        }
    }

    for (i = 0 ; i < VENDOR_COUNT; i++) {
        if (strstr(line, vendor[i])) {
            property_set("sys.fingerprint.support", vendor[i]);
            ALOGD("FINGERPRINT type is %s", vendor[i]);
            break;
        }
    }

    free(line);
    fclose(fp);

    if (i >= 0 && i < VENDOR_COUNT) {
        return i;
    } else {
        return -1;
    }
}

static int check_chipone_service(int index)
{
	FILE *fp;
	int ret = -1;
	fp = fopen("/proc/fpsensor", "r");
	if (fp == NULL) {
		//ALOGE("open fpsensor fail----please ignore\n");
		return -1;
	} else  {
		ALOGE("chipone index:%d\n",index);
		if (index != 3) {
			ALOGD("chipone fingerprint hal library");
			char *pFinaFpHalPath = (char *)"/vendor/lib/hw/fpsensor_fingerprint.default.so";
			void* lib_handle_ = dlopen(pFinaFpHalPath, RTLD_NOW);
			if(lib_handle_ == NULL){
				ALOGE(" dlopen failed can't find chipone hal so: %s\n",pFinaFpHalPath);
				return -2;
			}
			int (*load_extsvc2_for_non_fpsnesor)(void) = (int (*)(void))dlsym(lib_handle_, "load_extsvc2_for_non_fpsnesor");
			if(load_extsvc2_for_non_fpsnesor == NULL){
				ALOGE("dlopen can't get load_extsvc2_for_non_fpsnesor\n");
				return -3;
			}
			ret = load_extsvc2_for_non_fpsnesor();
			ALOGD(" load_extsvc2_for_non_fpsnesor is ready? ret:%d\n",ret);
		}
		fclose(fp);
	}
	return 0;
}

fingerprint_device_t* BiometricsFingerprint::openHal() {
    int err;
    const hw_module_t *hw_mdl = nullptr;
    char const* vendor_list[] = {
        "swfingerprint", //sunwave
        "microarray.fingerprint", //microarray
        "gf_fingerprint", //goodix
        "fpsensor_fingerprint", //chipone
        FINGERPRINT_HARDWARE_MODULE_ID //default fingerprint
    };
    const int VENDOR_LIST_COUNT =
        (sizeof(vendor_list)/sizeof(vendor_list[0]));

    int index = get_fingerprint_vendor();
    check_chipone_service(index);
    if (index == -1) {
        index = VENDOR_LIST_COUNT -1;
    }
    ALOGD("Opening fingerprint [%d]-[%s] hal library", index, vendor_list[index]);
    if (0 != (err = hw_get_module(vendor_list[index], &hw_mdl))) {
        ALOGE("Can't open fingerprint HW Module, error: %d", err);
        return nullptr;
    }
//modify XLLSHLSS-819 by qiaoqiao.pan 20181012 end

    if (hw_mdl == nullptr) {
        ALOGE("No valid fingerprint module");
        return nullptr;
    }

    fingerprint_module_t const *module =
        reinterpret_cast<const fingerprint_module_t*>(hw_mdl);
    if (module->common.methods->open == nullptr) {
        ALOGE("No valid open method");
        return nullptr;
    }

    hw_device_t *device = nullptr;

    if (0 != (err = module->common.methods->open(hw_mdl, nullptr, &device))) {
        ALOGE("Can't open fingerprint methods, error: %d", err);
        return nullptr;
    }

    if (kVersion != device->version) {
        // enforce version on new devices because of HIDL@2.1 translation layer
        ALOGE("Wrong fp version. Expected %d, got %d", kVersion, device->version);
        return nullptr;
    }

    fingerprint_device_t* fp_device =
        reinterpret_cast<fingerprint_device_t*>(device);

    if (0 != (err =
            fp_device->set_notify(fp_device, BiometricsFingerprint::notify))) {
        ALOGE("Can't register fingerprint module callback, error: %d", err);
        return nullptr;
    }

    return fp_device;
}

void BiometricsFingerprint::notify(const fingerprint_msg_t *msg) {
    BiometricsFingerprint* thisPtr = static_cast<BiometricsFingerprint*>(
            BiometricsFingerprint::getInstance());
    std::lock_guard<std::mutex> lock(thisPtr->mClientCallbackMutex);
    if (thisPtr == nullptr || thisPtr->mClientCallback == nullptr) {
        ALOGE("Receiving callbacks before the client callback is registered.");
        return;
    }
    const uint64_t devId = reinterpret_cast<uint64_t>(thisPtr->mDevice);
    switch (msg->type) {
        case FINGERPRINT_ERROR: {
                int32_t vendorCode = 0;
                FingerprintError result = VendorErrorFilter(msg->data.error, &vendorCode);
                ALOGD("onError(%d)", result);
                if (!thisPtr->mClientCallback->onError(devId, result, vendorCode).isOk()) {
                    ALOGE("failed to invoke fingerprint onError callback");
                }
            }
            break;
        case FINGERPRINT_ACQUIRED: {
                int32_t vendorCode = 0;
                FingerprintAcquiredInfo result =
                    VendorAcquiredFilter(msg->data.acquired.acquired_info, &vendorCode);
                //modify HLSSPUB-9188 by qiaoqiao.pan 20181015 start
                ALOGD("onAcquired(%d, %d)", result, vendorCode);
                //modify HLSSPUB-9188 by qiaoqiao.pan 20181015 end
                if (!thisPtr->mClientCallback->onAcquired(devId, result, vendorCode).isOk()) {
                    ALOGE("failed to invoke fingerprint onAcquired callback");
                }
            }
            break;
        case FINGERPRINT_TEMPLATE_ENROLLING:
            ALOGD("onEnrollResult(fid=%d, gid=%d, rem=%d)",
                msg->data.enroll.finger.fid,
                msg->data.enroll.finger.gid,
                msg->data.enroll.samples_remaining);
            if (!thisPtr->mClientCallback->onEnrollResult(devId,
                    msg->data.enroll.finger.fid,
                    msg->data.enroll.finger.gid,
                    msg->data.enroll.samples_remaining).isOk()) {
                ALOGE("failed to invoke fingerprint onEnrollResult callback");
            }
            break;
        case FINGERPRINT_TEMPLATE_REMOVED:
            ALOGD("onRemove(fid=%d, gid=%d, rem=%d)",
                msg->data.removed.finger.fid,
                msg->data.removed.finger.gid,
                msg->data.removed.remaining_templates);
            if (!thisPtr->mClientCallback->onRemoved(devId,
                    msg->data.removed.finger.fid,
                    msg->data.removed.finger.gid,
                    msg->data.removed.remaining_templates).isOk()) {
                ALOGE("failed to invoke fingerprint onRemoved callback");
            }
            break;
        case FINGERPRINT_AUTHENTICATED:
            if (msg->data.authenticated.finger.fid != 0) {
                ALOGD("onAuthenticated(fid=%d, gid=%d)",
                    msg->data.authenticated.finger.fid,
                    msg->data.authenticated.finger.gid);
                const uint8_t* hat =
                    reinterpret_cast<const uint8_t *>(&msg->data.authenticated.hat);
                const hidl_vec<uint8_t> token(
                    std::vector<uint8_t>(hat, hat + sizeof(msg->data.authenticated.hat)));
                if (!thisPtr->mClientCallback->onAuthenticated(devId,
                        msg->data.authenticated.finger.fid,
                        msg->data.authenticated.finger.gid,
                        token).isOk()) {
                    ALOGE("failed to invoke fingerprint onAuthenticated callback");
                }
            } else {
                // Not a recognized fingerprint
                if (!thisPtr->mClientCallback->onAuthenticated(devId,
                        msg->data.authenticated.finger.fid,
                        msg->data.authenticated.finger.gid,
                        hidl_vec<uint8_t>()).isOk()) {
                    ALOGE("failed to invoke fingerprint onAuthenticated callback");
                }
            }
            break;
        case FINGERPRINT_TEMPLATE_ENUMERATING:
            ALOGD("onEnumerate(fid=%d, gid=%d, rem=%d)",
                msg->data.enumerated.finger.fid,
                msg->data.enumerated.finger.gid,
                msg->data.enumerated.remaining_templates);
            if (!thisPtr->mClientCallback->onEnumerate(devId,
                    msg->data.enumerated.finger.fid,
                    msg->data.enumerated.finger.gid,
                    msg->data.enumerated.remaining_templates).isOk()) {
                ALOGE("failed to invoke fingerprint onEnumerate callback");
            }
            break;
    }
}

/*[BSP]add XLELHLES-11 by qiaoqiao.pan on 20181217 start*/
Return<uint32_t> BiometricsFingerprint::sendCommand(uint32_t command, uint32_t extras)
{
#ifdef FINGER_PRINT_SUPPORT_ENHANCE
    ALOGD("sendCommand, command<%d>, extras<%d>", command, extras);
    return mDevice->send_command(mDevice, command, extras);
#else
    ALOGE("sendCommand, command<%d>, extras<%d>, Not Supported Board!!!", command, extras);
    return 0;
#endif
}

/*[BSP]add XLELHLES-11 by qiaoqiao.pan on 20181217 end*/

} // namespace implementation
}  // namespace V2_2
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
