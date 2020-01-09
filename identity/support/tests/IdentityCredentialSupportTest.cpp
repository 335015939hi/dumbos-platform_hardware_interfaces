/*
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iomanip>
#include <iostream>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>
#include <cppbor_parse.h>

using std::string;
using std::vector;

namespace android {
namespace hardware {
namespace identity {

TEST(IdentityCredentialSupport, encodeHex) {
    EXPECT_EQ("", support::encodeHex(vector<uint8_t>({})));
    EXPECT_EQ("01", support::encodeHex(vector<uint8_t>({1})));
    EXPECT_EQ("000102030405060708090a0b0c0d0e0f10",
              support::encodeHex(
                      vector<uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16})));
    EXPECT_EQ("0102ffe060", support::encodeHex(vector<uint8_t>({1, 2, 255, 224, 96})));
}

TEST(IdentityCredentialSupport, decodeHex) {
    vector<uint8_t> out;

    EXPECT_TRUE(support::decodeHex("", out));
    EXPECT_EQ(vector<uint8_t>({}), out);

    EXPECT_TRUE(support::decodeHex("01", out));
    EXPECT_EQ(vector<uint8_t>({1}), out);

    EXPECT_TRUE(support::decodeHex("000102030405060708090a0b0c0d0e0f10", out));
    EXPECT_EQ(vector<uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}), out);

    EXPECT_FALSE(support::decodeHex("0g", out));
    EXPECT_FALSE(support::decodeHex("0", out));
    EXPECT_FALSE(support::decodeHex("012", out));
}

TEST(IdentityCredentialSupport, CborHasBstr) {
    vector<uint8_t> bstr = {1, 2, 3};
    vector<uint8_t> bstr2 = {4, 5, 6};
    vector<uint8_t> bstr3 = {7, 8, 9};

    EXPECT_EQ(support::cborHasBstr(cppbor::Uint(42).encode(), bstr), false);
    EXPECT_EQ(support::cborHasBstr(cppbor::Nint(-42).encode(), bstr), false);
    EXPECT_EQ(support::cborHasBstr(cppbor::Tstr("not here").encode(), bstr), false);
    EXPECT_EQ(support::cborHasBstr(cppbor::Bool(true).encode(), bstr), false);
    EXPECT_EQ(support::cborHasBstr(cppbor::Bool(false).encode(), bstr), false);

    EXPECT_EQ(support::cborHasBstr(cppbor::Bstr(bstr).encode(), bstr), true);
    EXPECT_EQ(support::cborHasBstr(cppbor::Bstr(bstr).encode(), bstr2), false);

    auto arrayEncoded = cppbor::Array(cppbor::Bstr(bstr), cppbor::Bstr(bstr2)).encode();
    EXPECT_EQ(support::cborHasBstr(arrayEncoded, bstr), true);
    EXPECT_EQ(support::cborHasBstr(arrayEncoded, bstr2), true);
    EXPECT_EQ(support::cborHasBstr(arrayEncoded, bstr3), false);

    auto mapEncoded =
            cppbor::Map().add("foo", cppbor::Bstr(bstr)).add("foo1", cppbor::Bstr(bstr2)).encode();
    EXPECT_EQ(support::cborHasBstr(mapEncoded, bstr), true);
    EXPECT_EQ(support::cborHasBstr(mapEncoded, bstr2), true);
    EXPECT_EQ(support::cborHasBstr(mapEncoded, bstr3), false);

    auto mapEncodedValueAsKey =
            cppbor::Map().add(cppbor::Bstr(bstr), "foo").add(cppbor::Bstr(bstr2), "foo1").encode();
    EXPECT_EQ(support::cborHasBstr(mapEncodedValueAsKey, bstr), true);
    EXPECT_EQ(support::cborHasBstr(mapEncodedValueAsKey, bstr2), true);
    EXPECT_EQ(support::cborHasBstr(mapEncodedValueAsKey, bstr3), false);
}

TEST(IdentityCredentialSupport, CborPrettyPrint) {
    string out;

    support::cborPrettyPrint(cppbor::Tstr("Some text").encode(), out);
    EXPECT_EQ("'Some text'", out);

    support::cborPrettyPrint(cppbor::Tstr("").encode(), out);
    EXPECT_EQ("''", out);

    support::cborPrettyPrint(cppbor::Bstr(vector<uint8_t>({1, 0, 2, 240, 255, 64})).encode(), out);
    EXPECT_EQ("{0x01, 0x00, 0x02, 0xf0, 0xff, 0x40}", out);

    support::cborPrettyPrint(cppbor::Bstr(vector<uint8_t>()).encode(), out);
    EXPECT_EQ("{}", out);

    support::cborPrettyPrint(cppbor::Bool(true).encode(), out);
    EXPECT_EQ("true", out);

    support::cborPrettyPrint(cppbor::Bool(false).encode(), out);
    EXPECT_EQ("false", out);

    support::cborPrettyPrint(cppbor::Uint(42).encode(), out);
    EXPECT_EQ("42", out);

    support::cborPrettyPrint(cppbor::Uint(std::numeric_limits<int64_t>::max()).encode(), out);
    EXPECT_EQ("9223372036854775807", out);  // 0x7fff ffff ffff ffff

    support::cborPrettyPrint(cppbor::Nint(-42).encode(), out);
    EXPECT_EQ("-42", out);

    support::cborPrettyPrint(cppbor::Nint(std::numeric_limits<int64_t>::min()).encode(), out);
    EXPECT_EQ("-9223372036854775808", out);  // -0x8000 0000 0000 0000
}

TEST(IdentityCredentialSupport, CborPrettyPrintCompound) {
    string out;

    cppbor::Array array = cppbor::Array("foo", "bar", "baz");
    support::cborPrettyPrint(array.encode(), out);
    EXPECT_EQ("['foo', 'bar', 'baz', ]", out);

    cppbor::Map map = cppbor::Map().add("foo", 42).add("bar", 43).add("baz", 44);
    support::cborPrettyPrint(map.encode(), out);
    EXPECT_EQ(
            "{\n"
            "  'foo' : 42,\n"
            "  'bar' : 43,\n"
            "  'baz' : 44,\n"
            "}",
            out);

    cppbor::Array array2 = cppbor::Array(cppbor::Tstr("Some text"), cppbor::Nint(-42));
    support::cborPrettyPrint(array2.encode(), out);
    EXPECT_EQ("['Some text', -42, ]", out);

    cppbor::Map map2 = cppbor::Map().add(42, "foo").add(43, "bar").add(44, "baz");
    support::cborPrettyPrint(map2.encode(), out);
    EXPECT_EQ(
            "{\n"
            "  42 : 'foo',\n"
            "  43 : 'bar',\n"
            "  44 : 'baz',\n"
            "}",
            out);

    cppbor::Array deeplyNestedArrays =
            cppbor::Array(cppbor::Array(cppbor::Array("a", "b", "c")),
                          cppbor::Array(cppbor::Array("d", "e", cppbor::Array("f", "g"))));
    support::cborPrettyPrint(deeplyNestedArrays.encode(), out);
    EXPECT_EQ(
            "[\n"
            "  ['a', 'b', 'c', ],\n"
            "  [\n    'd',\n"
            "    'e',\n"
            "    ['f', 'g', ],\n"
            "  ],\n"
            "]",
            out);

    support::cborPrettyPrint(
            cppbor::Array(
                    cppbor::Bstr(vector<uint8_t>{10, 11}), cppbor::Tstr("foo"), cppbor::Uint(42),
                    std::move(array), std::move(map),
                    (cppbor::Map().add("deep1", std::move(array2)).add("deep2", std::move(map2))))
                    .encode(),
            out);
    EXPECT_EQ(
            "[\n"
            "  {0x0a, 0x0b},\n"
            "  'foo',\n"
            "  42,\n"
            "  ['foo', 'bar', 'baz', ],\n"
            "  {\n"
            "    'foo' : 42,\n"
            "    'bar' : 43,\n"
            "    'baz' : 44,\n"
            "  },\n"
            "  {\n"
            "    'deep1' : ['Some text', -42, ],\n"
            "    'deep2' : {\n"
            "      42 : 'foo',\n"
            "      43 : 'bar',\n"
            "      44 : 'baz',\n"
            "    },\n"
            "  },\n"
            "]",
            out);
}

TEST(IdentityCredentialSupport, Signatures) {
    vector<uint8_t> data = {1, 2, 3};

    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> signature;
    ASSERT_TRUE(support::signEcDsa(privKey, data, signature));
    ASSERT_TRUE(support::checkEcDsaSignature(support::sha256(data), signature, pubKey));

    // Manipulate the signature, check that verification fails.
    vector<uint8_t> modifiedSignature = signature;
    modifiedSignature[0] ^= 0xff;
    ASSERT_FALSE(support::checkEcDsaSignature(support::sha256(data), modifiedSignature, pubKey));

    // Manipulate the data being checked, check that verification fails.
    vector<uint8_t> modifiedDigest = support::sha256(data);
    modifiedDigest[0] ^= 0xff;
    ASSERT_FALSE(support::checkEcDsaSignature(modifiedDigest, signature, pubKey));
}

string replaceLine(const string& str, ssize_t lineNumber, const string& replacement) {
    vector<string> lines;
    std::istringstream f(str);
    string s;
    while (std::getline(f, s, '\n')) {
        lines.push_back(s);
    }

    size_t numLines = lines.size();
    if (lineNumber < 0) {
        lineNumber = numLines - (-lineNumber);
    }

    string ret;
    size_t n = 0;
    for (const string& line : lines) {
        if (n == lineNumber) {
            ret += replacement + "\n";
        } else {
            ret += line + "\n";
        }
        n++;
    }
    return ret;
}

TEST(IdentityCredentialSupport, CoseSignatures) {
    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> data = {1, 2, 3};
    vector<uint8_t> coseSign1;
    ASSERT_TRUE(support::coseSignEcDsa(privKey, data, {} /* additionalData */, {} /* x5chain */,
                                       coseSign1));
    ASSERT_TRUE(support::coseCheckEcDsaSignature(coseSign1, {} /* additionalData */, pubKey));

    vector<uint8_t> payload;
    ASSERT_TRUE(support::coseSignGetPayload(coseSign1, payload));
    ASSERT_EQ(data, payload);

    // Finally, check that |coseSign1| are the bytes of a valid COSE_Sign1 message
    string out;
    support::cborPrettyPrint(coseSign1, out);
    out = replaceLine(out, -2, "  [] // Signature Removed");
    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x26},\n"  // Bytes of {1:-7} 1 is 'alg' label and -7 is "ECDSA 256"
            "  {},\n"
            "  {0x01, 0x02, 0x03},\n"
            "  [] // Signature Removed\n"
            "]\n",
            out);
}

TEST(IdentityCredentialSupport, CoseSignaturesAdditionalData) {
    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> additionalData = {1, 2, 3};
    vector<uint8_t> coseSign1;
    ASSERT_TRUE(support::coseSignEcDsa(privKey, {} /* data */, additionalData, {} /* x5chain */,
                                       coseSign1));
    ASSERT_TRUE(support::coseCheckEcDsaSignature(coseSign1, additionalData, pubKey));

    vector<uint8_t> payload = {1, 2, 3, 4, 5};
    ASSERT_TRUE(support::coseSignGetPayload(coseSign1, payload));
    ASSERT_EQ(0, payload.size());

    // Finally, check that |coseSign1| are the bytes of a valid COSE_Sign1 message
    string out;
    support::cborPrettyPrint(coseSign1, out);
    out = replaceLine(out, -2, "  [] // Signature Removed");
    EXPECT_EQ(
            "[\n"
            "  {0xa1, 0x01, 0x26},\n"  // Bytes of {1:-7} 1 is 'alg' label and -7 is "ECDSA 256"
            "  {},\n"
            "  null,\n"
            "  [] // Signature Removed\n"
            "]\n",
            out);
}

vector<uint8_t> generateCertChain(size_t numCerts) {
    vector<vector<uint8_t>> certs;

    for (size_t n = 0; n < numCerts; n++) {
        vector<uint8_t> keyPair;
        support::createEcKeyPair(keyPair);
        vector<uint8_t> privKey;
        support::ecKeyPairGetPrivateKey(keyPair, privKey);
        vector<uint8_t> pubKey;
        support::ecKeyPairGetPublicKey(keyPair, pubKey);

        vector<uint8_t> cert;
        support::ecPublicKeyGenerateCertificate(pubKey, privKey, "0001", "someIssuer",
                                                "someSubject", 0, 0, cert);
        certs.push_back(cert);
    }
    return support::certificateChainJoin(certs);
}

TEST(IdentityCredentialSupport, CoseSignaturesX5ChainWithSingleCert) {
    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> certChain = generateCertChain(1);
    vector<vector<uint8_t>> splitCerts;
    ASSERT_TRUE(support::certificateChainSplit(certChain, splitCerts));
    ASSERT_EQ(1, splitCerts.size());

    vector<uint8_t> additionalData = {1, 2, 3};
    vector<uint8_t> coseSign1;
    ASSERT_TRUE(
            support::coseSignEcDsa(privKey, {} /* data */, additionalData, certChain, coseSign1));
    ASSERT_TRUE(support::coseCheckEcDsaSignature(coseSign1, additionalData, pubKey));

    vector<uint8_t> payload = {1, 2, 3, 4, 5};
    ASSERT_TRUE(support::coseSignGetPayload(coseSign1, payload));
    ASSERT_EQ(0, payload.size());

    vector<uint8_t> certsRecovered;
    ASSERT_TRUE(support::coseSignGetX5Chain(coseSign1, certsRecovered));
    EXPECT_EQ(certsRecovered, certChain);
}

TEST(IdentityCredentialSupport, CoseSignaturesX5ChainWithMultipleCerts) {
    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> certChain = generateCertChain(5);
    vector<vector<uint8_t>> splitCerts;
    ASSERT_TRUE(support::certificateChainSplit(certChain, splitCerts));
    ASSERT_EQ(5, splitCerts.size());

    vector<uint8_t> additionalData = {1, 2, 3};
    vector<uint8_t> coseSign1;
    ASSERT_TRUE(
            support::coseSignEcDsa(privKey, {} /* data */, additionalData, certChain, coseSign1));
    ASSERT_TRUE(support::coseCheckEcDsaSignature(coseSign1, additionalData, pubKey));

    vector<uint8_t> payload = {1, 2, 3, 4, 5};
    ASSERT_TRUE(support::coseSignGetPayload(coseSign1, payload));
    ASSERT_EQ(0, payload.size());

    vector<uint8_t> certsRecovered;
    ASSERT_TRUE(support::coseSignGetX5Chain(coseSign1, certsRecovered));
    EXPECT_EQ(certsRecovered, certChain);
}

TEST(IdentityCredentialSupport, CertificateChain) {
    vector<uint8_t> keyPair;
    ASSERT_TRUE(support::createEcKeyPair(keyPair));
    vector<uint8_t> privKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(keyPair, privKey));
    vector<uint8_t> pubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(keyPair, pubKey));

    vector<uint8_t> cert;
    ASSERT_TRUE(support::ecPublicKeyGenerateCertificate(pubKey, privKey, "0001", "someIssuer",
                                                        "someSubject", 0, 0, cert));

    vector<uint8_t> extractedPubKey;
    ASSERT_TRUE(support::certificateChainGetTopMostKey(cert, extractedPubKey));
    ASSERT_EQ(pubKey, extractedPubKey);

    // We expect to the chain returned by ecPublicKeyGenerateCertificate() to only have a
    // single element
    vector<vector<uint8_t>> splitCerts;
    ASSERT_TRUE(support::certificateChainSplit(cert, splitCerts));
    ASSERT_EQ(1, splitCerts.size());
    ASSERT_EQ(splitCerts[0], cert);

    vector<uint8_t> otherKeyPair;
    ASSERT_TRUE(support::createEcKeyPair(otherKeyPair));
    vector<uint8_t> otherPrivKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(otherKeyPair, otherPrivKey));
    vector<uint8_t> otherPubKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(otherKeyPair, otherPubKey));
    vector<uint8_t> otherCert;
    ASSERT_TRUE(support::ecPublicKeyGenerateCertificate(otherPubKey, privKey, "0001", "someIssuer",
                                                        "someSubject", 0, 0, otherCert));

    // Now both cert and otherCert are two distinct certificates. Let's make a
    // chain and check that certificateChainSplit() works as expected.
    ASSERT_NE(cert, otherCert);
    const vector<vector<uint8_t>> certs2 = {cert, otherCert};
    vector<uint8_t> certs2combined = support::certificateChainJoin(certs2);
    ASSERT_EQ(certs2combined.size(), cert.size() + otherCert.size());
    vector<vector<uint8_t>> splitCerts2;
    ASSERT_TRUE(support::certificateChainSplit(certs2combined, splitCerts2));
    ASSERT_EQ(certs2, splitCerts2);
}

}  // namespace identity
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
