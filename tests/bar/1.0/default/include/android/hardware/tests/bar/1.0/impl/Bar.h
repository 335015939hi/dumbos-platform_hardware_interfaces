#ifndef HIDL_GENERATED_android_hardware_tests_bar_V1_0_Bar_H_
#define HIDL_GENERATED_android_hardware_tests_bar_V1_0_Bar_H_

#include <android/hardware/tests/bar/1.0/IBar.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::bar::V1_0::IBar;
using ::android::hardware::tests::foo::V1_0::Abc;
using ::android::hardware::tests::foo::V1_0::IFoo;
using ::android::hardware::tests::foo::V1_0::IFooCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Bar : public IBar {
    // Methods from ::android::hardware::tests::foo::V1_0::IFoo follow.
    Return<void> doThis(float param)  override;
    Return<int32_t> doThatAndReturnSomething(int64_t param)  override;
    Return<double> doQuiteABit(int32_t a, int64_t b, float c, double d)  override;
    Return<void> doSomethingElse(const hidl_array<int32_t, 15 /* 15 */>& param, doSomethingElse_cb _hidl_cb)  override;
    Return<void> doStuffAndReturnAString(doStuffAndReturnAString_cb _hidl_cb)  override;
    Return<void> mapThisVector(const hidl_vec<int32_t>& param, mapThisVector_cb _hidl_cb)  override;
    Return<void> callMe(const sp<IFooCallback>& cb)  override;
    Return<IFoo::SomeEnum> useAnEnum(IFoo::SomeEnum zzz)  override;
    Return<void> haveAGooberVec(const hidl_vec<IFoo::Goober>& param)  override;
    Return<void> haveAGoober(const IFoo::Goober& g)  override;
    Return<void> haveAGooberArray(const hidl_array<IFoo::Goober, 20 /* 20 */>& lots)  override;
    Return<void> haveATypeFromAnotherFile(const Abc& def)  override;
    Return<void> haveSomeStrings(const hidl_array<hidl_string, 3 /* 3 */>& array, haveSomeStrings_cb _hidl_cb)  override;
    Return<void> haveAStringVec(const hidl_vec<hidl_string>& vector, haveAStringVec_cb _hidl_cb)  override;
    Return<void> transposeMe(const hidl_array<float, 3 /* 3 */, 5 /* 5 */>& in, transposeMe_cb _hidl_cb)  override;
    Return<void> callingDrWho(const IFoo::MultiDimensional& in, callingDrWho_cb _hidl_cb)  override;
    Return<void> transpose(const IFoo::StringMatrix5x3& in, transpose_cb _hidl_cb)  override;
    Return<void> transpose2(const hidl_array<hidl_string, 5 /* 5 */, 3 /* 3 */>& in, transpose2_cb _hidl_cb)  override;
    Return<void> sendVec(const hidl_vec<uint8_t>& data, sendVec_cb _hidl_cb)  override;
    Return<void> sendVecVec(sendVecVec_cb _hidl_cb)  override;

    // Methods from ::android::hardware::tests::bar::V1_0::IBar follow.
    Return<void> thisIsNew()  override;

};

extern "C" IBar* HIDL_FETCH_IBar(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_bar_V1_0_Bar_H_
