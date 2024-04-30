# Fuzzers for libkeymint_support

## Plugin Design Considerations
The fuzzer plugins for libkeymint_support are designed based on the understanding of the source code and try to achieve the following:

#### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on incoming data. This ensures more code paths are reached by the fuzzers.

#### Maximize utilization of input data
The plugins feed the entire input data to the module. This ensures that the plugins tolerate any kind of input (empty, huge, malformed, etc) and dont `exit()` on any input and thereby increasing the chance of identifying vulnerabilities.

## Table of contents
+ [keymint_attestation_fuzzer](#KeyMintAttestation)

# <a name="KeyMintAttestation"></a> Fuzzer for KeyMintAttestation
KeyMintAttestation supports the following parameters:
1. IssuerSubjectName(parameter name: "issuerSubjectName")
2. Index(parameter name: "idx")
3. PaddingMode(parameter name: "padding")

| Parameter| Valid Values| Configured Value|
|------------- |--------------| -------------------- |
|`issuerSubjectName`| `uint8_t` |Value obtained from FuzzedDataProvider|
|`idx`| `size_t` |Value obtained from FuzzedDataProvider|
|`padding`| `PaddingMode` |Value obtained from FuzzedDataProvider|

#### Steps to run
1. Build the fuzzer
```
$ mm -j$(nproc) keymint_attestation_fuzzer
```
2. Run on device
```
$ adb sync data
$ adb shell /data/fuzz/arm64/keymint_attestation_fuzzer
```
