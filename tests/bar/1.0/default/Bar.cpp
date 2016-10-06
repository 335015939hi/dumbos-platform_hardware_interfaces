#include <android/hardware/tests/bar/1.0/impl/Bar.h>
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::foo::V1_0::IFoo follow.
// None of them are implemented, as they are not tested under the IBar interface,
// but only under the IFoo interface.
Return<void> Bar::doThis(float)  {
    CHECK(!"implemented");
    return Void();
}

Return<int32_t> Bar::doThatAndReturnSomething(int64_t)  {
    CHECK(!"implemented");
    return int32_t {};
}

Return<double> Bar::doQuiteABit(int32_t, int64_t, float, double)  {
    CHECK(!"implemented");
    return double {};
}

Return<void> Bar::doSomethingElse(const hidl_array<int32_t, 15 /* 15 */>&, doSomethingElse_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::doStuffAndReturnAString(doStuffAndReturnAString_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::mapThisVector(const hidl_vec<int32_t>&, mapThisVector_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::callMe(const sp<IFooCallback>&)  {
    CHECK(!"implemented");
    return Void();
}

Return<IFoo::SomeEnum> Bar::useAnEnum(IFoo::SomeEnum)  {
    CHECK(!"implemented");
    return ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum {};
}

Return<void> Bar::haveAGooberVec(const hidl_vec<IFoo::Goober>&)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::haveAGoober(const IFoo::Goober&)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::haveAGooberArray(const hidl_array<IFoo::Goober, 20 /* 20 */>&)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::haveATypeFromAnotherFile(const Abc&)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::haveSomeStrings(const hidl_array<hidl_string, 3 /* 3 */>&, haveSomeStrings_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::haveAStringVec(const hidl_vec<hidl_string>&, haveAStringVec_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::transposeMe(const hidl_array<float, 3 /* 3 */, 5 /* 5 */>&, transposeMe_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::callingDrWho(const IFoo::MultiDimensional&, callingDrWho_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::transpose(const IFoo::StringMatrix5x3&, transpose_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::transpose2(const hidl_array<hidl_string, 5 /* 5 */, 3 /* 3 */>&, transpose2_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::sendVec(const hidl_vec<uint8_t>&, sendVec_cb)  {
    CHECK(!"implemented");
    return Void();
}

Return<void> Bar::sendVecVec(sendVecVec_cb)  {
    CHECK(!"implemented");
    return Void();
}


// Methods from ::android::hardware::tests::bar::V1_0::IBar follow.
Return<void> Bar::thisIsNew()  {
    ALOGI("SERVER(Bar) thisIsNew");

    return Void();
}


IBar* HIDL_FETCH_IBar(const char* /* name */) {
    return new Bar();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android
