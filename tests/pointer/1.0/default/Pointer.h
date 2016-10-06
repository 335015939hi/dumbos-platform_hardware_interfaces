#ifndef HIDL_GENERATED_android_hardware_tests_pointer_V1_0_Pointer_H_
#define HIDL_GENERATED_android_hardware_tests_pointer_V1_0_Pointer_H_

#include <android/hardware/tests/pointer/1.0/IPointer.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace pointer {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::pointer::V1_0::IPointer;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Pointer : public IPointer {
    // Methods from ::android::hardware::tests::pointer::V1_0::IPointer follow.
    Return<void> foo1(const IPointer::Sam& s, ::android::hardware::tests::pointer::V1_0::IPointer::Sam const* s_ptr)  override;
    Return<void> foo2(const IPointer::Sam& s, const IPointer::Ada& a)  override;
    Return<void> foo3(const IPointer::Sam& s, const IPointer::Ada& a, const IPointer::Bob& b)  override;
    Return<void> foo4(::android::hardware::tests::pointer::V1_0::IPointer::Sam const* s_ptr)  override;
    Return<void> foo5(const IPointer::Ada& a, const IPointer::Bob& b)  override;
    Return<void> foo6(::android::hardware::tests::pointer::V1_0::IPointer::Ada const* a_ptr)  override;
    Return<void> foo7(::android::hardware::tests::pointer::V1_0::IPointer::Ada const* a_ptr, ::android::hardware::tests::pointer::V1_0::IPointer::Bob const* b_ptr)  override;
    Return<void> foo8(const IPointer::Dom& d)  override;
    Return<void> foo9(::android::hardware::hidl_string const* str_ref)  override;
    Return<void> foo10(const hidl_vec<::android::hardware::tests::pointer::V1_0::IPointer::Sam const*>& s_ptr_vec)  override;
    Return<void> foo11(::android::hardware::hidl_vec<::android::hardware::tests::pointer::V1_0::IPointer::Sam> const* s_vec_ptr)  override;
    Return<void> foo12(hidl_array<::android::hardware::tests::pointer::V1_0::IPointer::Sam, 5 /* 5 */> const* s_array_ref)  override;
    Return<void> foo13(const hidl_array<::android::hardware::tests::pointer::V1_0::IPointer::Sam const*, 5 /* 5 */>& s_ref_array)  override;
    Return<void> foo14(::android::hardware::tests::pointer::V1_0::IPointer::Sam const* const* const* s_3ptr)  override;
    Return<void> foo15(int32_t const* const* const* i_3ptr)  override;
    Return<void> foo16(const IPointer::Ptr& p)  override;
    Return<void> foo17(::android::hardware::tests::pointer::V1_0::IPointer::Ptr const* p)  override;
    Return<void> foo18(::android::hardware::hidl_string const* str_ref, ::android::hardware::hidl_string const* str_ref2, const hidl_string& str)  override;
    Return<void> foo19(::android::hardware::hidl_vec<::android::hardware::tests::pointer::V1_0::IPointer::Ada> const* a_vec_ref, const hidl_vec<IPointer::Ada>& a_vec, ::android::hardware::hidl_vec<::android::hardware::tests::pointer::V1_0::IPointer::Ada> const* a_vec_ref2)  override;
    Return<void> foo20(const hidl_vec<::android::hardware::tests::pointer::V1_0::IPointer::Sam const*>& s_ptr_vec)  override;
    Return<void> foo21(hidl_array<::android::hardware::tests::pointer::V1_0::IPointer::Ada, 3 /* 3 */, 2 /* 2 */, 1 /* 1 */> const* a_array_ptr)  override;
    Return<void> foo22(const hidl_array<::android::hardware::tests::pointer::V1_0::IPointer::Ada const*, 3 /* 3 */, 2 /* 2 */, 1 /* 1 */>& a_ptr_array)  override;
    Return<void> bar1(bar1_cb _hidl_cb)  override;
    Return<void> bar2(bar2_cb _hidl_cb)  override;
    Return<void> bar3(bar3_cb _hidl_cb)  override;
    Return<void> bar4(bar4_cb _hidl_cb)  override;
    Return<void> bar5(bar5_cb _hidl_cb)  override;
    Return<void> bar6(bar6_cb _hidl_cb)  override;
    Return<void> bar7(bar7_cb _hidl_cb)  override;
    Return<void> bar8(bar8_cb _hidl_cb)  override;
    Return<void> bar9(bar9_cb _hidl_cb)  override;
    Return<void> bar10(bar10_cb _hidl_cb)  override;
    Return<void> bar11(bar11_cb _hidl_cb)  override;
    Return<void> bar12(bar12_cb _hidl_cb)  override;
    Return<void> bar13(bar13_cb _hidl_cb)  override;
    Return<void> bar14(bar14_cb _hidl_cb)  override;
    Return<void> bar15(bar15_cb _hidl_cb)  override;
    Return<void> bar16(bar16_cb _hidl_cb)  override;
    Return<void> bar17(bar17_cb _hidl_cb)  override;
    Return<void> bar18(bar18_cb _hidl_cb)  override;
    Return<void> bar19(bar19_cb _hidl_cb)  override;
    Return<void> bar20(bar20_cb _hidl_cb)  override;
    Return<void> bar21(bar21_cb _hidl_cb)  override;
    Return<void> bar22(bar22_cb _hidl_cb)  override;
    Return<int32_t> getErrors()  override;

};

extern "C" IPointer* HIDL_FETCH_IPointer(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace pointer
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_pointer_V1_0_Pointer_H_
