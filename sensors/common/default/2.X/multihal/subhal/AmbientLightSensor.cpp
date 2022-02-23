#include "AmbientLightSensor.h"

#include <fcntl.h>
#include <hardware/sensors.h>
#include <log/log.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/SystemClock.h>

/* Just for testing purposes */
#define MIN_FUNC_NAME_LEN (25)
#define MAX_FUNC_NAME_LEN (25)
#define MAX_LINE_NUM_LEN (4)
#define log_color(COLOR, msg, ...)                                                  \
    ALOGE("HelloGoogle3 %s%*.*s(): %*d --> " msg " %s\n", COLOR, MIN_FUNC_NAME_LEN, \
          MAX_FUNC_NAME_LEN, __func__, MAX_LINE_NUM_LEN, __LINE__, ##__VA_ARGS__, NC)

extern const char* RED;
extern const char* BLUE;
extern const char* GREEN;
extern const char* YELLOW;
extern const char* NC;

const char* RED = "\e[0;31m";
const char* BLUE = "\e[0;34m";
const char* GREEN = "\e[0;32m";
const char* YELLOW = "\e[0;33m";
const char* NC = "\e[0m";

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::SensorInfo;
using ::android::hardware::sensors::V2_1::SensorType;

AmbientLightSensor::AmbientLightSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : mIsEnabled(false),
      mSamplingPeriodNs(0),
      mLastSampleTimeNs(0),
      mMode(OperationMode::NORMAL),
      mSensorThread(this),
      mCallback(callback) {
    sensor_info_.sensorHandle = sensorHandle;
    sensor_info_.name = "TCS3410 Ambient Light Sensor";
    sensor_info_.type = SensorType::LIGHT;
    sensor_info_.typeAsString = SENSOR_STRING_TYPE_LIGHT;
    sensor_info_.maxRange = kMaxRange;
    sensor_info_.minDelay = kMinDelay;
    sensor_info_.resolution = kResolution;

    openInputDevice();

    device_info_.sample_time = readSampleTime();
    device_info_.als_nr_samples = readAlsNrSamples();

    openFifo();

    // Start polling sensor
    mSensorThread.start();
}

AmbientLightSensor::~AmbientLightSensor() {
    disableFeature();
    closeFifo();
    closeInputDevice();
}

const SensorInfo& AmbientLightSensor::getSensorInfo() const {
    return sensor_info_;
}

void AmbientLightSensor::batch(int32_t sampling_period_ns) {
    sampling_period_ns = std::clamp(sampling_period_ns, sensor_info_.minDelay * 1000,
                                    sensor_info_.maxDelay * 1000);
    if (mSamplingPeriodNs != sampling_period_ns) {
        mSamplingPeriodNs = sampling_period_ns;
        mSensorThread.notifyAll();
    }
}

void AmbientLightSensor::activate(bool enable) {
    if (mIsEnabled != enable) {
        std::unique_lock<std::mutex> lock(mSensorThread.lock());
        mIsEnabled = enable;
        mSensorThread.notifyAll();
    }
}

Result AmbientLightSensor::flush() {
    if (!mIsEnabled) {
        return Result::BAD_VALUE;
    }

    Event ev;
    ev.sensorHandle = sensor_info_.sensorHandle;
    ev.sensorType = SensorType::META_DATA;
    ev.u.meta.what = V1_0::MetaDataEventType::META_DATA_FLUSH_COMPLETE;
    std::vector<Event> evs{ev};
    mCallback->postEvents(evs, false);

    return Result::OK;
}

void AmbientLightSensor::pollSensor() {
    if (!mIsEnabled || mMode == OperationMode::DATA_INJECTION) {
    } else {
        readEvents();
    }
}

void AmbientLightSensor::setOperationMode(android::hardware::sensors::V1_0::OperationMode mode) {
    if (mMode != mode) {
        std::unique_lock<std::mutex> lock(mSensorThread.lock());
        mMode = mode;
        mSensorThread.notifyAll();
    }
}

Result AmbientLightSensor::injectEvent(const Event& event) {
    if (event.sensorType == SensorType::ADDITIONAL_INFO) {
    } else if (mMode == android::hardware::sensors::V1_0::OperationMode::DATA_INJECTION) {
        mCallback->postEvents(std::vector<Event>{event}, false);
    }
    return Result::OK;
}

int AmbientLightSensor::openInputDevice() {
    device_info_.input_fd = open(kInputDeviceBaseName, O_RDONLY);

    if (device_info_.input_fd > 0) {
        return 0;
    }

    return -1;
}

void AmbientLightSensor::closeInputDevice() {
    if (device_info_.input_fd) {
        close(device_info_.input_fd);
        device_info_.input_fd = -1;
    }
}

int AmbientLightSensor::openFifo() {
    device_info_.fifo_fd = open(kSysFsFifoName, O_RDONLY);

    if (device_info_.fifo_fd > 0) {
        return 0;
    }

    return -1;
}

void AmbientLightSensor::readFifo(char* p_buffer) {
    /* Move buffer ptr to the end of data */
    p_buffer += device_info_.fifo_data_sz;

    (void)lseek(device_info_.fifo_fd, 0, SEEK_SET);
    int bytes_read = read(device_info_.fifo_fd, p_buffer, kUserPageSize);

    device_info_.fifo_data_sz += bytes_read;
}

void AmbientLightSensor::closeFifo() {
    if (device_info_.fifo_fd) {
        close(device_info_.fifo_fd);
        device_info_.fifo_fd = -1;
    }
}

void AmbientLightSensor::writeToSysFs(const char* file, int value) {
    char command[255];
    snprintf(command, sizeof(command), "echo \"%d\" > %s", value, file);
    auto fp = popen(command, "w");

    if (fp) {
        pclose(fp);
    }
}

void AmbientLightSensor::readFromSysFs(const char* file, int16_t* data) {
    char command[255];
    snprintf(command, sizeof(command), "cat %s", file);

    auto fp = popen(command, "r");
    if (fp) {
        fscanf(fp, "%hd", data);
        pclose(fp);
    }
}

void AmbientLightSensor::enableFeature() {
    writeToSysFs(kSysFsAlsEnable, 1);
}

void AmbientLightSensor::disableFeature() {
    writeToSysFs(kSysFsAlsEnable, 0);
}

int AmbientLightSensor::readSampleTime() {
    int16_t sample_time = -1;

    readFromSysFs(kSysFsSampleTime, &sample_time);
    /* Register value is base 0. Increment by 1 */
    sample_time++;

    return sample_time;
}

int AmbientLightSensor::readAlsNrSamples() {
    int16_t als_nr_samples = -1;

    readFromSysFs(kSysFsAlsNrSamples, &als_nr_samples);
    /* Register value is base 0. Incrememt by 1 */
    als_nr_samples++;

    return als_nr_samples;
}

bool AmbientLightSensor::checkSaturation(AlsData als_data[]) {
    uint16_t step;
    uint8_t seq_step, ch0_sat, ch1_sat, ch2_sat;
    for (step = 0; step < kNumStepAls; step++) {
        seq_step = (als_data[step].status[kAlsStatusReg] & AlsStatusMask.ALS_MEAS_SEQ_STEP_MASK) >>
                   AlsStatusShift.ALS_MEAS_SEQ_STEP_SHIFT;
        ch0_sat = (als_data[step].status[kAlsStatusReg] &
                   AlsStatusMask.ALS_DATA0_ANALOG_SATURATION_STATUS_MASK) >>
                  AlsStatusShift.ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT;
        ch1_sat = (als_data[step].status[kAlsStatusReg] &
                   AlsStatusMask.ALS_DATA1_ANALOG_SATURATION_STATUS_MASK) >>
                  AlsStatusShift.ALS_DATA1_ANALOG_SATURATION_STATUS_SHIFT;
        ch2_sat = (als_data[step].status[kAlsStatusReg] &
                   AlsStatusMask.ALS_DATA2_ANALOG_SATURATION_STATUS_MASK) >>
                  AlsStatusShift.ALS_DATA2_ANALOG_SATURATION_STATUS_SHIFT;

        if (ch0_sat || ch1_sat || ch2_sat) {
            return true;
        }
    }

    return false;
}

void AmbientLightSensor::processAlsData() {
    AlsData als_data[kNumStepAls];
    int idx = 0;
    for (int step = 0; step < kNumStepAls; step++) {
        for (int mod = 0; mod < kNumModulators; mod++) {
            als_data[step].mod_counts[mod] =
                    (((fifo_buffer_[idx + 2] & 0xFF) << 16) |
                     ((fifo_buffer_[idx + 1] & 0xFF) << 8) | ((fifo_buffer_[idx] & 0xFF) << 0));
            idx += 3;
        }

        for (uint16_t i = 0; i < kNumAlsStatusRegs; i++) {
            als_data[step].status[i] = fifo_buffer_[idx++];
        }

        als_data[step].gains[kModulator0] = ((als_data[step].status[kAlsStatus2Reg] & 0x0F) >> 0);
        als_data[step].gains[kModulator1] = ((als_data[step].status[kAlsStatus2Reg] & 0xF0) >> 4);
        als_data[step].gains[kModulator2] = ((als_data[step].status[kAlsStatus3Reg] & 0x0F) >> 0);

        als_data[step].mod_gains[kModulator0] = (1 << (als_data[step].gains[kModulator0] - 1));
        als_data[step].mod_gains[kModulator1] = (1 << (als_data[step].gains[kModulator1] - 1));
        als_data[step].mod_gains[kModulator2] = (1 << (als_data[step].gains[kModulator2] - 1));
    }

    if (checkSaturation(&als_data[0])) {
        return;
    }

    /* Normalize the channel data */
    /* Calculate gain ratios with Clear (step 0, mod 0) as the reference */
    for (int step = 0; step < kNumStepAls; step++) {
        for (int mod = 0; mod < kNumModulators; mod++) {
            /* Reference gain and count */
            if (step == kStep0 && mod == kModulator0) {
                als_data[kStep0].mod_normalized_gains[kModulator0] =
                        als_data[kStep0].mod_gains[kModulator0];
                als_data[kStep0].mod_normalized_counts[kModulator0] =
                        (double)als_data[kStep0].mod_counts[kModulator0];
            } else {
                /* Normalize all other channel data to clear */
                als_data[step].mod_normalized_gains[mod] =
                        als_data[step].mod_gains[mod] / als_data[kStep0].mod_gains[kModulator0];
                /* Normalize the [step, mod] count to that of its normalized gains */
                als_data[step].mod_normalized_counts[mod] =
                        als_data[step].mod_counts[mod] / als_data[step].mod_normalized_gains[mod];
            }
        }
    }

    /* Ambient light condition might be different across sequencer steps. */
    /* Calculate matching factor with clear channel that appears in both steps */
    als_data[kStep0].matching_factor = 1;
    als_data[kStep1].matching_factor = als_data[kStep1].mod_normalized_counts[kModulator0] /
                                       als_data[kStep0].mod_normalized_counts[kModulator0];

    /* Correct als counts with channel matching factor */
    for (int step = 0; step < kNumStepAls; step++) {
        for (int mod = 0; mod < kNumModulators; mod++) {
            als_data[step].mod_normalized_counts[mod] =
                    als_data[step].matching_factor * als_data[step].mod_normalized_counts[mod];
        }
    }

    Sample sample;
    sample.als.red = als_data[kStep0].mod_normalized_counts[kModulator2];
    sample.als.green = als_data[kStep0].mod_normalized_counts[kModulator1];
    sample.als.blue = als_data[kStep1].mod_normalized_counts[kModulator1];
    sample.als.wb = als_data[kStep1].mod_normalized_counts[kModulator2];
    sample.als.clear = (als_data[kStep0].mod_normalized_counts[kModulator0] +
                        als_data[kStep1].mod_normalized_counts[kModulator0]) /
                       2.0;

    log_color(RED, "RED: %.2f", sample.als.red);
    log_color(GREEN, "GREEN: %.2f", sample.als.green);
    log_color(BLUE, "BLUE: %.2f", sample.als.blue);
    log_color(YELLOW, "WIDEBAND: %.2f", sample.als.wb);
    log_color(NC, "CLEAR: %.2f", sample.als.clear);

    float ir_comp = (sample.als.red + sample.als.green + sample.als.blue) / 2.0;
    float ir_comp_ratio = ir_comp / sample.als.clear;

    int n;
    if (ir_comp_ratio < kIrCompRatioLow) {
        n = kIrCompLow;
    } else if (ir_comp_ratio >= kIrCompRatioLow && ir_comp_ratio < kIrCompRatioHigh) {
        n = kIrCompMed;
    } else {
        n = kIrCompHigh;
    }

    auto atime = (device_info_.sample_time) * (device_info_.als_nr_samples) * kModulatorClockMs;

    /* Calculate lux */
    sample.als.lux = lux_coeffs_[kDgf][n] +
                     ((lux_coeffs_[kRedCoeff][n] * sample.als.red) +
                      (lux_coeffs_[kGreenCoeff][n] * sample.als.green) +
                      (lux_coeffs_[kBlueCoeff][n] * sample.als.blue) +
                      (lux_coeffs_[kWbCoeff][n] * sample.als.wb) +
                      (lux_coeffs_[kClearCoeff][n] * sample.als.clear)) /
                             (atime * als_data[kStep0].mod_normalized_gains[kModulator0]);

    /* Calculate CCT */
    float r_prime = sample.als.red - ir_comp;
    if (r_prime == 0) {
        r_prime = 1.0;
    }

    float b_prime = sample.als.blue - ir_comp;

    sample.als.cct = (cct_coeffs_[kCoeffA][n] * (b_prime / r_prime)) + cct_coeffs_[kCtOffset][n];
    log_color(NC, "LUX: %.2f", sample.als.lux);
    log_color(NC, "CCT: %.2f", sample.als.cct);

    std::vector<Event> events;
    Event event;
    event.sensorHandle = sensor_info_.sensorHandle;
    event.sensorType = sensor_info_.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    event.u.data = sample.data;
    event.u.vec3.x = sample.als.cct;
    event.u.vec3.z = sample.als.cct;
    mCallback->postEvents(events, false);
}

void AmbientLightSensor::processFifoData(char* p_buffer) {
    processAlsData();
    /* Move past ALS data */
    p_buffer += kFifoFrameSize;
    device_info_.fifo_data_sz -= kFifoFrameSize;
}

void AmbientLightSensor::processInputEvent(struct input_event* event) {
    if (event->type == EV_ABS) {
        /* SINT event occurs at the end of every sequence round */
        /* First 36 bytes are ALS Data, and the remainder of the data is flicker */
        /* SINT event signifieds that the data can be processed. */
        if (event->code == ABS_DISTANCE) {
            if ((device_info_.event_state == kIdleState) ||
                (device_info_.event_state == kDataGatherInProgress)) {
                readFifo(fifo_buffer_);
                disableFeature();

                device_info_.event_state = kDataProcessing;
                processFifoData(fifo_buffer_);
            }
        }
    }
}

void AmbientLightSensor::readInputEvent(fd_set* fds) {
    struct input_event event;
    if ((device_info_.input_fd > 0) && FD_ISSET(device_info_.input_fd, fds)) {
        if (read(device_info_.input_fd, &event, sizeof(input_event)) > 0) {
            processInputEvent(&event);
        }
    }
}

void AmbientLightSensor::readEvents() {
    enableFeature();

    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(device_info_.input_fd, &fds);

    int max_fd = device_info_.input_fd + 1;

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (select(max_fd, &fds, NULL, NULL, &timeout) > 0) {
        readInputEvent(&fds);

        device_info_.event_state = kIdleState;
        device_info_.fifo_data_sz = 0;
        enableFeature();
    }
}

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
