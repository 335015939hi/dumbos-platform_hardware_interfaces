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

// FIXME Remove this file if you don't need to translate types in this backend.

#include "android/hardware/tv/cec/translate-cpp.h"

namespace android::h2a {

static_assert(android::hardware::tv::cec::MaxLength::MESSAGE_BODY ==
              static_cast<android::hardware::tv::cec::MaxLength>(
                      ::android::hardware::tv::cec::V1_0::MaxLength::MESSAGE_BODY));

static_assert(android::hardware::tv::cec::CecDeviceType::INACTIVE ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::INACTIVE));
static_assert(android::hardware::tv::cec::CecDeviceType::TV ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::TV));
static_assert(android::hardware::tv::cec::CecDeviceType::RECORDER ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::RECORDER));
static_assert(android::hardware::tv::cec::CecDeviceType::TUNER ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::TUNER));
static_assert(android::hardware::tv::cec::CecDeviceType::PLAYBACK ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::PLAYBACK));
static_assert(android::hardware::tv::cec::CecDeviceType::AUDIO_SYSTEM ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::AUDIO_SYSTEM));
static_assert(android::hardware::tv::cec::CecDeviceType::MAX ==
              static_cast<android::hardware::tv::cec::CecDeviceType>(
                      ::android::hardware::tv::cec::V1_0::CecDeviceType::MAX));

static_assert(android::hardware::tv::cec::AbortReason::UNRECOGNIZED_MODE ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::UNRECOGNIZED_MODE));
static_assert(android::hardware::tv::cec::AbortReason::NOT_IN_CORRECT_MODE ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::NOT_IN_CORRECT_MODE));
static_assert(android::hardware::tv::cec::AbortReason::CANNOT_PROVIDE_SOURCE ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::CANNOT_PROVIDE_SOURCE));
static_assert(android::hardware::tv::cec::AbortReason::INVALID_OPERAND ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::INVALID_OPERAND));
static_assert(android::hardware::tv::cec::AbortReason::REFUSED ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::REFUSED));
static_assert(android::hardware::tv::cec::AbortReason::UNABLE_TO_DETERMINE ==
              static_cast<android::hardware::tv::cec::AbortReason>(
                      ::android::hardware::tv::cec::V1_0::AbortReason::UNABLE_TO_DETERMINE));

static_assert(android::hardware::tv::cec::Result::SUCCESS ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::SUCCESS));
static_assert(android::hardware::tv::cec::Result::FAILURE_UNKNOWN ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::FAILURE_UNKNOWN));
static_assert(android::hardware::tv::cec::Result::FAILURE_INVALID_ARGS ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::FAILURE_INVALID_ARGS));
static_assert(android::hardware::tv::cec::Result::FAILURE_INVALID_STATE ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::FAILURE_INVALID_STATE));
static_assert(android::hardware::tv::cec::Result::FAILURE_NOT_SUPPORTED ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::FAILURE_NOT_SUPPORTED));
static_assert(android::hardware::tv::cec::Result::FAILURE_BUSY ==
              static_cast<android::hardware::tv::cec::Result>(
                      ::android::hardware::tv::cec::V1_0::Result::FAILURE_BUSY));

static_assert(android::hardware::tv::cec::SendMessageResult::SUCCESS ==
              static_cast<android::hardware::tv::cec::SendMessageResult>(
                      ::android::hardware::tv::cec::V1_0::SendMessageResult::SUCCESS));
static_assert(android::hardware::tv::cec::SendMessageResult::NACK ==
              static_cast<android::hardware::tv::cec::SendMessageResult>(
                      ::android::hardware::tv::cec::V1_0::SendMessageResult::NACK));
static_assert(android::hardware::tv::cec::SendMessageResult::BUSY ==
              static_cast<android::hardware::tv::cec::SendMessageResult>(
                      ::android::hardware::tv::cec::V1_0::SendMessageResult::BUSY));
static_assert(android::hardware::tv::cec::SendMessageResult::FAIL ==
              static_cast<android::hardware::tv::cec::SendMessageResult>(
                      ::android::hardware::tv::cec::V1_0::SendMessageResult::FAIL));

static_assert(android::hardware::tv::cec::HdmiPortType::INPUT ==
              static_cast<android::hardware::tv::cec::HdmiPortType>(
                      ::android::hardware::tv::cec::V1_0::HdmiPortType::INPUT));
static_assert(android::hardware::tv::cec::HdmiPortType::OUTPUT ==
              static_cast<android::hardware::tv::cec::HdmiPortType>(
                      ::android::hardware::tv::cec::V1_0::HdmiPortType::OUTPUT));

static_assert(android::hardware::tv::cec::OptionKey::WAKEUP ==
              static_cast<android::hardware::tv::cec::OptionKey>(
                      ::android::hardware::tv::cec::V1_0::OptionKey::WAKEUP));
static_assert(android::hardware::tv::cec::OptionKey::ENABLE_CEC ==
              static_cast<android::hardware::tv::cec::OptionKey>(
                      ::android::hardware::tv::cec::V1_0::OptionKey::ENABLE_CEC));
static_assert(android::hardware::tv::cec::OptionKey::SYSTEM_CEC_CONTROL ==
              static_cast<android::hardware::tv::cec::OptionKey>(
                      ::android::hardware::tv::cec::V1_0::OptionKey::SYSTEM_CEC_CONTROL));

static_assert(android::hardware::tv::cec::CecMessageType::FEATURE_ABORT ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::FEATURE_ABORT));
static_assert(android::hardware::tv::cec::CecMessageType::IMAGE_VIEW_ON ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::IMAGE_VIEW_ON));
static_assert(android::hardware::tv::cec::CecMessageType::TUNER_STEP_INCREMENT ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TUNER_STEP_INCREMENT));
static_assert(android::hardware::tv::cec::CecMessageType::TUNER_STEP_DECREMENT ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TUNER_STEP_DECREMENT));
static_assert(android::hardware::tv::cec::CecMessageType::TUNER_DEVICE_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TUNER_DEVICE_STATUS));
static_assert(
        android::hardware::tv::cec::CecMessageType::GIVE_TUNER_DEVICE_STATUS ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_TUNER_DEVICE_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::RECORD_ON ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::RECORD_ON));
static_assert(android::hardware::tv::cec::CecMessageType::RECORD_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::RECORD_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::RECORD_OFF ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::RECORD_OFF));
static_assert(android::hardware::tv::cec::CecMessageType::TEXT_VIEW_ON ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TEXT_VIEW_ON));
static_assert(android::hardware::tv::cec::CecMessageType::RECORD_TV_SCREEN ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::RECORD_TV_SCREEN));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_DECK_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_DECK_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::DECK_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::DECK_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::SET_MENU_LANGUAGE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_MENU_LANGUAGE));
static_assert(android::hardware::tv::cec::CecMessageType::CLEAR_ANALOG_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::CLEAR_ANALOG_TIMER));
static_assert(android::hardware::tv::cec::CecMessageType::SET_ANALOG_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_ANALOG_TIMER));
static_assert(android::hardware::tv::cec::CecMessageType::TIMER_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TIMER_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::STANDBY ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::STANDBY));
static_assert(android::hardware::tv::cec::CecMessageType::PLAY ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::PLAY));
static_assert(android::hardware::tv::cec::CecMessageType::DECK_CONTROL ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::DECK_CONTROL));
static_assert(android::hardware::tv::cec::CecMessageType::TIMER_CLEARED_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TIMER_CLEARED_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::USER_CONTROL_PRESSED ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::USER_CONTROL_PRESSED));
static_assert(android::hardware::tv::cec::CecMessageType::USER_CONTROL_RELEASED ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::USER_CONTROL_RELEASED));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_OSD_NAME ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_OSD_NAME));
static_assert(android::hardware::tv::cec::CecMessageType::SET_OSD_NAME ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_OSD_NAME));
static_assert(android::hardware::tv::cec::CecMessageType::SET_OSD_STRING ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_OSD_STRING));
static_assert(android::hardware::tv::cec::CecMessageType::SET_TIMER_PROGRAM_TITLE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_TIMER_PROGRAM_TITLE));
static_assert(
        android::hardware::tv::cec::CecMessageType::SYSTEM_AUDIO_MODE_REQUEST ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::SYSTEM_AUDIO_MODE_REQUEST));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_AUDIO_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_AUDIO_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::SET_SYSTEM_AUDIO_MODE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_SYSTEM_AUDIO_MODE));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_AUDIO_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_AUDIO_STATUS));
static_assert(
        android::hardware::tv::cec::CecMessageType::GIVE_SYSTEM_AUDIO_MODE_STATUS ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_SYSTEM_AUDIO_MODE_STATUS));
static_assert(
        android::hardware::tv::cec::CecMessageType::SYSTEM_AUDIO_MODE_STATUS ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::SYSTEM_AUDIO_MODE_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::ROUTING_CHANGE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::ROUTING_CHANGE));
static_assert(android::hardware::tv::cec::CecMessageType::ROUTING_INFORMATION ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::ROUTING_INFORMATION));
static_assert(android::hardware::tv::cec::CecMessageType::ACTIVE_SOURCE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::ACTIVE_SOURCE));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_PHYSICAL_ADDRESS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_PHYSICAL_ADDRESS));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_PHYSICAL_ADDRESS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_PHYSICAL_ADDRESS));
static_assert(android::hardware::tv::cec::CecMessageType::REQUEST_ACTIVE_SOURCE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REQUEST_ACTIVE_SOURCE));
static_assert(android::hardware::tv::cec::CecMessageType::SET_STREAM_PATH ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_STREAM_PATH));
static_assert(android::hardware::tv::cec::CecMessageType::DEVICE_VENDOR_ID ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::DEVICE_VENDOR_ID));
static_assert(android::hardware::tv::cec::CecMessageType::VENDOR_COMMAND ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::VENDOR_COMMAND));
static_assert(
        android::hardware::tv::cec::CecMessageType::VENDOR_REMOTE_BUTTON_DOWN ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::VENDOR_REMOTE_BUTTON_DOWN));
static_assert(android::hardware::tv::cec::CecMessageType::VENDOR_REMOTE_BUTTON_UP ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::VENDOR_REMOTE_BUTTON_UP));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_DEVICE_VENDOR_ID ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_DEVICE_VENDOR_ID));
static_assert(android::hardware::tv::cec::CecMessageType::MENU_REQUEST ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::MENU_REQUEST));
static_assert(android::hardware::tv::cec::CecMessageType::MENU_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::MENU_STATUS));
static_assert(
        android::hardware::tv::cec::CecMessageType::GIVE_DEVICE_POWER_STATUS ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_DEVICE_POWER_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_POWER_STATUS ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_POWER_STATUS));
static_assert(android::hardware::tv::cec::CecMessageType::GET_MENU_LANGUAGE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GET_MENU_LANGUAGE));
static_assert(android::hardware::tv::cec::CecMessageType::SELECT_ANALOG_SERVICE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SELECT_ANALOG_SERVICE));
static_assert(android::hardware::tv::cec::CecMessageType::SELECT_DIGITAL_SERVICE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SELECT_DIGITAL_SERVICE));
static_assert(android::hardware::tv::cec::CecMessageType::SET_DIGITAL_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_DIGITAL_TIMER));
static_assert(android::hardware::tv::cec::CecMessageType::CLEAR_DIGITAL_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::CLEAR_DIGITAL_TIMER));
static_assert(android::hardware::tv::cec::CecMessageType::SET_AUDIO_RATE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_AUDIO_RATE));
static_assert(android::hardware::tv::cec::CecMessageType::INACTIVE_SOURCE ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::INACTIVE_SOURCE));
static_assert(android::hardware::tv::cec::CecMessageType::CEC_VERSION ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::CEC_VERSION));
static_assert(android::hardware::tv::cec::CecMessageType::GET_CEC_VERSION ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GET_CEC_VERSION));
static_assert(android::hardware::tv::cec::CecMessageType::VENDOR_COMMAND_WITH_ID ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::VENDOR_COMMAND_WITH_ID));
static_assert(android::hardware::tv::cec::CecMessageType::CLEAR_EXTERNAL_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::CLEAR_EXTERNAL_TIMER));
static_assert(android::hardware::tv::cec::CecMessageType::SET_EXTERNAL_TIMER ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::SET_EXTERNAL_TIMER));
static_assert(
        android::hardware::tv::cec::CecMessageType::REPORT_SHORT_AUDIO_DESCRIPTOR ==
        static_cast<android::hardware::tv::cec::CecMessageType>(
                ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_SHORT_AUDIO_DESCRIPTOR));
static_assert(android::hardware::tv::cec::CecMessageType::REQUEST_SHORT_AUDIO_DESCRIPTOR ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::
                              REQUEST_SHORT_AUDIO_DESCRIPTOR));
static_assert(android::hardware::tv::cec::CecMessageType::INITIATE_ARC ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::INITIATE_ARC));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_ARC_INITIATED ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_ARC_INITIATED));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_ARC_TERMINATED ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_ARC_TERMINATED));
static_assert(android::hardware::tv::cec::CecMessageType::REQUEST_ARC_INITIATION ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REQUEST_ARC_INITIATION));
static_assert(android::hardware::tv::cec::CecMessageType::REQUEST_ARC_TERMINATION ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REQUEST_ARC_TERMINATION));
static_assert(android::hardware::tv::cec::CecMessageType::TERMINATE_ARC ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::TERMINATE_ARC));
static_assert(android::hardware::tv::cec::CecMessageType::ABORT ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::ABORT));
static_assert(android::hardware::tv::cec::CecMessageType::GIVE_FEATURES ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::GIVE_FEATURES));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_FEATURES ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_FEATURES));
static_assert(android::hardware::tv::cec::CecMessageType::REQUEST_CURRENT_LATENCY ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REQUEST_CURRENT_LATENCY));
static_assert(android::hardware::tv::cec::CecMessageType::REPORT_CURRENT_LATENCY ==
              static_cast<android::hardware::tv::cec::CecMessageType>(
                      ::android::hardware::tv::cec::V1_1::CecMessageType::REPORT_CURRENT_LATENCY));

static_assert(android::hardware::tv::cec::CecLogicalAddress::TV ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::TV));
static_assert(android::hardware::tv::cec::CecLogicalAddress::RECORDER_1 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::RECORDER_1));
static_assert(android::hardware::tv::cec::CecLogicalAddress::RECORDER_2 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::RECORDER_2));
static_assert(android::hardware::tv::cec::CecLogicalAddress::TUNER_1 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::TUNER_1));
static_assert(android::hardware::tv::cec::CecLogicalAddress::PLAYBACK_1 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::PLAYBACK_1));
static_assert(android::hardware::tv::cec::CecLogicalAddress::AUDIO_SYSTEM ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::AUDIO_SYSTEM));
static_assert(android::hardware::tv::cec::CecLogicalAddress::TUNER_2 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::TUNER_2));
static_assert(android::hardware::tv::cec::CecLogicalAddress::TUNER_3 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::TUNER_3));
static_assert(android::hardware::tv::cec::CecLogicalAddress::PLAYBACK_2 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::PLAYBACK_2));
static_assert(android::hardware::tv::cec::CecLogicalAddress::RECORDER_3 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::RECORDER_3));
static_assert(android::hardware::tv::cec::CecLogicalAddress::TUNER_4 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::TUNER_4));
static_assert(android::hardware::tv::cec::CecLogicalAddress::PLAYBACK_3 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::PLAYBACK_3));
static_assert(android::hardware::tv::cec::CecLogicalAddress::FREE_USE ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::FREE_USE));
static_assert(android::hardware::tv::cec::CecLogicalAddress::UNREGISTERED ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::UNREGISTERED));
static_assert(android::hardware::tv::cec::CecLogicalAddress::BROADCAST ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::BROADCAST));
static_assert(android::hardware::tv::cec::CecLogicalAddress::BACKUP_1 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::BACKUP_1));
static_assert(android::hardware::tv::cec::CecLogicalAddress::BACKUP_2 ==
              static_cast<android::hardware::tv::cec::CecLogicalAddress>(
                      ::android::hardware::tv::cec::V1_1::CecLogicalAddress::BACKUP_2));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_0::HotplugEvent& in,
        android::hardware::tv::cec::HotplugEvent* out) {
    out->connected = static_cast<bool>(in.connected);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.portId > std::numeric_limits<int32_t>::max() || in.portId < 0) {
        return false;
    }
    out->portId = static_cast<int32_t>(in.portId);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_0::HdmiPortInfo& in,
        android::hardware::tv::cec::HdmiPortInfo* out) {
    out->type = static_cast<android::hardware::tv::cec::HdmiPortType>(in.type);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.portId > std::numeric_limits<int32_t>::max() || in.portId < 0) {
        return false;
    }
    out->portId = static_cast<int32_t>(in.portId);
    out->cecSupported = static_cast<bool>(in.cecSupported);
    out->arcSupported = static_cast<bool>(in.arcSupported);
    out->physicalAddress = static_cast<char16_t>(in.physicalAddress);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_1::CecMessage& in,
        android::hardware::tv::cec::CecMessage* out) {
    out->initiator = static_cast<android::hardware::tv::cec::CecLogicalAddress>(in.initiator);
    out->destination = static_cast<android::hardware::tv::cec::CecLogicalAddress>(in.destination);
    {
        size_t size = in.body.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.body[i] > std::numeric_limits<int8_t>::max() || in.body[i] < 0) {
                return false;
            }
            out->body.push_back(static_cast<int8_t>(in.body[i]));
        }
    }
    return true;
}

}  // namespace android::h2a