/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "TestDirectAccessTests"

#include <future>
#include <map>
#include <utility>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <aidl/android/hardware/identity/Certificate.h>
#include <aidl/android/security/identity/direct_access/BnMDocCredential.h>
#include <aidl/android/security/identity/direct_access/BnMDocStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <cppbor.h>
#include <cppbor_parse.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>

namespace aidl::android::security::identity::direct_access::test {

using ::aidl::android::hardware::identity::Certificate;
using ::aidl::android::security::identity::direct_access::IMDocCredential;
using ::aidl::android::security::identity::direct_access::IMDocStore;
using ::aidl::android::security::identity::direct_access::MDocPresentationPackage;
using std::make_pair;
using std::map;
using std::optional;
using std::pair;
using std::string;
using std::tie;
using std::vector;
using Status = ::ndk::ScopedAStatus;

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

string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

/**
 * CredentialData = {
 *      "docType": "org.iso.18013.5.1.mdl",
 *      "digestIdMapping": DigestIdMapping,
 *      "issuerAuth" : IssuerAuth,
 *      "readerAccess" : ReaderAccess
 * }
 *
 * IssuerAuth = COSE_Sign1  ; payload as mentioned in
 * mDL specification.
 *
 * DigestIdMapping = {
 *      NameSpace => [ + IssuerSignedItemBytes ]
 * }
 *
 * NameSpace = "org.iso.18013.5.1"
 *
 * ReaderAccess = [ * COSE_Key ]
 *
 * IssuerSignedItemBytes = #6.24(bstr .cbor IssuerSignedItem)
 *
 * IssuerSignedItem = {
 *   "digestID" : uint, ; Digest ID for issuer data authentication
 *   "random" : bstr, ; Random value for issuer data authentication
 *   "elementIdentifier" : DataElementIdentifier, ; Data element identifier
 *   "elementValue" : DataElementValue ; Data element value
 * }
 *
 * DataElementIdentifier: tstr
 *
 */
std::string credential_data = hex2str(
        "A4"                                          // {
        "67"                                          // tstr of length 7
        "646F6354797065"                              // "docType"
        "75"                                          // tstr of length 21
        "6F72672E69736F2E31383031332E352E312E6D444C"  // "org.iso.18013.5.1.mDL"
        "6F"                                          // tstr of length 15
        "64696765737449644D617070696E67"              // "digestIdMapping":
        "A1"                                          // {
        "71"                                          // tstr of length 17
        "6F72672E69736F2E31383031332E352E31"          // "org.iso.18013.5.1"
        "85"                                          // [ Array of issuserSignedItems
        "D818"                                        // tag(24)
        "5863"                                        // bstr of length 99
        "A4686469676573744944006672616E646F6D58208798645B20EA200E19FFABAC92624BEE6AEC63ACEEDECFB1"
        "B80077D22BFC20E971656C656D656E744964656E7469666965726B66616D696C795F6E616D656C656C656D65"
        "6E7456616C756563446F65"  // "family_name"
        "D818"                    // tag(24)
        "586C"                    // bstr of length 108
        "A4686469676573744944036672616E646F6D5820B23F627E8999C706DF0C0A4ED98AD74AF988AF619B4BB078"
        "B89058553F44615D71656C656D656E744964656E7469666965726A69737375655F646174656C656C656D656E"
        "7456616C7565D903EC6A323032342D31302D3230"  // "issuer_date"
        "D818"                                      // tag(24)
        "586D"                                      // bstr of length 109
        "A4686469676573744944076672616E646F6D582026052A42E5880557A806C1459AF3FB7EB505D3781566329D"
        "0B604B845B5F9E6871656C656D656E744964656E7469666965726F646F63756D656E745F6E756D6265726C65"
        "6C656D656E7456616C756569313233343536373839"  // "document_number"
        "D818"                                        // tag(24)
        "590471"                                      // bstr of length 1137
        "A4686469676573744944086672616E646F6D5820D094DAD764A2EB9DEB5210E9D899643EFBD1D069CC311D32"
        "95516CA0B024412D71656C656D656E744964656E74696669657268706F7274726169746C656C656D656E7456"
        "616C7565590412FFD8FFE000104A46494600010101009000900000FFDB004300130D0E110E0C13110F111514"
        "13171D301F1D1A1A1D3A2A2C2330453D4947443D43414C566D5D4C51685241435F82606871757B7C7B4A5C86"
        "9085778F6D787B76FFDB0043011415151D191D381F1F38764F434F7676767676767676767676767676767676"
        "767676767676767676767676767676767676767676767676767676767676767676FFC0001108001800640301"
        "2200021101031101FFC4001B00000301000301000000000000000000000005060401020307FFC40032100001"
        "0303030205020309000000000000010203040005110612211331141551617122410781A1163542527391B2C1"
        "F1FFC4001501010100000000000000000000000000000001FFC4001A11010101000301000000000000000000"
        "0000014111213161FFDA000C03010002110311003F00A5BBDE22DA2329C7D692BC7D0D03F52CFB0FF75E7A7E"
        "F3E7709723A1D0DAE146DDFBB3C039CE07AD2BD47A7E32DBB8DD1D52D6EF4B284F64A480067DFB51F87FFB95"
        "FF00EB9FF14D215DE66AF089CE44B7DBDE9CB6890A2838EDDF18078F7ADD62D411EF4DB9B10A65D6B95A1473"
        "81EA0D495B933275FE6BBA75C114104A8BA410413E983DFF004F5AF5D34B4B4CDE632D0BF1FD1592BDD91C64"
        "11F3934C2FA6AF6B54975D106DCF4A65AE56E856001EBC03C7CE29DD9EEF1EF10FC447DC9DA76AD2AEE93537"
        "A1BA7E4F70DD8EFF0057C6DFFB5E1A19854A83758E54528750946EC6704850CD037BCEB08B6D7D2CC76D3317"
        "FC7B5CC04FB6707269C5C6E0C5B60AE549242123B0E493F602A075559E359970D98DB89525456B51C951C8AF"
        "A13EA8E98E3C596836783D5C63F5A61A99FDB7290875DB4BE88AB384BBBBBFC7183FDEAA633E8951DB7DA396"
        "DC48524FB1A8BD611A5AA2A2432F30AB420A7A6D3240C718CF031FA9EF4C9AD550205AA02951DF4A1D6C8421"
        "B015B769DB8C9229837EA2BE8B1B0D39D0EBA9C51484EFDB8C0EFD8D258DAF3C449699F2EDBD4584E7AF9C64"
        "E3F96B9BEB28D4AC40931E6478C8E76A24A825449501D867D2B1DCDEBAE99B9C752AE4ECD6DDE4A179C1C1E4"
        "60938F9149EF655E515C03919A289CB3DCA278FB7BF177F4FAA829DD8CE3F2AC9A7ECDE490971FAFD7DCE15E"
        "ED9B71C018C64FA514514B24E8E4F8C5C9B75C1E82579DC1233DFEC08238F6ADD62D391ACC1C5256A79E706D"
        "52D431C7A0145140B9FD149EB3A60DC5E88CBBC2DA092411E9DC71F39A7766B447B344E847DCAC9DCB5ABBA8"
        "D145061D43A6FCF1E65CF15D0E90231D3DD9CFE62995C6DCC5CA12A2C904A15F71DD27D451453E09D1A21450"
        "961CBB3EA8A956433B781F1CE33DFED54F0E2B50A2B71D84ED6DB18028A28175F74FC6BDA105C529A791C25C"
        "4F3C7A11F71586268F4A66B726E33DE9EA6F1B52B181C760724E47B514520A5A28A283FFD9"  // "potrait"
        "D818"                                                                        // tag(24)
        "58FF"  // bstr of length 255
        "A4686469676573744944096672616E646F6D58204599F81BEAA2B20BD0FFCC9AA03A6F985BEFAB3F6BEAFFA4"
        "1E6354CDB2AB2CE471656C656D656E744964656E7469666965727264726976696E675F70726976696C656765"
        "736C656C656D656E7456616C756582A37576656869636C655F63617465676F72795F636F646561416A697373"
        "75655F64617465D903EC6A323031382D30382D30396B6578706972795F64617465D903EC6A323032342D3130"
        "2D3230A37576656869636C655F63617465676F72795F636F646561426A69737375655F64617465D903EC6A32"
        "3031372D30322D32336B6578706972795F64617465D903EC6A323032342D31"
        "302D3230"  // "driving_privileges"
        // ] End of issuerSignedItems array
        // } End of digestIdMapping
        "6A"                    // tstr of length 10
        "69737375657241757468"  // "issuerAuth"
        "84"                    // [  CoseSign1
        "44"                    // bstr of length 4
        "43A10126"              // protected params
        "A1"                    // { unprotected Params
        "1821"                  // 33
        "5901F3"                // bytes of 499
        "308201EF30820195A00302010202143C4416EED784F3B413E48F56F075ABFA6D87EB84300A06082A8648CE3D"
        "04030230233114301206035504030C0B75746F7069612069616361310B3009060355040613025553301E170D"
        "3230313030313030303030305A170D3231313030313030303030305A30213112301006035504030C0975746F"
        "706961206473310B30090603550406130255533059301306072A8648CE3D020106082A8648CE3D0301070342"
        "0004ACE7AB7340E5D9648C5A72A9A6F56745C7AAD436A03A43EFEA77B5FA7B88F0197D57D8983E1B37D3A539"
        "F4D588365E38CBBF5B94D68C547B5BC8731DCD2F146BA381A83081A5301E0603551D12041730158113657861"
        "6D706C65406578616D706C652E636F6D301C0603551D1F041530133011A00FA00D820B6578616D706C652E63"
        "6F6D301D0603551D0E0416041414E29017A6C35621FFC7A686B7B72DB06CD12351301F0603551D2304183016"
        "801454FA2383A04C28E0D930792261C80C4881D2C00B300E0603551D0F0101FF04040302078030150603551D"
        "250101FF040B3009060728818C5D050102300A06082A8648CE3D040302034800304502210097717AB9016740"
        "C8D7BCDAA494A62C053BBDECCE1383C1ACA72AD08DBC04CBB202203BAD859C13A63C6D1AD67D814D43E2425C"
        "AF90D422422C04A8EE0304C0D3A68D"
        // } End of unprotected params
        "59039F"  // bstr of length 927
        "D818A66776657273696F6E63312E306F646967657374416C676F726974686D675348412D3235366C76616C75"
        "6544696765737473A2716F72672E69736F2E31383031332E352E31AD00582075167333B47B6C2BFB86ECCC1F"
        "438CF57AF055371AC55E1E359E20F254ADCEBF01582067E539D6139EBD131AEF441B445645DD831B2B375B39"
        "0CA5EF6279B205ED45710258203394372DDB78053F36D5D869780E61EDA313D44A392092AD8E0527A2FBFE55"
        "AE0358202E35AD3C4E514BB67B1A9DB51CE74E4CB9B7146E41AC52DAC9CE86B8613DB555045820EA5C3304BB"
        "7C4A8DCB51C4C13B65264F845541341342093CCA786E058FAC2D59055820FAE487F68B7A0E87A749774E56E9"
        "E1DC3A8EC7B77E490D21F0E1D3475661AA1D0658207D83E507AE77DB815DE4D803B88555D0511D894C897439"
        "F5774056416A1C7533075820F0549A145F1CF75CBEEFFA881D4857DD438D627CF32174B1731C4C38E12CA936"
        "085820B68C8AFCB2AAF7C581411D2877DEF155BE2EB121A42BC9BA5B7312377E068F660958200B3587D1DD0C"
        "2A07A35BFB120D99A0ABFB5DF56865BB7FA15CC8B56A66DF6E0C0A5820C98A170CF36E11ABB724E98A75A534"
        "3DFA2B6ED3DF2ECFBB8EF2EE55DD41C8810B5820B57DD036782F7B14C6A30FAAAAE6CCD5054CE88BDFA51A01"
        "6BA75EDA1EDEA9480C5820651F8736B18480FE252A03224EA087B5D10CA5485146C67C74AC4EC3112D4C3A74"
        "6F72672E69736F2E31383031332E352E312E5553A4005820D80B83D25173C484C5640610FF1A31C949C1D934"
        "BF4CF7F18D5223B15DD4F21C0158204D80E1E2E4FB246D97895427CE7000BB59BB24C8CD003ECF94BF35BBD2"
        "917E340258208B331F3B685BCA372E85351A25C9484AB7AFCDF0D2233105511F778D98C2F544035820C343AF"
        "1BD1690715439161ABA73702C474ABF992B20C9FB55C36A336EBE01A876D6465766963654B6579496E666FA1"
        "696465766963654B6579A40102200121582096313D6C63E24E3372742BFDB1A33BA2C897DCD68AB8C753E4FB"
        "D48DCA6B7F9A2258201FB3269EDD418857DE1B39A4E4A44B92FA484CAA722C228288F01D0C03A2C3D667646F"
        "6354797065756F72672E69736F2E31383031332E352E312E6D444C6C76616C6964697479496E666FA3667369"
        "676E6564C074323032302D31302D30315431333A33303A30325A6976616C696446726F6DC074323032302D31"
        "302D30315431333A33303A30325A6A76616C6964556E74696CC074323032312D31302D30315431333A33303A"
        "30325A"  // payload
        "5840"    // bstr of length 64
        "59E64205DF1E2F708DD6DB0847AED79FC7C0201D80FA55BADCAF2E1BCF5902E1E5A62E4832044B890AD85AA5"
        "3F129134775D733754D7CB7A413766AEFF13CB2E"  // signature
        "6C"                                        // tstr of length 12
        "726561646572416363657373"                  // "readerAccess"
        "80"                                        // [] Array of 0.
                                                    // } End of the CredentialData
);

#define INSTANTIATE_IDENTITY_DIRECT_AIDL_TEST(name)                                        \
    INSTANTIATE_TEST_SUITE_P(                                                              \
            PerInstance, name,                                                             \
            testing::ValuesIn(::android::getAidlHalInstanceNames(IMDocStore::descriptor)), \
            ::android::PrintInstanceNameToString);                                         \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(name);

class TestDireactAccessTests : public testing::TestWithParam<string> {
  public:
    void SetUp() {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        mDocStore_ = IMDocStore::fromBinder(binder);
        ASSERT_NE(mDocStore_, nullptr);
    }

    void createMDocCredential(int32_t slot) {
        string sChallenge = "test_challenge";
        vector<uint8_t> challenge;
        std::vector<Certificate> out_certificate;
        challenge.assign(sChallenge.begin(), sChallenge.end());

        ASSERT_TRUE(
                mDocStore_->createMDocCredential(slot, true, challenge, &out_certificate).isOk());
        ASSERT_GE(out_certificate.size(), 1);
    }

  protected:
    std::shared_ptr<IMDocStore> mDocStore_;
};

TEST_P(TestDireactAccessTests, SimpleDirectAccessFlowTest) {
    int32_t slotNu = 0, totalNuSlot, usageCnt;
    int64_t maxCredDataSize;
    MDocPresentationPackage mPackage;

    ASSERT_TRUE(mDocStore_->getNumberOfCredentialSlots(&totalNuSlot).isOk());
    ASSERT_GE(totalNuSlot, 1);
    ASSERT_TRUE(mDocStore_->getMaximumCredentialDataSize(&maxCredDataSize).isOk());
    ASSERT_LE(maxCredDataSize, 32768);

    createMDocCredential(slotNu);

    std::shared_ptr<IMDocCredential> mDocCredential;
    ASSERT_TRUE(mDocStore_->lookupMDocCredential(slotNu, &mDocCredential).isOk());

    // generate presentation package
    ASSERT_TRUE(mDocCredential->presentationPackageGenerate(86400000 /*1 day */, &mPackage).isOk());
    ASSERT_NE(mPackage.encryptedData.size(), 0);
    // TODO verify the certificate and it should be signed by the Credential Key.

    vector<uint8_t> credDataVec(credential_data.begin(), credential_data.end());

    MDocPresentationPackage mPackageTemp;
    ASSERT_TRUE(mDocCredential->presentationPackageSetData(mPackage, credDataVec, &mPackageTemp)
                        .isOk());

    ASSERT_NE(mPackage.encryptedData.size(), 0);
    // TODO verify the certificate

    // set mPackageTemp as current data
    ASSERT_TRUE(mDocCredential->currentPresentationPackageSet(mPackageTemp).isOk());

    // get current signing certificate
    Certificate certificate;
    ASSERT_TRUE(mDocCredential->currentPresentationPackageGet(&certificate).isOk());

    // verify the certificate
    // ASSERT_NE(certificate, mPackage.signingKeyCertificate);
    ASSERT_EQ(0, memcmp(certificate.encodedCertificate.data(),
                        mPackage.signingKeyCertificate.encodedCertificate.data(),
                        certificate.encodedCertificate.size()));

    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_EQ(usageCnt, 0);

    // TODO request and response implementation
    std::vector<uint8_t> in_deviceRequestCbor, out_DeviceResponse;
    ASSERT_TRUE(
            mDocCredential->simulatePresentation(in_deviceRequestCbor, &out_DeviceResponse).isOk());

    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_NE(usageCnt, 1);

    // reset the usage count
    ASSERT_TRUE(mDocCredential->currentPresentationPackageClear().isOk());
    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_EQ(usageCnt, 0);

    // delete MDocCredential
    ASSERT_TRUE(mDocStore_->deleteMDocCredential(slotNu).isOk());
}

INSTANTIATE_IDENTITY_DIRECT_AIDL_TEST(TestDireactAccessTests);

}  // namespace aidl::android::security::identity::direct_access::test
