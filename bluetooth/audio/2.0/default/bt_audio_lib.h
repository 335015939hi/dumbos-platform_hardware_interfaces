/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef BLUETOOTH_AUDIO_HIDL_LIB_H_
#define BLUETOOTH_AUDIO_HIDL_LIB_H_

#include <hardware/audio.h>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <android/hardware/bluetooth/audio/2.0/types.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>

using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::hidl_vec;
using android::sp;

using ::android::hardware::bluetooth::audio::V2_0::SessionType;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;
typedef ::android::hardware::bluetooth::audio::V2_0::Status BluetoothAudioStatus;
typedef ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration::PcmDataConfiguration PcmDataConfiguration;

const sp<IBluetoothAudioPort> bt_audio_get_port_ctrl_path(const SessionType& sessionType);
const DataMQ::Descriptor* bt_audio_get_data_fmq(const SessionType& sessionType);
const PcmDataConfiguration& bt_audio_get_pcm_data_cfg(const SessionType& sessionType);

bool bt_audio_get_presentation_position(const SessionType& sessionType,
                                        timespec &remote_delay_report,
                                        uint64_t &total_bytes_readed,
                                        timespec &data_position);
void bt_audio_update_tracks_metadata(const SessionType& sessionType,
                                     const struct source_metadata *source_metadata);

uint16_t bt_audio_set_port_ctrl_result_cb(
    const SessionType& sessionType,
    std::function<void(const uint16_t&, const BluetoothAudioStatus&)>& res_cb,
    std::function<void(const uint16_t&)>& end_cb);
void bt_audio_reset_port_ctrl_result_cb(const SessionType& sessionType,
                                        uint16_t& ctrl_key);

#endif // BLUETOOTH_AUDIO_HIDL_LIB_H_
