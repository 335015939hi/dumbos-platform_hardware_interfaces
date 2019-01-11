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

#ifndef HARDWARE_INTERFACES_KEYMASTER_40_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_UTILS_H_
#define HARDWARE_INTERFACES_KEYMASTER_40_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_UTILS_H_

#include <cutils/log.h>

#include <iostream>

#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/x509.h>

#include <cutils/properties.h>

#include <keymasterV4_0/attestation_record.h>
#include <keymasterV4_0/authorization_set.h>
#include <keymasterV4_0/key_param_output.h>
#include <keymasterV4_0/openssl_utils.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {

template <typename T>
bool operator==(const hidl_vec<T>& a, const hidl_vec<T>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

namespace keymaster {
namespace V4_0 {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b);

bool operator==(const KeyCharacteristics& a, const KeyCharacteristics& b);

namespace test {

using ::android::sp;
using ::std::string;

template <TagType tag_type, Tag tag, typename ValueT>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag> ttag, ValueT expected_value) {
    size_t count = std::count_if(set.begin(), set.end(), [&](const KeyParameter& param) {
        return param.tag == tag && accessTagValue(ttag, param) == expected_value;
    });
    return count == 1;
}

template <TagType tag_type, Tag tag>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag>) {
    size_t count = std::count_if(set.begin(), set.end(),
                                 [&](const KeyParameter& param) { return param.tag == tag; });
    return count > 0;
}

class HidlBuf : public hidl_vec<uint8_t> {
    typedef hidl_vec<uint8_t> super;

   public:
    HidlBuf() {}
    HidlBuf(const super& other) : super(other) {}
    HidlBuf(super&& other) : super(std::move(other)) {}
    explicit HidlBuf(const std::string& other) : HidlBuf() { *this = other; }

    HidlBuf& operator=(const super& other) {
        super::operator=(other);
        return *this;
    }

    HidlBuf& operator=(super&& other) {
        super::operator=(std::move(other));
        return *this;
    }

    HidlBuf& operator=(const string& other) {
        resize(other.size());
        std::copy(other.begin(), other.end(), begin());
        return *this;
    }

    string to_string() const { return string(reinterpret_cast<const char*>(data()), size()); }
};

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

constexpr char hex_value[256] = {0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'..'f'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};

constexpr char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

const string hex2str(string a);

const string bin2hex(const hidl_vec<uint8_t>& data);

const string rsa_key =
        hex2str("30820275020100300d06092a864886f70d01010105000482025f3082025b"
                "02010002818100c6095409047d8634812d5a218176e45c41d60a75b13901"
                "f234226cffe776521c5a77b9e389417b71c0b6a44d13afe4e4a2805d46c9"
                "da2935adb1ff0c1f24ea06e62b20d776430a4d435157233c6f916783c30e"
                "310fcbd89b85c2d56771169785ac12bca244abda72bfb19fc44d27c81e1d"
                "92de284f4061edfd99280745ea6d2502030100010281801be0f04d9cae37"
                "18691f035338308e91564b55899ffb5084d2460e6630257e05b3ceab0297"
                "2dfabcd6ce5f6ee2589eb67911ed0fac16e43a444b8c861e544a05933657"
                "72f8baf6b22fc9e3c5f1024b063ac080a7b2234cf8aee8f6c47bbf4fd3ac"
                "e7240290bef16c0b3f7f3cdd64ce3ab5912cf6e32f39ab188358afcccd80"
                "81024100e4b49ef50f765d3b24dde01aceaaf130f2c76670a91a61ae08af"
                "497b4a82be6dee8fcdd5e3f7ba1cfb1f0c926b88f88c92bfab137fba2285"
                "227b83c342ff7c55024100ddabb5839c4c7f6bf3d4183231f005b31aa58a"
                "ffdda5c79e4cce217f6bc930dbe563d480706c24e9ebfcab28a6cdefd324"
                "b77e1bf7251b709092c24ff501fd91024023d4340eda3445d8cd26c14411"
                "da6fdca63c1ccd4b80a98ad52b78cc8ad8beb2842c1d280405bc2f6c1bea"
                "214a1d742ab996b35b63a82a5e470fa88dbf823cdd02401b7b57449ad30d"
                "1518249a5f56bb98294d4b6ac12ffc86940497a5a5837a6cf946262b4945"
                "26d328c11e1126380fde04c24f916dec250892db09a6d77cdba351024077"
                "62cd8f4d050da56bd591adb515d24d7ccd32cca0d05f866d583514bd7324"
                "d5f33645e8ed8b4a1cb3cc4a1d67987399f2a09f5b3fb68c88d5e5d90ac3"
                "3492d6");

const string ec_256_key =
        hex2str("308187020100301306072a8648ce3d020106082a8648ce3d030107046d30"
                "6b0201010420737c2ecd7b8d1940bf2930aa9b4ed3ff941eed09366bc032"
                "99986481f3a4d859a14403420004bf85d7720d07c25461683bc648b4778a"
                "9a14dd8a024e3bdd8c7ddd9ab2b528bbc7aa1b51f14ebbbb0bd0ce21bcc4"
                "1c6eb00083cf3376d11fd44949e0b2183bfe");

const string ec_521_key =
        hex2str("3081EE020100301006072A8648CE3D020106052B810400230481D63081D3"
                "02010104420011458C586DB5DAA92AFAB03F4FE46AA9D9C3CE9A9B7A006A"
                "8384BEC4C78E8E9D18D7D08B5BCFA0E53C75B064AD51C449BAE0258D54B9"
                "4B1E885DED08ED4FB25CE9A1818903818600040149EC11C6DF0FA122C6A9"
                "AFD9754A4FA9513A627CA329E349535A5629875A8ADFBE27DCB932C05198"
                "6377108D054C28C6F39B6F2C9AF81802F9F326B842FF2E5F3C00AB7635CF"
                "B36157FC0882D574A10D839C1A0C049DC5E0D775E2EE50671A208431BB45"
                "E78E70BEFE930DB34818EE4D5C26259F5C6B8E28A652950F9F88D7B4B2C9"
                "D9");

template <TagType tag_type, Tag tag, typename ValueT>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag> ttag, ValueT expected_value);
template <TagType tag_type, Tag tag>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag>);

struct RSA_Delete {
    void operator()(RSA* p) { RSA_free(p); }
};

X509* parse_cert_blob(const hidl_vec<uint8_t>& blob);

bool verify_chain(const hidl_vec<hidl_vec<uint8_t>>& chain);

ASN1_OCTET_STRING* get_attestation_record(X509* certificate);
bool tag_in_list(const KeyParameter& entry);

AuthorizationSet filter_tags(const AuthorizationSet& set);

string make_string(const uint8_t* data, size_t length);

template <size_t N>
std::string make_string(const uint8_t (&a)[N]) {
    return make_string(a, N);
}

bool verify_attestation_record(const string& challenge, const string& app_id,
                               AuthorizationSet expected_sw_enforced,
                               AuthorizationSet expected_tee_enforced, SecurityLevel security_level,
                               const hidl_vec<uint8_t>& attestation_cert);

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_KEYMASTER_40_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_UTILS_H_