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

#include <sensors/common_convert.h>
#include <cstring>

namespace android {
namespace hardware {
namespace sensors {
namespace implementation {
namespace common {

sensors_event_t convertASensorEvent(const ASensorEvent& src) {
    sensors_event_t dst;
    dst.version = src.version;
    dst.sensor = src.sensor;
    dst.type = src.type;
    dst.reserved0 = src.reserved0;
    dst.timestamp = src.timestamp;
    dst.flags = src.flags;
    dst.reserved1[0] = src.reserved1[0];
    dst.reserved1[1] = src.reserved1[1];
    dst.reserved1[2] = src.reserved1[2];
    switch (src.type) {
#if 0
        No such case?
        case ASENSOR_TYPE_META_DATA: {
            // Legacy HALs expect the handle reference in the meta data field.
            // Copy it over from the handle of the event.
            dst.meta_data.what = src.meta_data.what;
            dst.meta_data.sensor = src.meta_data.sensor;
            // Set the sensor handle to 0 to maintain compatibility.
            dst.sensor = 0;
            break;
        }
#endif

#if 0
        no such case?
        case ASENSOR_TYPE_ORIENTATION:
#endif

        case ASENSOR_TYPE_ACCELEROMETER:
        case ASENSOR_TYPE_MAGNETIC_FIELD:
        case ASENSOR_TYPE_GYROSCOPE:
        case ASENSOR_TYPE_GRAVITY:
        case ASENSOR_TYPE_LINEAR_ACCELERATION: {
            dst.acceleration.x = src.acceleration.x;
            dst.acceleration.y = src.acceleration.y;
            dst.acceleration.z = src.acceleration.z;
            dst.acceleration.status = src.acceleration.status;
            break;
        }

        case ASENSOR_TYPE_GAME_ROTATION_VECTOR: {
            dst.data[0] = src.data[0];
            dst.data[1] = src.data[1];
            dst.data[2] = src.data[2];
            dst.data[3] = src.data[3];
            break;
        }

        case ASENSOR_TYPE_ROTATION_VECTOR:
        case ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR: {
            dst.data[0] = src.data[0];
            dst.data[1] = src.data[1];
            dst.data[2] = src.data[2];
            dst.data[3] = src.data[3];
            dst.data[4] = src.data[4];
            break;
        }

        case ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        case ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED: {
            dst.uncalibrated_gyro.x_uncalib = src.uncalibrated_gyro.x_uncalib;
            dst.uncalibrated_gyro.y_uncalib = src.uncalibrated_gyro.y_uncalib;
            dst.uncalibrated_gyro.z_uncalib = src.uncalibrated_gyro.z_uncalib;
            dst.uncalibrated_gyro.x_bias = src.uncalibrated_gyro.x_bias;
            dst.uncalibrated_gyro.y_bias = src.uncalibrated_gyro.y_bias;
            dst.uncalibrated_gyro.z_bias = src.uncalibrated_gyro.z_bias;
            break;
        }
#if 0
        case ASENSOR_TYPE_DEVICE_ORIENTATION:
        case ASENSOR_TYPE_TILT_DETECTOR:
        case ASENSOR_TYPE_WAKE_GESTURE:
        case ASENSOR_TYPE_GLANCE_GESTURE:
        case ASENSOR_TYPE_PICK_UP_GESTURE:
        case ASENSOR_TYPE_WRIST_TILT_GESTURE:
#endif
        case ASENSOR_TYPE_HINGE_ANGLE:
        case ASENSOR_TYPE_LIGHT:
        case ASENSOR_TYPE_PRESSURE:
        case ASENSOR_TYPE_PROXIMITY:
        case ASENSOR_TYPE_RELATIVE_HUMIDITY:
        case ASENSOR_TYPE_AMBIENT_TEMPERATURE:
        case ASENSOR_TYPE_SIGNIFICANT_MOTION:
        case ASENSOR_TYPE_STEP_DETECTOR:
        case ASENSOR_TYPE_STATIONARY_DETECT:
        case ASENSOR_TYPE_MOTION_DETECT:
        case ASENSOR_TYPE_HEART_BEAT:
        case ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT: {
            dst.data[0] = src.data[0];
            break;
        }

        case ASENSOR_TYPE_STEP_COUNTER: {
            dst.u64.step_counter = src.u64.step_counter;
            break;
        }

        case ASENSOR_TYPE_HEART_RATE: {
            dst.heart_rate.bpm = src.heart_rate.bpm;
            dst.heart_rate.status = src.heart_rate.status;
            break;
        }

        case ASENSOR_TYPE_POSE_6DOF: {  // 15 floats
            for (size_t i = 0; i < 15; ++i) {
                dst.data[i] = src.data[i];
            }
            break;
        }

        case ASENSOR_TYPE_DYNAMIC_SENSOR_META: {
            dst.dynamic_sensor_meta.connected = src.dynamic_sensor_meta.connected;
            dst.dynamic_sensor_meta.handle = src.dynamic_sensor_meta.handle;
            dst.dynamic_sensor_meta.sensor = NULL;  // to be filled in later
#if 0
            memcpy(dst.dynamic_sensor_meta.uuid,
                   src.dynamic_sensor_meta.uuid, 16);
#endif

            break;
        }

        case ASENSOR_TYPE_ADDITIONAL_INFO: {
            dst.additional_info.type = src.additional_info.type;
            dst.additional_info.serial = src.additional_info.serial;

            static_assert(sizeof(src.additional_info.data_float) ==
                          sizeof(dst.additional_info.data_float));
            for (int i = 0; i < sizeof(src.additional_info.data_float); i++) {
                dst.additional_info.data_float[i] = src.additional_info.data_float[i];
            }
#if 0
            I don't see any int32 types in AAdditionalInfoEvent types? What's up?
            https://source.corp.google.com/android/frameworks/native/include/android/sensor.h;rcl=89c5ff0882cd0fff74215e263545e9b7e6e3d099;l=388
            switch (src.additional_info.type) {
                case AdditionalInfo::AdditionalInfoPayload::Tag::dataInt32: {
                    const auto& values =
                            srcInfo.get<AdditionalInfo::AdditionalInfoPayload::dataInt32>()
                                    .values;
                    CHECK_EQ(values.size() * sizeof(int32_t), sizeof(dstInfo->data_int32));
                    memcpy(dstInfo->data_int32, values.data(), sizeof(dstInfo->data_int32));
                    break;
                }
                case AdditionalInfo::AdditionalInfoPayload::Tag::dataFloat: {
                    const auto& values =
                            srcInfo.get<AdditionalInfo::AdditionalInfoPayload::dataFloat>()
                                    .values;
                    CHECK_EQ(values.size() * sizeof(float), sizeof(dstInfo->data_float));
                    memcpy(dstInfo->data_float, values.data(), sizeof(dstInfo->data_float));
                    break;
                }
                default: {
                    LOG(ERROR) << "Invalid sensor additional info tag: ",
                            (int)srcInfo.getTag();
                }
            }
#endif
            break;
        }

        case ASENSOR_TYPE_HEAD_TRACKER: {
            dst.head_tracker.rx = src.head_tracker.rx;
            dst.head_tracker.ry = src.head_tracker.ry;
            dst.head_tracker.rz = src.head_tracker.rz;
            dst.head_tracker.vx = src.head_tracker.vx;
            dst.head_tracker.vy = src.head_tracker.vy;
            dst.head_tracker.vz = src.head_tracker.vz;
            dst.head_tracker.discontinuity_count = src.head_tracker.discontinuity_count;
            break;
        }

        case ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES:
        case ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES:
            dst.limited_axes_imu.x = src.limited_axes_imu.x;
            dst.limited_axes_imu.y = src.limited_axes_imu.y;
            dst.limited_axes_imu.z = src.limited_axes_imu.z;
            dst.limited_axes_imu.x_supported = src.limited_axes_imu.x_supported;
            dst.limited_axes_imu.y_supported = src.limited_axes_imu.y_supported;
            dst.limited_axes_imu.z_supported = src.limited_axes_imu.z_supported;
            break;

        case ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED:
        case ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED:
            dst.limited_axes_imu_uncalibrated.x_uncalib =
                    src.limited_axes_imu_uncalibrated.x_uncalib;
            dst.limited_axes_imu_uncalibrated.y_uncalib =
                    src.limited_axes_imu_uncalibrated.y_uncalib;
            dst.limited_axes_imu_uncalibrated.z_uncalib =
                    src.limited_axes_imu_uncalibrated.z_uncalib;
            dst.limited_axes_imu_uncalibrated.x_bias = src.limited_axes_imu_uncalibrated.x_bias;
            dst.limited_axes_imu_uncalibrated.y_bias = src.limited_axes_imu_uncalibrated.y_bias;
            dst.limited_axes_imu_uncalibrated.z_bias = src.limited_axes_imu_uncalibrated.z_bias;
            dst.limited_axes_imu_uncalibrated.x_supported =
                    src.limited_axes_imu_uncalibrated.x_supported;
            dst.limited_axes_imu_uncalibrated.y_supported =
                    src.limited_axes_imu_uncalibrated.y_supported;
            dst.limited_axes_imu_uncalibrated.z_supported =
                    src.limited_axes_imu_uncalibrated.z_supported;
            break;

        case ASENSOR_TYPE_HEADING:
            dst.heading.heading = src.heading.heading;
            dst.heading.accuracy = src.heading.accuracy;
            break;

        default: {
#if 0
            This type doesn't exist, should we copy the data over in this case?
            CHECK_GE(src.sensorType, ASENSOR_TYPE_DEVICE_PRIVATE_BASE);
#endif
            memcpy(dst.data, src.data, 16 * sizeof(float));
            break;
        }
    }

    return dst;
}

}  // namespace common
}  // namespace implementation
}  // namespace sensors
}  // namespace hardware
}  // namespace android
