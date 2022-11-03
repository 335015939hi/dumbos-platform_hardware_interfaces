// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#include "android/hardware/secure_element/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::secure_element::SecureElementStatus::SUCCESS ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::SUCCESS));
static_assert(aidl::android::hardware::secure_element::SecureElementStatus::FAILED ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::FAILED));
static_assert(aidl::android::hardware::secure_element::SecureElementStatus::CHANNEL_NOT_AVAILABLE ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::
                              CHANNEL_NOT_AVAILABLE));
static_assert(aidl::android::hardware::secure_element::SecureElementStatus::NO_SUCH_ELEMENT_ERROR ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::
                              NO_SUCH_ELEMENT_ERROR));
static_assert(aidl::android::hardware::secure_element::SecureElementStatus::UNSUPPORTED_OPERATION ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::
                              UNSUPPORTED_OPERATION));
static_assert(aidl::android::hardware::secure_element::SecureElementStatus::IOERROR ==
              static_cast<aidl::android::hardware::secure_element::SecureElementStatus>(
                      ::android::hardware::secure_element::V1_0::SecureElementStatus::IOERROR));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::secure_element::V1_0::LogicalChannelResponse& in,
        aidl::android::hardware::secure_element::LogicalChannelResponse* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.channelNumber > std::numeric_limits<int8_t>::max() || in.channelNumber < 0) {
        return false;
    }
    out->channelNumber = static_cast<int8_t>(in.channelNumber);
    {
        size_t size = in.selectResponse.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.selectResponse[i] > std::numeric_limits<int8_t>::max() ||
                in.selectResponse[i] < 0) {
                return false;
            }
            out->selectResponse.push_back(static_cast<int8_t>(in.selectResponse[i]));
        }
    }
    return true;
}

}  // namespace android::h2a