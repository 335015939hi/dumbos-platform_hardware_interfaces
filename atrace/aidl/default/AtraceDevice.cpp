/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "atrace-impl/AtraceDevice.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace atrace {
namespace {

const char* kTracingPath = "/sys/kernel/debug/tracing/events/";

const std::vector<TracingCategory> kTracingCategories = {
        {"gfx",
         "Graphics",
         {
                 {"mdss", "*", false},
                 {"sde", "*", false},
                 {"mali_systrace", "*", false},
         }},
        {"ion",
         "ION allocation",
         {
                 {"kmem", "ion_alloc_buffer_start", false},
         }}};

std::string TraceEventToPath(const TracingEvent& event) {
    if (event.name == "*") {
        return std::string(kTracingPath) + event.group + "/enable";
    } else {
        return std::string(kTracingPath) + event.group + "/" + event.name + "/enable";
    }
}

const TracingCategory* GetCategoryByName(const std::string& name) {
    for (const TracingCategory& category : kTracingCategories) {
        if (category.name == name) {
            return &category;
        }
    }
    return nullptr;
}

}  // namespace

ndk::ScopedAStatus AtraceDevice::listCategories(std::vector<TracingCategory>* out) {
    out->clear();
    for (const TracingCategory& category : kTracingCategories) {
        out->push_back(category);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AtraceDevice::enableCategories(const std::vector<std::string>& names) {
    if (!names.size()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_ARGUMENT, "enableCategories requires at least one category");
    }

    // Check we know about all the desired categories before attempting to turn
    // them on so we don't leave tracing in an inconsistent state:
    std::vector<const TracingCategory*> categories;
    for (const std::string& name : names) {
        const TracingCategory* category = GetCategoryByName(name);
        if (!category) {
            std::string message = "Unknown category '" + name + "'";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    message.c_str());
        }
        categories.push_back(category);
    }

    for (const TracingCategory* category : categories) {
        for (const TracingEvent& event : category->events) {
            const std::string path = TraceEventToPath(event);
            if (!::android::base::WriteStringToFile("1", path)) {
                if (event.required) {
                    LOG(ERROR) << "Failed to enable tracing on: " << path;
                    AtraceDevice::disableAllCategories();
                    std::string message = "Could not enable the required event: " + event.group +
                                          "/" + event.name;
                    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE,
                                                                            message.c_str());
                }
            }
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AtraceDevice::disableAllCategories() {
    std::vector<const TracingEvent*> bad_events;

    for (const TracingCategory& category : kTracingCategories) {
        for (const TracingEvent& event : category.events) {
            const std::string path = TraceEventToPath(event);
            if (!::android::base::WriteStringToFile("0", path)) {
                if (event.required) {
                    LOG(ERROR) << "Failed to disable tracing on: " << path;
                    bad_events.push_back(&event);
                }
            }
        }
    }

    if (bad_events.size()) {
        std::string message = "Could not disable the following required events: ";
        for (const TracingEvent* event : bad_events) {
            message += event->group + "/" + event->name + ", ";
        }
        // Remove the last ", ":
        message.resize(message.size() - 2);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, message.c_str());
    } else {
        return ndk::ScopedAStatus::ok();
    }
}

}  // namespace atrace
}  // namespace hardware
}  // namespace android
}  // namespace aidl
