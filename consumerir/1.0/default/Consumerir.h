#ifndef ANDROID_HARDWARE_CONSUMERIR_V1_0_CONSUMERIR_H
#define ANDROID_HARDWARE_CONSUMERIR_V1_0_CONSUMERIR_H

#include <android/hardware/consumerir/1.0/IConsumerir.h>
#include <hardware/consumerir.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace consumerir {
namespace V1_0 {
namespace implementation {

using ::android::hardware::consumerir::V1_0::ConsumerirFreqRange;
using ::android::hardware::consumerir::V1_0::IConsumerir;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Consumerir : public IConsumerir {
    Consumerir(consumerir_device_t *device);
    // Methods from ::android::hardware::consumerir::V1_0::IConsumerir follow.
    Return<int32_t> transmit(int32_t carrierFreq, const hidl_vec<int32_t>& pattern, int32_t patternLen) override;
    Return<int32_t> getNumCarrierFreqs() override;
    Return<void> getCarrierFreqs(uint64_t len, getCarrierFreqs_cb _hidl_cb) override;
private:
    consumerir_device_t *mDevice;
};

extern "C" IConsumerir* HIDL_FETCH_IConsumerir(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace consumerir
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONSUMERIR_V1_0_CONSUMERIR_H
