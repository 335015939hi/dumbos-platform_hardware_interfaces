#ifndef MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_AMBIENTLIGHTSENSOR_H_
#define MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_AMBIENTLIGHTSENSOR_H_

#include <android/hardware/sensors/2.1/types.h>
#include <linux/input.h>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>
#include "SensorThread.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

class ISensorsEventCallback {
  public:
    virtual ~ISensorsEventCallback(){};
    virtual void postEvents(const std::vector<Event>& events, bool wakeup) = 0;
};

class AmbientLightSensor {
  public:
    AmbientLightSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
    virtual ~AmbientLightSensor();

    const SensorInfo& getSensorInfo() const;
    void batch(int32_t sampling_period_ns);
    virtual void activate(bool enable);
    android::hardware::sensors::V1_0::Result flush();
    void setOperationMode(android::hardware::sensors::V1_0::OperationMode mode);
    android::hardware::sensors::V1_0::Result injectEvent(const Event& event);

  protected:
    friend class SensorThread;
    void pollSensor();

    android::hardware::sensors::V2_1::SensorInfo sensor_info_;
    bool mIsEnabled;
    int64_t mSamplingPeriodNs;
    int64_t mLastSampleTimeNs;
    android::hardware::sensors::V1_0::OperationMode mMode;

    SensorThread mSensorThread;
    ISensorsEventCallback* mCallback;

  private:
    static constexpr char kInputDeviceBaseName[] = "/dev/input/event1";
    static constexpr char kSysFsFifoName[] = "/sys/class/input/input1/fifo";
    static constexpr char kSysFsSampleTime[] = "/sys/class/input/input1/sample_time";
    static constexpr char kSysFsAlsEnable[] = "/sys/class/input/input2/enable";
    static constexpr char kSysFsAlsNrSamples[] = "/sys/class/input/input2/als_nr_samples";

    static constexpr int kFifoFrameSize = 24;
    static constexpr int kNumStepAls = 2;
    static constexpr float kModulatorClockMs = 0.001388889f;
    static constexpr int kUserPageSize = 4096;

    static constexpr float kMaxRange = 1e6f;
    static constexpr float kMinDelay = 1000000.0 / 120;
    static constexpr float kResolution = 1e-2f;

    static constexpr float kIrCompRatioLow = 0.078;
    static constexpr float kIrCompRatioHigh = 0.131;

    enum AlsStatusReg {
        kAlsStatusReg,
        kAlsStatus2Reg,
        kAlsStatus3Reg,
        kNumAlsStatusRegs,
    };

    enum CctCoefficient {
        kCoeffA,
        kCtOffset,
    };

    enum EventState {
        kIdleState,
        kDataGatherInProgress,
        kDataProcessing,
    };

    enum IrCompLevel {
        kIrCompLow,
        kIrCompMed,
        kIrCompHigh,
        kNumIrCompLevels,
    };

    enum LuxCoefficient {
        kRedCoeff,
        kGreenCoeff,
        kBlueCoeff,
        kWbCoeff,
        kClearCoeff,
        kDgf,
    };

    enum Modulator {
        kModulator0,
        kModulator1,
        kModulator2,
        kNumModulators,
    };

    enum SequencerStep {
        kStep0,
        kStep1,
        kStep2,
        kStep3,
        kNumSteps,
    };

    struct AlsData {
        /* Raw counts from the device. */
        uint32_t mod_counts[kNumModulators];
        /* Normalized raw counts by a specific gain. */
        double mod_normalized_counts[kNumModulators];
        /* Status registers from the device. */
        uint8_t status[kNumAlsStatusRegs];
        /* Register value of the gains */
        uint8_t gains[kNumModulators];
        /* Numeric equivalent of gains */
        double mod_gains[kNumModulators];
        /* Normalized gains to the clear channel in step 0 mod 0 */
        double mod_normalized_gains[kNumModulators];
        /* Compare clear channel from each step */
        double matching_factor;
    };

    struct AlsDeviceInfo {
        EventState event_state = kIdleState;
        int input_fd = -1;
        int fifo_fd = -1;
        int32_t fifo_data_sz = 0;
        int16_t sample_time = 0;
        int16_t als_nr_samples = 0;
    };

    struct AlsSample {
        float lux;
        float cct;
        float red;
        float green;
        float blue;
        float clear;
        float wb;
    };

    struct ALS_STATUS_SHIFT {
        uint8_t ALS_DATA2_ANALOG_SATURATION_STATUS_SHIFT = 3;
        uint8_t ALS_DATA1_ANALOG_SATURATION_STATUS_SHIFT = 4;
        uint8_t ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT = 5;
        uint8_t ALS_MEAS_SEQ_STEP_SHIFT = 6;
    } AlsStatusShift;

    struct ALS_STATUS_MASK {
        uint8_t ALS_DATA2_ANALOG_SATURATION_STATUS_MASK = (1 << 3);
        uint8_t ALS_DATA1_ANALOG_SATURATION_STATUS_MASK = (1 << 4);
        uint8_t ALS_DATA0_ANALOG_SATURATION_STATUS_MASK = (1 << 5);
        uint8_t ALS_MEAS_SEQ_STEP_MASK = (3 << 6);
    } AlsStatusMask;

    union Sample {
        AlsSample als;
        float data[7];
    };

    int openInputDevice();
    void closeInputDevice();
    int openFifo();
    void readFifo(char* p_buffer);
    void closeFifo();
    void writeToSysFs(const char* file, int value);
    void readFromSysFs(const char* file, int16_t* data);
    void enableFeature();
    void disableFeature();
    int readSampleTime();
    int readAlsNrSamples();

    bool checkSaturation(AlsData als_data[]);
    void processAlsData();
    void processFifoData(char* p_buffer);
    void processInputEvent(struct input_event* event);
    void readInputEvent(fd_set* fds);

    void readEvents();

    /* Buffer that holds FIFO data */
    char fifo_buffer_[kUserPageSize];

    AlsDeviceInfo device_info_;
    static constexpr double cct_coeffs_[][kNumIrCompLevels] = {
            [kCoeffA] = {5239, 8096, 215},
            [kCtOffset] = {1747, 1518, 2309},
    };
    static constexpr double lux_coeffs_[][kNumIrCompLevels] = {
            [kRedCoeff] = {0.861, -1.37, 0.461},    [kGreenCoeff] = {2.004, -1.197, -0.186},
            [kBlueCoeff] = {0.132, -0.659, 0.778},  [kWbCoeff] = {0.671, 0.015, -0.051},
            [kClearCoeff] = {-1.895, 1.808, 0.199}, [kDgf] = {5.0, 5.0, 5.0},
    };
};

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
#endif  // MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_AMBIENTLIGHTSENSOR_H_
