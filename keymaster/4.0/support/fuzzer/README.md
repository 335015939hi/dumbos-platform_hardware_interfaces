# Fuzzers for libkeymaster4support

## Plugin Design Considerations
The fuzzer plugins for libkeymaster4support are designed based on the understanding of the
source code and try to achieve the following:

##### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on
incoming data. This ensures more code paths are reached by the fuzzers.

libkeymaster4support supports the following parameters:
1. Security Level (parameter name: `securityLevel`)
2. Ec Curve (parameter name: `ecCurve`)
3. Padding Mode (parameter name: `paddingMode`)
4. Digest (parameter name: `digest`)

| Parameter| Valid Values| Configured Value|
|------------- |-------------| ----- |
| `securityLevel` | 0.`SecurityLevel::SOFTWARE` 1.`SecurityLevel::TRUSTED_ENVIRONMENT` 2.`SecurityLevel::STRONGBOX`| Value obtained from FuzzedDataProvider|
| `ecCurve` | 0.`EcCurve::P_224` 1.`EcCurve::P_256` 2.`EcCurve::P_384` 3. `EcCurve::P_521`| Value obtained from FuzzedDataProvider|
| `paddingMode` | 0.`PaddingMode::NONE` 1.`PaddingMode::RSA_OAEP` 2.`PaddingMode::RSA_PSS` 3. `PaddingMode::RSA_PKCS1_1_5_ENCRYPT` 4.`PaddingMode::RSA_PKCS1_1_5_SIGN` 5.`PaddingMode::PKCS7`| Value obtained from FuzzedDataProvider|
| `digest` | 1. `Digest::NONE` 2.`Digest::MD5` 3.`Digest::SHA1` 4.`Digest::SHA_2_224` 5.`Digest::SHA_2_256` 6.`Digest::SHA_2_384`  7.`Digest::SHA_2_512`| Value obtained from FuzzedDataProvider|

This also ensures that the plugins are always deterministic for any given input.

##### Maximize utilization of input data
The plugins feed the entire input data to the module.
This ensures that the plugins tolerate any kind of input (empty, huge,
malformed, etc) and dont `exit()` on any input and thereby increasing the
chance of identifying vulnerabilities.

## Build

This describes steps to build keymaster4_attestation_fuzzer, keymaster4_authSet_fuzzer and keymaster4_utils_fuzzer binaries

### Android

#### Steps to build
Build the fuzzer
```
  $ mm -j$(nproc) keymaster4_attestation_fuzzer
  $ mm -j$(nproc) keymaster4_authSet_fuzzer
  $ mm -j$(nproc) keymaster4_utils_fuzzer
```
#### Steps to run
To run on device
```
  $ adb sync data
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_attestation_fuzzer/keymaster4_attestation_fuzzer
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_authSet_fuzzer/keymaster4_authSet_fuzzer
  $ adb shell /data/fuzz/${TARGET_ARCH}/keymaster4_utils_fuzzer/keymaster4_utils_fuzzer
```

## References:
 * http://llvm.org/docs/LibFuzzer.html
 * https://github.com/google/oss-fuzz
