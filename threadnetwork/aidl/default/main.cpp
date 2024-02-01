/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/threadnetwork/IThreadChip.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <getopt.h>
#include <utils/Log.h>

#include "log.hpp"
#include "service.hpp"
#include "thread_chip.hpp"

using aidl::android::hardware::threadnetwork::IThreadChip;
using aidl::android::hardware::threadnetwork::ThreadChip;

static void dumpUsage(const char* aProgramName) {
    logCrit("Syntax:\n"
            "    %s [Options] RadioURL [RadioURL]\n"
            "Options:\n"
            "    -d  --debug-level   Debug level of logging\n"
            "                        (3: Debug, 4:Info, 5:Warn, 6: Error, 7: Crit)\n",
            aProgramName);
}

static std::shared_ptr<ThreadChip> createThreadChipService(int identifier, char* url) {
    const std::string serviceName(std::string() + IThreadChip::descriptor + "/chip" +
                                  std::to_string(identifier));

    logInfo("ServiceName: %s", serviceName.c_str());
    logInfo("Url: %s", url);

    auto threadChip = ndk::SharedRefBase::make<ThreadChip>(url);
    CHECK_NE(threadChip, nullptr);

    auto status = AServiceManager_addService(threadChip->asBinder().get(), serviceName.c_str());
    CHECK_EQ(status, STATUS_OK);

    return threadChip;
}

int main(int argc, char* argv[]) {
    constexpr struct option kOptions[] = {{"debug-level", required_argument, NULL, 'd'},
                                          {0, 0, 0, 0}};
    std::vector<std::shared_ptr<ThreadChip>> threadChips;
    aidl::android::hardware::threadnetwork::Service service;
    int identifier = 0;

    optind = 1;

    while (true) {
        int index = 0;
        int option = getopt_long(argc, argv, "d:", kOptions, &index);

        if (option == -1) {
            break;
        }

        switch (option) {
            case 'd':
                setLogLevel(static_cast<android_LogPriority>(atoi(optarg)));
                break;
            default:
                dumpUsage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        logCrit("No radio URL is specified!");
        dumpUsage(argv[0]);
        return EXIT_FAILURE;
    }

    for (; optind < argc; optind++) {
        auto threadChip = createThreadChipService(identifier++, argv[optind]);
        threadChips.push_back(std::move(threadChip));
    }

    logInfo("Thread Network HAL is running");

    service.startLoop();
    return EXIT_FAILURE;  // should not reach
}
