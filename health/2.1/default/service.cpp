/*
 * Copyright (C) 2019 The Android Open Source Project
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
#define LOG_TAG "android.hardware.health@2.1-service"

#include <health2impl/Health.h>
#include <health2impl/BinderHealthLoop.h>

using ::android::sp;
using ::android::hardware::health::V2_1::implementation::BinderHealthLoop;
using ::android::hardware::health::V2_1::implementation::Health;

int main(int /* argc */, char* /* argv */[]) {
    sp<Health> service = new Health("default");
    BinderHealthLoop loop{service.get()};
    return loop.StartLoop();
}
