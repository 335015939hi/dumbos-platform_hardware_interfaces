/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <fstream>
#include "keymaster4_common.h"

namespace android::hardware::keymaster::V4_0::fuzzer {

class KeyMaster4AuthSetFuzzer {
  public:
    ~KeyMaster4AuthSetFuzzer() {
        if (mFdp) {
            delete mFdp;
        }
    }
    void process(const uint8_t* data, size_t size);

  private:
    void invokeAuthSetAPIs();
    FuzzedDataProvider* mFdp = nullptr;
};

/**
 * @brief invokeAuthSetAPIs() function aims at calling functions of authorization_set.cpp
 * in order to get a good coverage for libkeymaster4support.
 */
void KeyMaster4AuthSetFuzzer::invokeAuthSetAPIs() {
    AuthorizationSet authSet = createAuthorizationSet(mFdp);
    while (mFdp->remaining_bytes() > 0) {
        uint32_t action = mFdp->ConsumeIntegralInRange<uint32_t>(0, 5);
        switch (action) {
            case 0: {
                authSet.Sort();
            } break;
            case 1: {
                authSet.Deduplicate();
            } break;
            case 2: {
                authSet.Union(createAuthorizationSet(mFdp));
            } break;
            case 3: {
                authSet.Subtract(createAuthorizationSet(mFdp));
            } break;
            case 4: {
                std::filebuf fbOut;
                fbOut.open("/dev/zero", std::ios::out);
                std::ostream out(&fbOut);
                authSet.Serialize(&out);
            } break;
            case 5: {
                std::filebuf fbIn;
                fbIn.open("/dev/zero", std::ios::in);
                std::istream in(&fbIn);
                authSet.Deserialize(&in);
            } break;
            default:
                break;
        };
    }
}

void KeyMaster4AuthSetFuzzer::process(const uint8_t* data, size_t size) {
    mFdp = new FuzzedDataProvider(data, size);
    invokeAuthSetAPIs();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMaster4AuthSetFuzzer km4AuthSetFuzzer;
    km4AuthSetFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::keymaster::V4_0::fuzzer
