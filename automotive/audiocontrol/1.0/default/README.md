# Fuzzer for android.hardware.automotive.audiocontrol@1.0-service

## Plugin Design Considerations
The fuzzer plugin for android.hardware.automotive.audiocontrol@1.0-service is
designed based on the understanding of the service and tries to achieve the following:

##### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on
incoming data. This ensures more code paths are reached by the fuzzer.

AudioControl supports the following parameters:
1. Context Number (parameter name: `contextNumber`)
2. Balance (parameter name: `balance`)

| Parameter| Valid Values| Configured Value|
|------------- |-------------| ----- |
| `contextNumber` | 0. `INVALID` 1. `MUSIC` 2. `NAVIGATION` 3. `VOICE_COMMAND`  4. `CALL_RING ` 5. `CALL` 6. `ALARM` 7. `NOTIFICATION` 7. `SYSTEM_SOUND`| Value obtained from FuzzedDataProvider|
| `balance`   | In the range `-1 to 1` | Value obtained from FuzzedDataProvider |

This also ensures that the plugin is always deterministic for any given input.

## Build

This describes steps to build audiocontrolV1.0_fuzzer binary.

### Android

#### Steps to build
Build the fuzzer
```
  $ mm -j$(nproc) audiocontrolV1.0_fuzzer
```

#### Steps to run
Create a directory CORPUS_DIR and copy some files to that folder
Push this directory to device.

To run on device
```
  $ adb sync data
  $ adb shell /data/fuzz/arm64/audiocontrolV1.0_fuzzer/audiocontrolV1.0_fuzzer CORPUS_DIR
```
To run on host
```
  $ $ANDROID_HOST_OUT/fuzz/x86_64/audiocontrolV1.0_fuzzer/audiocontrolV1.0_fuzzer CORPUS_DIR
```

## References:
 * http://llvm.org/docs/LibFuzzer.html
 * https://github.com/google/oss-fuzz
