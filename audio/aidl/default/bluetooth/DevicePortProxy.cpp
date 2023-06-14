/*
 * Copyright 2023 The Android Open Source Project
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

#define LOG_TAG "AHAL_BluetoothPortProxy"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <audio_utils/primitives.h>
#include <inttypes.h>
#include <log/log.h>
#include <stdlib.h>

#include "BluetoothAudioSessionControl.h"
#include "DevicePortProxy.h"

namespace android::bluetooth::audio::aidl {

namespace {

// The maximum time to wait in std::condition_variable::wait_for()
constexpr unsigned int kMaxWaitingTimeMs = 4500;

}  // namespace

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
using ::aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioSessionControl;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioStatus;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::PortStatusCallbacks;
using ::aidl::android::hardware::bluetooth::audio::PresentationPosition;
using ::aidl::android::hardware::bluetooth::audio::SessionType;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;

using ::android::base::StringPrintf;
using ControlResultCallback =
        std::function<void(uint16_t cookie, bool start_resp, const BluetoothAudioStatus& status)>;
using SessionChangedCallback = std::function<void(uint16_t cookie)>;

std::ostream& operator<<(std::ostream& os, const BluetoothStreamState& state) {
    switch (state) {
        case BluetoothStreamState::DISABLED:
            return os << "DISABLED";
        case BluetoothStreamState::STANDBY:
            return os << "STANDBY";
        case BluetoothStreamState::STARTING:
            return os << "STARTING";
        case BluetoothStreamState::STARTED:
            return os << "STARTED";
        case BluetoothStreamState::SUSPENDING:
            return os << "SUSPENDING";
        case BluetoothStreamState::UNKNOWN:
            return os << "UNKNOWN";
        default:
            return os << android::base::StringPrintf("%#hhx", state);
    }
}

BluetoothAudioPortAidl::BluetoothAudioPortAidl()
    : cookie_(::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined),
      state_(BluetoothStreamState::DISABLED),
      session_type_(SessionType::UNKNOWN) {}

BluetoothAudioPortAidlOut::~BluetoothAudioPortAidlOut() {
    if (inUse()) TearDown();
}

BluetoothAudioPortAidlIn::~BluetoothAudioPortAidlIn() {
    if (inUse()) TearDown();
}

bool BluetoothAudioPortAidl::SetUp(const AudioDeviceDescription& description) {
    if (!initSessionType(description)) return false;

    state_ = BluetoothStreamState::STANDBY;

    auto control_result_cb = [port = this](uint16_t cookie, bool start_resp,
                                           const BluetoothAudioStatus& status) {
        (void)start_resp;
        if (!port->inUse()) {
            LOG(ERROR) << "control_result_cb: BluetoothAudioPortAidl is not in use";
            return;
        }
        if (port->cookie_ != cookie) {
            LOG(ERROR) << "control_result_cb: proxy of device port (cookie="
                       << StringPrintf("%#hx", cookie) << ") is corrupted";
            return;
        }
        port->ControlResultHandler(status);
    };
    auto session_changed_cb = [port = this](uint16_t cookie) {
        if (!port->inUse()) {
            LOG(ERROR) << "session_changed_cb: BluetoothAudioPortAidl is not in use";
            return;
        }
        if (port->cookie_ != cookie) {
            LOG(ERROR) << "session_changed_cb: proxy of device port (cookie="
                       << StringPrintf("%#hx", cookie) << ") is corrupted";
            return;
        }
        port->SessionChangedHandler();
    };
    // TODO: Add audio_config_changed_cb
    PortStatusCallbacks cbacks = {
            .control_result_cb_ = control_result_cb,
            .session_changed_cb_ = session_changed_cb,
    };
    cookie_ = BluetoothAudioSessionControl::RegisterControlResultCback(session_type_, cbacks);
    LOG(INFO) << __func__ << debugMessage();

    return (cookie_ != ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined);
}

bool BluetoothAudioPortAidl::initSessionType(const AudioDeviceDescription& description) {
    if (description.connection == AudioDeviceDescription::CONNECTION_BT_A2DP &&
        (description.type == AudioDeviceType::OUT_DEVICE ||
         description.type == AudioDeviceType::OUT_HEADPHONE ||
         description.type == AudioDeviceType::OUT_SPEAKER)) {
        LOG(VERBOSE) << __func__
                     << ": device=AUDIO_DEVICE_OUT_BLUETOOTH_A2DP (HEADPHONES/SPEAKER) ("
                     << description.toString() << ")";
        session_type_ = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_WIRELESS &&
               description.type == AudioDeviceType::OUT_HEARING_AID) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_HEARING_AID (MEDIA/VOICE) ("
                     << description.toString() << ")";
        session_type_ = SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_HEADSET) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_HEADSET (MEDIA/VOICE) ("
                     << description.toString() << ")";
        session_type_ = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_SPEAKER) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_SPEAKER (MEDIA) ("
                     << description.toString() << ")";
        session_type_ = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::IN_HEADSET) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_IN_BLE_HEADSET (VOICE) ("
                     << description.toString() << ")";
        session_type_ = SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_BROADCAST) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_BROADCAST (MEDIA) ("
                     << description.toString() << ")";
        session_type_ = SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH;
    } else {
        LOG(ERROR) << __func__ << ": unknown device=" << description.toString();
        return false;
    }

    if (!BluetoothAudioSessionControl::IsSessionReady(session_type_)) {
        LOG(ERROR) << __func__ << ": device=" << description.toString()
                   << ", session_type=" << toString(session_type_) << " is not ready";
        return false;
    }
    return true;
}

void BluetoothAudioPortAidl::TearDown() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << " unknown monitor";
        return;
    }

    LOG(INFO) << __func__ << debugMessage();
    BluetoothAudioSessionControl::UnregisterControlResultCback(session_type_, cookie_);
    cookie_ = ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined;
}

void BluetoothAudioPortAidl::ControlResultHandler(const BluetoothAudioStatus& status) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidlis not in use";
        return;
    }
    std::unique_lock<std::mutex> port_lock(cv_mutex_);
    BluetoothStreamState previous_state = state_;
    LOG(INFO) << "control_result_cb:" << debugMessage() << ", previous_state=" << previous_state
              << ", status=" << toString(status);

    std::string msg = StringPrintf(
            "control_result_cb: status=%s failure for session_type= %s, cookie=%#hx, "
            "previous_state=%#hhx",
            toString(status).c_str(), toString(session_type_).c_str(), cookie_, previous_state);
    switch (previous_state) {
        case BluetoothStreamState::STARTED:
            /* Only Suspend signal can be send in STARTED state*/
            if (status == BluetoothAudioStatus::RECONFIGURATION ||
                status == BluetoothAudioStatus::SUCCESS) {
                state_ = BluetoothStreamState::STANDBY;
            } else {
                // Set to standby since the stack may be busy switching between outputs
                LOG(WARNING) << msg;
            }
            break;
        case BluetoothStreamState::STARTING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                state_ = BluetoothStreamState::STARTED;
            } else {
                // Set to standby since the stack may be busy switching between outputs
                LOG(WARNING) << msg;
                state_ = BluetoothStreamState::STANDBY;
            }
            break;
        case BluetoothStreamState::SUSPENDING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                state_ = BluetoothStreamState::STANDBY;
            } else {
                // It will be failed if the headset is disconnecting, and set to disable
                // to wait for re-init again
                LOG(WARNING) << msg;
                state_ = BluetoothStreamState::DISABLED;
            }
            break;
        default:
            LOG(ERROR) << "control_result_cb: unexpected previous_state=" << msg;
            return;
    }
    port_lock.unlock();
    internal_cv_.notify_all();
}

void BluetoothAudioPortAidl::SessionChangedHandler() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    std::unique_lock<std::mutex> port_lock(cv_mutex_);
    BluetoothStreamState previous_state = state_;
    LOG(INFO) << "session_changed_cb:" << debugMessage() << ", previous_state=" << previous_state;
    state_ = BluetoothStreamState::DISABLED;
    port_lock.unlock();
    internal_cv_.notify_all();
}

bool BluetoothAudioPortAidl::inUse() const {
    return (cookie_ != ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined);
}

bool BluetoothAudioPortAidl::GetPreferredDataIntervalUs(size_t* interval_us) const {
    if (!inUse()) {
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(session_type_);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        return false;
    }

    const PcmConfiguration& pcm_cfg = hal_audio_cfg.get<AudioConfiguration::pcmConfig>();
    *interval_us = pcm_cfg.dataIntervalUs;
    return true;
}

bool BluetoothAudioPortAidlOut::LoadAudioConfig(PcmConfiguration* audio_cfg) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidlOut is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(session_type_);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        return false;
    }
    *audio_cfg = hal_audio_cfg.get<AudioConfiguration::pcmConfig>();
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << ", PcmConfig=["
                 << (*audio_cfg).toString() << "]";
    if ((*audio_cfg).channelMode == ChannelMode::UNKNOWN) {
        return false;
    }
    return true;
}

bool BluetoothAudioPortAidlIn::LoadAudioConfig(PcmConfiguration* audio_cfg) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidlIn is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(session_type_);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        return false;
    }
    *audio_cfg = hal_audio_cfg.get<AudioConfiguration::pcmConfig>();
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << ", PcmConfig=["
                 << (*audio_cfg).toString() << "]";
    if ((*audio_cfg).channelMode == ChannelMode::UNKNOWN) {
        return false;
    }
    return true;
}

bool BluetoothAudioPortAidl::CondwaitState(BluetoothStreamState state) {
    bool retval;
    std::unique_lock<std::mutex> port_lock(cv_mutex_);
    switch (state) {
        case BluetoothStreamState::STARTING:
            LOG(VERBOSE) << __func__ << debugMessage() << " waiting for STARTED";
            retval = internal_cv_.wait_for(
                    port_lock, std::chrono::milliseconds(kMaxWaitingTimeMs),
                    [this] { return this->state_ != BluetoothStreamState::STARTING; });
            retval = retval && state_ == BluetoothStreamState::STARTED;
            break;
        case BluetoothStreamState::SUSPENDING:
            LOG(VERBOSE) << __func__ << debugMessage() << " waiting for SUSPENDED";
            retval = internal_cv_.wait_for(
                    port_lock, std::chrono::milliseconds(kMaxWaitingTimeMs),
                    [this] { return this->state_ != BluetoothStreamState::SUSPENDING; });
            retval = retval && state_ == BluetoothStreamState::STANDBY;
            break;
        default:
            LOG(WARNING) << __func__ << debugMessage() << " waiting for KNOWN";
            return false;
    }

    return retval;  // false if any failure like timeout
}

bool BluetoothAudioPortAidl::Start() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_
                 << ", mono=" << (is_stereo_to_mono_ ? "true" : "false") << " request";
    bool retval = false;
    if (state_ == BluetoothStreamState::STANDBY) {
        state_ = BluetoothStreamState::STARTING;
        if (BluetoothAudioSessionControl::StartStream(session_type_)) {
            retval = CondwaitState(BluetoothStreamState::STARTING);
        } else {
            LOG(ERROR) << __func__ << ": session_type=" << toString(session_type_)
                       << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                       << " Hal fails";
        }
    }

    if (retval) {
        LOG(INFO) << __func__ << ": session_type=" << toString(session_type_)
                  << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                  << ", mono=" << (is_stereo_to_mono_ ? "true" : "false") << " done";
    } else {
        LOG(ERROR) << __func__ << ": session_type=" << toString(session_type_)
                   << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                   << " failure";
    }

    return retval;  // false if any failure like timeout
}

bool BluetoothAudioPortAidl::Suspend() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << " request";
    bool retval = false;
    if (state_ == BluetoothStreamState::STARTED) {
        state_ = BluetoothStreamState::SUSPENDING;
        if (BluetoothAudioSessionControl::SuspendStream(session_type_)) {
            retval = CondwaitState(BluetoothStreamState::SUSPENDING);
        } else {
            LOG(ERROR) << __func__ << ": session_type=" << toString(session_type_)
                       << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                       << " Hal fails";
        }
    }

    if (retval) {
        LOG(INFO) << __func__ << ": session_type=" << toString(session_type_)
                  << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                  << " done";
    } else {
        LOG(ERROR) << __func__ << ": session_type=" << toString(session_type_)
                   << ", cookie=" << StringPrintf("%#hx", cookie_) << ", state=" << state_
                   << " failure";
    }

    return retval;  // false if any failure like timeout
}

void BluetoothAudioPortAidl::Stop() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << " request";
    state_ = BluetoothStreamState::DISABLED;
    BluetoothAudioSessionControl::StopStream(session_type_);
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << " done";
}

size_t BluetoothAudioPortAidlOut::WriteData(const void* buffer, size_t bytes) const {
    if (!inUse()) return 0;
    if (!is_stereo_to_mono_) {
        return BluetoothAudioSessionControl::OutWritePcmData(session_type_, buffer, bytes);
    }

    // WAR to mix the stereo into Mono (16 bits per sample)
    const size_t write_frames = bytes >> 2;
    if (write_frames == 0) return 0;
    auto src = static_cast<const int16_t*>(buffer);
    std::unique_ptr<int16_t[]> dst{new int16_t[write_frames]};
    downmix_to_mono_i16_from_stereo_i16(dst.get(), src, write_frames);
    // a frame is 16 bits, and the size of a mono frame is equal to half a stereo.
    return BluetoothAudioSessionControl::OutWritePcmData(session_type_, dst.get(),
                                                         write_frames * 2) *
           2;
}

size_t BluetoothAudioPortAidlIn::ReadData(void* buffer, size_t bytes) const {
    if (!inUse()) return 0;
    return BluetoothAudioSessionControl::InReadPcmData(session_type_, buffer, bytes);
}

bool BluetoothAudioPortAidl::GetPresentationPosition(uint64_t* delay_ns, uint64_t* bytes,
                                                     timespec* timestamp) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    PresentationPosition presentation_position;
    bool retval = BluetoothAudioSessionControl::GetPresentationPosition(session_type_,
                                                                        presentation_position);
    *delay_ns = presentation_position.remoteDeviceAudioDelayNanos;
    *bytes = presentation_position.transmittedOctets;
    *timestamp = {
            .tv_sec = static_cast<__kernel_old_time_t>(
                    presentation_position.transmittedOctetsTimestamp.tvSec),
            .tv_nsec = static_cast<long>(presentation_position.transmittedOctetsTimestamp.tvNSec)};
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << state_ << ", delay=" << *delay_ns
                 << "ns, data=" << *bytes << " bytes, timestamp=" << timestamp->tv_sec << "."
                 << StringPrintf("%09ld", timestamp->tv_nsec) << "s";

    return retval;
}

void BluetoothAudioPortAidl::UpdateSourceMetadata(const SourceMetadata& source_metadata) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << state_ << ", "
               << source_metadata.tracks.size() << " track(s)";
    if (source_metadata.tracks.size() == 0) return;
    BluetoothAudioSessionControl::UpdateSourceMetadata(session_type_, source_metadata);
}

void BluetoothAudioPortAidl::UpdateSinkMetadata(const SinkMetadata& sink_metadata) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << state_ << ", "
               << sink_metadata.tracks.size() << " track(s)";
    if (sink_metadata.tracks.size() == 0) return;
    BluetoothAudioSessionControl::UpdateSinkMetadata(session_type_, sink_metadata);
}

BluetoothStreamState BluetoothAudioPortAidl::GetState() const {
    return state_;
}

void BluetoothAudioPortAidl::SetState(BluetoothStreamState state) {
    state_ = state;
}

bool BluetoothAudioPortAidl::IsA2dp() const {
    return session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
           session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

bool BluetoothAudioPortAidl::IsLeAudio() const {
    return session_type_ == SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH ||
           session_type_ == SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH ||
           session_type_ == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
           session_type_ == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
           session_type_ == SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH ||
           session_type_ == SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

std::string BluetoothAudioPortAidl::debugMessage() const {
    return StringPrintf(": session_type=%s, cookie=%#hx", toString(session_type_).c_str(), cookie_);
}

}  // namespace android::bluetooth::audio::aidl
