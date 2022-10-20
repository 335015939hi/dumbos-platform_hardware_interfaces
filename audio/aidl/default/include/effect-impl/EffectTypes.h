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

#pragma once
#include <ostream>
#include <string>

namespace aidl::android::hardware::audio::effect {

enum class RetCode {
    SUCCESS,
    ERROR_ILLEGAL_PARAMETER, /* Illegal parameter */
    ERROR_THREAD,            /* Effect thread error */
    ERROR_NULL_POINTER,      /* NULL pointer */
    ERROR_ALIGNMENT_ERROR,   /* Memory alignment error */
    ERROR_BLOCK_SIZE_EXCEED  /* Maximum block size exceeded */
};

inline std::ostream& operator<<(std::ostream& out, const RetCode& code) {
    switch (code) {
        case RetCode::SUCCESS:
            return out << "SUCCESS";
        case RetCode::ERROR_ILLEGAL_PARAMETER:
            return out << "ERROR_ILLEGAL_PARAMETER";
        case RetCode::ERROR_THREAD:
            return out << "ERROR_THREAD";
        case RetCode::ERROR_NULL_POINTER:
            return out << "ERROR_NULL_POINTER";
        case RetCode::ERROR_ALIGNMENT_ERROR:
            return out << "ERROR_ALIGNMENT_ERROR";
        case RetCode::ERROR_BLOCK_SIZE_EXCEED:
            return out << "ERROR_BLOCK_SIZE_EXCEED";
    }

    return out << "EnumError: " << code;
}

#define RETURN_IF_RETCODE_NOT_SUCCESS(ret_code, exception, message)                      \
    do {                                                                                 \
        const RetCode code = (ret_code);                                                 \
        if (code != RetCode::SUCCESS) {                                                  \
            LOG(ERROR) << __func__ << "return with: " << code;                           \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exception, message); \
        }                                                                                \
    } while (0)

#define RETURN_IF_ASTATUS_NOT_OK(status, exception, message)                             \
    do {                                                                                 \
        const ::ndk::ScopedAStatus curr_status = (status);                               \
        if (!curr_status.isOk()) {                                                       \
            LOG(ERROR) << __func__ << "return with status: " << curr_status << message;  \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exception, message); \
        }                                                                                \
    } while (0)

#define RETURN_IF_STATE_NOT_MATCH(state, expected_state, exception, message)               \
    do {                                                                                   \
        const auto& curr_state = (state);                                                  \
        if (curr_state != expected_state) {                                                \
            LOG(ERROR) << __func__ << "return with illegal state " << toString(curr_state) \
                       << " expecting " << expected_state;                                 \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exception, message);   \
        }                                                                                  \
    } while (0)

#define RETURN_OK_IF_STATE_NOT_MATCH(state, expected_state)                                     \
    do {                                                                                        \
        const auto& curr_state = (state);                                                       \
        if (curr_state != expected_state) {                                                     \
            LOG(INFO) << __func__ << " mismatch state " << toString(curr_state) << " expected " \
                      << expected_state;                                                        \
            return ndk::ScopedAStatus::ok();                                                    \
        }                                                                                       \
    } while (0)

#define RETURN_IF(expr, exception, message)                                              \
    do {                                                                                 \
        if (expr) {                                                                      \
            LOG(ERROR) << __func__ << " return with false expr " << #expr;               \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exception, message); \
        }                                                                                \
    } while (0)

#define RETURN_OK_IF(expr)                                                \
    do {                                                                  \
        if (expr) {                                                       \
            LOG(INFO) << __func__ << " return with false expr " << #expr; \
            return ndk::ScopedAStatus::ok();                              \
        }                                                                 \
    } while (0)

}  // namespace aidl::android::hardware::audio::effect
