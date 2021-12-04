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

#include "Sensor.h"

#include <errno.h>
#include <fcntl.h>
#include <hardware/sensors.h>
#include <linux/input.h>
#include <log/log.h>
#include <utils/SystemClock.h>
#include <cmath>

typedef int32_t STATUS;
#define SUCCESS 0
#define FAILURE 1

#define INPUT_DEVICE_BASE_NAME "/dev/input/event1"
#define SYS_FS_FIFO_NAME "/sys/class/input/input1/fifo"
#define SYS_FS_FIFO_RESET "/sys/class/input/input1/fifo_reset"
#define SYS_FS_SAMPLE_TIME "/sys/class/input/input1/sample_time"
#define SYS_FS_ALS_ENABLE "/sys/class/input/input2/enable"
#define SYS_FS_ALS_NR_SAMPLES "/sys/class/input/input2/als_nr_samples"
#define SYS_FS_FLICKER_ENABLE "/sys/class/input/input3/enable"
#define SYS_FS_FLICKER_NR_SAMPLES "/sys/class/input/input3/fd_nr_samples"
#define SYS_FS_FLICKER_DEVICE_REGS "/sys/class/input/input4/regs"

/* Copied from ams_tcs3410_internal.h */
#define FIFO_DEPTH_EVENT ABS_VOLUME
#define SINT_EVENT ABS_DISTANCE

#define NUM_STEPS_ALS (3)
#define NUM_STATUS_REGS (3)

#define ALS_STATUS_REG_INDEX (0)  /* saturation info, scaled status  */
#define ALS_STATUS2_REG_INDEX (1) /* mod 0 and 1 gain */
#define ALS_STATUS3_REG_INDEX (2) /* mod 2 gain */

#define ALS_MEAS_SEQ_STEP_SHIFT (6)

#define ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT (5)
#define ALS_DATA1_ANALOG_SATURATION_STATUS_SHIFT (4)
#define ALS_DATA2_ANALOG_SATURATION_STATUS_SHIFT (3)

#define ALS_MEAS_SEQ_STEP_MASK (3 << ALS_MEAS_SEQ_STEP_SHIFT)

#define ALS_DATA0_ANALOG_SATURATION_STATUS_MASK (1 << ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT)
#define ALS_DATA1_ANALOG_SATURATION_STATUS_MASK (1 << ALS_DATA1_ANALOG_SATURATION_STATUS_SHIFT)
#define ALS_DATA2_ANALOG_SATURATION_STATUS_MASK (1 << ALS_DATA2_ANALOG_SATURATION_STATUS_SHIFT)

#define IR_COMP_RATIO_LO (0.078)
#define IR_COMP_RATIO_HI (0.131)

/* unit milliseconds */
#define MOD_CLOCK_STEP_MS (0.001388889)

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

using ::android::hardware::sensors::V1_0::MetaDataEventType;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorStatus;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::SensorInfo;
using ::android::hardware::sensors::V2_1::SensorType;

typedef enum {
    NOT_USED_MODE,
    ALS_ONLY_MODE,
    FLICKER_ONLY_MODE,
    ALS_FLICKER_MODE,
    LAST_MODE,
} ams_tcs3410_mode_t;

typedef enum {
    IDLE_STATE,
    WAITING_FOR_SINT,
    DATA_GATHER_IN_PROGRESS,
    DATA_PROCESSING,
    LAST_STATE,
} ams_tcs3410_event_state_t;

typedef enum {
    MODULATOR_0,
    MODULATOR_1,
    MODULATOR_2,
    NUM_MODULATORS,
} ams_tcs3410_modulator_t;

typedef enum {
    STEP_0,
    STEP_1,
    STEP_2,
    STEP_3,
    NUM_STEPS,
} ams_tcs3410_step_t;

typedef enum {
    RED_PD,
    GREEN_PD,
    BLUE_PD,
    WIDE_BAND_PD,
    CLEAR_PD,
    NUM_PD,
} ams_tcs3410_pd_filters_t;

typedef enum {
    RED_COEFF,
    GREEN_COEFF,
    BLUE_COEFF,
    WB_COEFF,
    CLEAR_COEFF,
    DGF,
} ams_tcs3410_coeffs_t;

typedef enum {
    COEFA,
    CTOFFSET,
} ams_tcs3410_cct_coeffs_t;

typedef enum {
    N_LO,
    N_MED,
    N_HI,
    N_MAX,
} ams_tcs3410_ircomp_c_t;

typedef struct _ams_tcs3410_als_data_t {
    uint32_t mod_counts[NUM_MODULATORS];          /* raw counts from the device */
    double mod_normalized_counts[NUM_MODULATORS]; /* normalized to a specific gain */
    uint8_t status[NUM_STATUS_REGS];              /* status registers from the device */
    uint8_t gains[NUM_MODULATORS];                /* register value of the gains */
    double mod_gains[NUM_MODULATORS];             /* numeric equiv of gains - 128x, 256x, etc */
    double mod_normalized_gains[NUM_MODULATORS];  /* normalized to the clear channel in step 0 mod
                                                     0*/
    double matching_factors; /* compare clear channel from each step: 0 -> 0, 1 -> 0 , 2 -> 0 */
} ams_tcs3410_als_data_t;

/* supported fifo formats  */
typedef enum _e_fifo_data_format {
    FIFO_FORMAT_UNCOMPRESSED = 1,
    FIFO_FORMAT_DIFFERENCE,
    FIFO_FORMAT_COMPRESSED,
    FIFO_FORMAT_DIFFERENCE_COMPRESSED,
    FIFO_FORMAT_MULTI_CHL_COMPRESSED,
    FIFO_FORMAT_MULTI_CHL_DIFFERENCE_COMPRESSED
} ams_fifo_format_t;

typedef struct _ams_tcs3410_input_t {
    ams_tcs3410_mode_t running_mode;
    ams_tcs3410_event_state_t event_state;
    int input_fd;
    int fifo_fd;
    bool run;
    int32_t fifo_data_sz;
    int32_t flicker_sample_freq;  // 1/(sample_time+1)
    int32_t fd_nr_samples;   // base 1 -> already incremented +1 , needs to be odd power of 2 - fft
                             // optimization
    int32_t als_nr_samples;  // base 1 -> already incremented +1
    int32_t sample_time;     // base 1 -> already incremented +1
    bool start_next_round;
    bool single_shot;
    uint16_t input_index;
    ams_fifo_format_t fifo_format;
    int32_t fg_frequency;
    FILE* log_fd;
} ams_tcs3410_input_t;

#define MAX_FQDN_LEN 255

/* Only defined for kernel headers */
/* use getpagesize() or sysconf(_SC_PAGESIZE)*/
#define USER_PAGE_SIZE (4096)

#define ALS_DATA_LENGTH (36)

#define MIN_FUNC_NAME_LEN (25)
#define MAX_FUNC_NAME_LEN (25)
#define MAX_LINE_NUM_LEN (4)
#define loggy(msg, ...)                                                            \
    ALOGE("HelloGoogle3 %*.*s(): %*d --> " msg " \n", MIN_FUNC_NAME_LEN, \
            MAX_FUNC_NAME_LEN, __func__, MAX_LINE_NUM_LEN, __LINE__, ##__VA_ARGS__)
#define log_color(COLOR, msg, ...)                                                            \
    ALOGE("HelloGoogle3 %s%*.*s(): %*d --> " msg " %s\n", COLOR, MIN_FUNC_NAME_LEN, \
            MAX_FUNC_NAME_LEN, __func__, MAX_LINE_NUM_LEN, __LINE__, ##__VA_ARGS__, NC)
#define log_raw(...) fprintf(stdout, __VA_ARGS__)

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

static const char* photodiode_connect[][3] = {
        {"CLEAR", "GREEN", "RED"},
        {"CLEAR", "BLUE", "WIDEBAND"},
        {"CLEAR", "BLUE", "RED"},
};

static const double lux_coeffs[][N_MAX] = {
        /* N_LO    N_MED   N_HI */
        [RED_COEFF] = {0.861, -1.37, 0.461},    [GREEN_COEFF] = {2.004, -1.197, -0.186},
        [BLUE_COEFF] = {0.132, -0.659, 0.778},  [WB_COEFF] = {0.671, 0.015, -0.051},
        [CLEAR_COEFF] = {-1.895, 1.808, 0.199}, [DGF] = {5.0, 5.0, 5.0},
};

static const double cct_coeffs[][N_MAX] = {
        /* N_LO    N_MED   N_HI */
        [COEFA] = {5239, 8096, 215},
        [CTOFFSET] = {1747, 1518, 2309},
};

static void show_als_data(ams_tcs3410_als_data_t als_data[]) {
    int step;

    for (step = 0; step < NUM_STEPS_ALS; step++) {
        log_color(BLUE,
                  "HelloGoogle3 Step %d:  mod_0(%s): {count:%5d, gain:0x%02X[%dx]}  mod_1(%s): "
                  "{count: %5d, "
                  "gain: 0x%02X[%dx]}  mod_2(%s): {count: %5d, gain: 0x%02X[%dx]}",
                  step, photodiode_connect[step][0], als_data[step].mod_counts[0],
                  als_data[step].gains[MODULATOR_0], (int)als_data[step].mod_gains[MODULATOR_0],
                  photodiode_connect[step][1], als_data[step].mod_counts[1],
                  als_data[step].gains[MODULATOR_1], (int)als_data[step].mod_gains[MODULATOR_1],
                  photodiode_connect[step][2], als_data[step].mod_counts[2],
                  als_data[step].gains[MODULATOR_2], (int)als_data[step].mod_gains[MODULATOR_2]);
    }

    return;
}

static bool check_for_saturation(ams_tcs3410_als_data_t als_data[]) {
    bool ret = false;
    uint16_t step;
    uint8_t ch0_sat, ch1_sat, ch2_sat, seq_step;

    for (step = 0; step < NUM_STEPS_ALS; step++) {
        seq_step = (als_data[step].status[ALS_STATUS_REG_INDEX] & ALS_MEAS_SEQ_STEP_MASK) >>
                   ALS_MEAS_SEQ_STEP_SHIFT;
        ch0_sat = (als_data[step].status[ALS_STATUS_REG_INDEX] &
                   ALS_DATA0_ANALOG_SATURATION_STATUS_MASK) >>
                  ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT;
        ch1_sat = (als_data[step].status[ALS_STATUS_REG_INDEX] &
                   ALS_DATA1_ANALOG_SATURATION_STATUS_MASK) >>
                  ALS_DATA1_ANALOG_SATURATION_STATUS_SHIFT;
        ch2_sat = (als_data[step].status[ALS_STATUS_REG_INDEX] &
                   ALS_DATA2_ANALOG_SATURATION_STATUS_MASK) >>
                  ALS_DATA0_ANALOG_SATURATION_STATUS_SHIFT;

        if (ch0_sat || ch1_sat || ch2_sat) {
            log_color(RED, "Saturation occured during ALS Sequence Step: %d (%d, %d, %d)", seq_step,
                      ch0_sat, ch1_sat, ch2_sat);
            ret = true;
            break;
        }
    }
    return (ret);
}

float calculate_lux(ams_tcs3410_input_t* pdevice_info, char* buffer) {
    uint16_t step, mods, status_regs, idx;
    ams_tcs3410_als_data_t als_data[NUM_STEPS_ALS];
    double average_counts[NUM_PD], ir_comp, ir_comp_ratio, atime_correction, lux, cct;
    uint8_t n;
    double atime;
    double r_prime;
    double b_prime;

#if defined(DEBUG_FIFO_RAW_DATA)
    for (idx = 1; idx <= ALS_DATA_LENGTH; idx++) {
        log_raw("\t0x%02X  ", buffer[idx - 1]);
        if ((idx % 9) == 0) {
            log_raw("\n");
        }
    }
#endif  // DEBUG_FIFO_RAW_DATA

    idx = 0;
    /* Convert Data Format */
    for (step = 0; step < NUM_STEPS_ALS; step++) {
        for (mods = 0; mods < NUM_MODULATORS; mods++) {
            als_data[step].mod_counts[mods] =
                    (((buffer[idx + 2] & 0xFF) << 16) | ((buffer[idx + 1] & 0xFF) << 8) |
                     ((buffer[idx] & 0xFF) << 0));
            idx += 3;
        }

        /* als_data[step].status[0] contains the saturation information */
        for (status_regs = 0; status_regs < NUM_STATUS_REGS; status_regs++) {
            als_data[step].status[status_regs] = buffer[idx++];
        }

        als_data[step].gains[MODULATOR_0] =
                ((als_data[step].status[ALS_STATUS2_REG_INDEX] & 0x0F) >> 0);
        als_data[step].gains[MODULATOR_1] =
                ((als_data[step].status[ALS_STATUS2_REG_INDEX] & 0xF0) >> 4);
        als_data[step].gains[MODULATOR_2] =
                ((als_data[step].status[ALS_STATUS3_REG_INDEX] & 0x0F) >> 0);

        als_data[step].mod_gains[MODULATOR_0] = (1 << (als_data[step].gains[MODULATOR_0] - 1));
        als_data[step].mod_gains[MODULATOR_1] = (1 << (als_data[step].gains[MODULATOR_1] - 1));
        als_data[step].mod_gains[MODULATOR_2] = (1 << (als_data[step].gains[MODULATOR_2] - 1));
    }

    show_als_data(&als_data[0]);
    if (check_for_saturation(&als_data[0])) {
        log_color(RED, "Terminating sequence due to analog saturation");
        return (FAILURE);
    }

    /* Start Normalizating the Channel Data */
    /* Calculate Gain ratios where step 0, mod 0 (Clear) is the reference */
    /* This logic only works because the reference is step 0, mod 0 */
    /* If you change the reference, this logic needs to change */
    for (step = 0; step < NUM_STEPS_ALS; step++) {
        for (mods = 0; mods < NUM_MODULATORS; mods++) {
            /* The reference gain remains constant and the counts are not adjusted*/
            if ((step == STEP_0) && (mods == MODULATOR_0)) {
                als_data[STEP_0].mod_normalized_gains[MODULATOR_0] =
                        als_data[STEP_0].mod_gains[MODULATOR_0];
                als_data[STEP_0].mod_normalized_counts[MODULATOR_0] =
                        (double)als_data[STEP_0].mod_counts[MODULATOR_0];
            } else {
                /* Normalize all other channel data to step 0, mod 0 */
                als_data[step].mod_normalized_gains[mods] =
                        als_data[step].mod_gains[mods] / als_data[STEP_0].mod_gains[MODULATOR_0];

                /* Normalize the [step,mod] count to that of its normalized gain */
                als_data[step].mod_normalized_counts[mods] =
                        (double)als_data[step].mod_counts[mods] /
                        als_data[step].mod_normalized_gains[mods];
            }
        }
    }

    /* Calculate matching factor for the clear channel that appears in all 3 ALS steps */
    als_data[0].matching_factors = 1;
    als_data[1].matching_factors =
            als_data[1].mod_normalized_counts[0] / als_data[0].mod_normalized_counts[0];
    als_data[2].matching_factors =
            als_data[2].mod_normalized_counts[0] / als_data[0].mod_normalized_counts[0];

    /* The atime can be corrected by using the flicker calculation during this round and adjust the
     * atime */
    /* atime_correction = atime/atime_next, where atime_next is determined by the detected flicker
     * frequency. */
    /* Vudu.....what if no flicker, what if 2 dominant frequencies, not science */
    atime_correction = 1.0;

    /* Correct ALS counts with channel matching factor and atime correction */
    for (step = 0; step < NUM_STEPS_ALS; step++) {
        for (mods = 0; mods < NUM_MODULATORS; mods++) {
            als_data[step].mod_normalized_counts[mods] = als_data[step].matching_factors *
                                                         atime_correction *
                                                         als_data[step].mod_normalized_counts[mods];
        }
    }

    /* Calculate the Average counts for RGB, WB, and Clear */
    average_counts[RED_PD] = ((double)(als_data[0].mod_normalized_counts[2] +
                                       als_data[2].mod_normalized_counts[2])) /
                             2.0;
    average_counts[GREEN_PD] = (double)(als_data[0].mod_normalized_counts[1]);
    average_counts[BLUE_PD] =
            (double)(als_data[1].mod_normalized_counts[1] + als_data[2].mod_normalized_counts[1]) /
            2.0;
    average_counts[WIDE_BAND_PD] = (double)(als_data[1].mod_normalized_counts[2]);
    average_counts[CLEAR_PD] =
            (double)(als_data[0].mod_normalized_counts[0] + als_data[1].mod_normalized_counts[0] +
                     als_data[2].mod_normalized_counts[0]) /
            3.0;

    ir_comp = (average_counts[RED_PD] + average_counts[GREEN_PD] + average_counts[BLUE_PD] -
               average_counts[CLEAR_PD]) /
              2.0;
    ir_comp_ratio = ir_comp / average_counts[CLEAR_PD];
    /* ir_comp_ratio= (average_counts[RED_PD] + average_counts[GREEN_PD] + average_counts[BLUE_PD] -
     * average_counts[CLEAR_PD])/(2.0 * average_counts[CLEAR_PD]); */
    if (ir_comp_ratio < IR_COMP_RATIO_LO) {
        n = N_LO;
    } else if ((ir_comp_ratio >= IR_COMP_RATIO_LO) && (ir_comp_ratio < IR_COMP_RATIO_HI)) {
        n = N_MED;
    } else {
        n = N_HI;
    }

    log_color(BLUE, "ir_comp_ratio = %.2f, coeff_index(n) = %d", ir_comp_ratio, n);
    log_color(RED, "RED: %.2f", average_counts[RED_PD]);
    log_color(GREEN, "GREEN: %.2f", average_counts[GREEN_PD]);
    log_color(BLUE, "BLUE: %.2f", average_counts[BLUE_PD]);
    log_color(YELLOW, "WIDEBAND: %.2f", average_counts[WIDE_BAND_PD]);
    log_color(NC, "CLEAR: %.2f", average_counts[CLEAR_PD]);

    /* values have already been adjusted for the + 1 */
    atime = (pdevice_info->sample_time) * (pdevice_info->als_nr_samples) * MOD_CLOCK_STEP_MS;

    log_color(BLUE, "atime = %.2f, again = %.2f, ir_comp = %.3f (%d)", atime,
              als_data[STEP_0].mod_normalized_gains[MODULATOR_0], ir_comp_ratio, n);
    lux = lux_coeffs[DGF][n] *
          ((lux_coeffs[RED_COEFF][n] * average_counts[RED_PD]) +
           (lux_coeffs[GREEN_COEFF][n] * average_counts[GREEN_PD]) +
           (lux_coeffs[BLUE_COEFF][n] * average_counts[BLUE_PD]) +
           (lux_coeffs[WB_COEFF][n] * average_counts[WIDE_BAND_PD]) +
           (lux_coeffs[CLEAR_COEFF][n] * average_counts[CLEAR_PD])) /
          (atime * als_data[STEP_0].mod_normalized_gains[MODULATOR_0]);

    /* calculate cct */
    r_prime = average_counts[RED_PD] - ir_comp;
    if (r_prime == 0) {
        r_prime = 1.0;
    }
    b_prime = average_counts[BLUE_PD] - ir_comp;
    cct = (cct_coeffs[COEFA][n] * (b_prime / r_prime)) + cct_coeffs[CTOFFSET][n];

#if defined(DEBUG_SHOW_LUX_EQ)
    log_raw("LUX = DGF * ((RED_Coeff * RED_Count) + (GREEN_Coeff * GREEN_Count) + (BLUE_Coeff * "
            "BLUE_Count) + (WB_Coeff * WB_Count) + CLEAR_Coeff * CLEAR_Count))/(atime * again))\n");
    log_raw("%.2f = %.3f * ((%.2f * %.2f) + (%.2f * %.2f) + (%.2f * %.2f) + (%.2f * %.2f) + (%.2f "
            "* %.2f))/(%.2f * %.2f)\n",
            lux, lux_coeffs[DGF][n], lux_coeffs[RED_COEFF][n], average_counts[RED_PD],
            lux_coeffs[GREEN_COEFF][n], average_counts[GREEN_PD], lux_coeffs[BLUE_COEFF][n],
            average_counts[BLUE_PD], lux_coeffs[WB_COEFF][n], average_counts[WIDE_BAND_PD],
            lux_coeffs[CLEAR_COEFF][n], average_counts[CLEAR_PD], atime,
            als_data[STEP_0].mod_normalized_gains[MODULATOR_0]);

    log_raw("CCT = (CoefA * (B' / R')) + CTOffset\n");
    log_raw("%.2f = (%.2f * (%.2f / %.2f)) + %.2f\n", cct, cct_coeffs[COEFA][n], b_prime, r_prime,
            cct_coeffs[CTOFFSET][n]);ZZ

#endif

    log_color(NC, "Calculated Lux: %.2f", lux);
    log_color(NC, "Calculated CCT: %.2f", cct);
    return cct;
}

static ams_tcs3410_input_t device_info = {
        .event_state = IDLE_STATE,
        .input_fd = -1,
        .fifo_fd = -1,
        .run = false,
        .fifo_data_sz = 0,
        .start_next_round = false,
        /* Command line params */
        .running_mode = ALS_ONLY_MODE,
        .single_shot = false,
        .input_index = 0,
        .fifo_format = FIFO_FORMAT_DIFFERENCE_COMPRESSED,
        .fg_frequency = -1,
        .log_fd = NULL,
};

static const char* event_state_2_str[] = {
        [IDLE_STATE] = "IDLE_STATE",
        [WAITING_FOR_SINT] = "WAITING_FOR_SINT",
        [DATA_GATHER_IN_PROGRESS] = "DATA_GATHER_IN_PROGRESS",
        [DATA_PROCESSING] = "DATA_PROCESSING",
        [LAST_STATE] = "LAST_STATE",
};

static char fifo_buffer[USER_PAGE_SIZE];

STATUS read_fifo(ams_tcs3410_input_t* pdevice_info, char* buffer) {
    int32_t bytes_read = 0;
    STATUS status = SUCCESS;

    /* Move buffer ptr to end of data */
    buffer += pdevice_info->fifo_data_sz;

    (void)lseek(pdevice_info->fifo_fd, 0, SEEK_SET);
    bytes_read = read(pdevice_info->fifo_fd, buffer, USER_PAGE_SIZE);

    pdevice_info->fifo_data_sz += bytes_read;

    loggy("\tread %d bytes, total = %d", bytes_read, pdevice_info->fifo_data_sz);

    return (status);
}

void write_to_sys_fs(int16_t value, const char* file) {
    char command[MAX_FQDN_LEN];
    FILE* fp = NULL;

    snprintf(command, sizeof(command), "echo \"%d\" > %s", value, file);

    loggy("Executing the following command: %s", command);
    fp = popen(command, "w");
    if (fp) {
        pclose(fp);
    }

    return;
}

void read_from_sys_fs(bool num, void* data, int sz, const char* file) {
    char command[MAX_FQDN_LEN];
    FILE* fp = NULL;

    snprintf(command, sizeof(command), "cat %s", file);

    loggy("Executing the following command: %s", command);
    fp = popen(command, "r");

    if (fp) {
        if (num) {
            int16_t* value = (int16_t*)data;
            fscanf(fp, "%hd", value);
        } else {
            char* str_data = (char*)data;
            int num_bytes = sz;
            while (fgets(str_data, num_bytes, fp) != NULL) {
                str_data += strlen(str_data);
                num_bytes -= strlen(str_data);
            }
        }
        pclose(fp);
    }

    return;
}

int32_t read_sample_time(void) {
    int16_t sample_time = -1;

    read_from_sys_fs(true, &sample_time, 0, SYS_FS_SAMPLE_TIME);

    /* register value is base 0 - increment by 1 */
    sample_time++;

    loggy("Sample Time is %d", sample_time);

    return (sample_time);
}

int32_t read_als_nr_samples(void) {
    int16_t als_samples = -1;

    read_from_sys_fs(true, &als_samples, 0, SYS_FS_ALS_NR_SAMPLES);

    /* register value is base 0 - increment by 1 */
    als_samples++;

    loggy("ALS nr sample is %d", als_samples);

    return (als_samples);
}

void enable_features(ams_tcs3410_mode_t feature) {
    switch (feature) {
        case ALS_ONLY_MODE: {
            write_to_sys_fs(1, SYS_FS_ALS_ENABLE);
            break;
        }
        case FLICKER_ONLY_MODE: {
            write_to_sys_fs(1, SYS_FS_FLICKER_ENABLE);
            break;
        }
        case ALS_FLICKER_MODE: {
            write_to_sys_fs(1, SYS_FS_ALS_ENABLE);
            write_to_sys_fs(1, SYS_FS_FLICKER_ENABLE);
            break;
        }
        default: {
            break;
        }
    }

    return;
}

void disable_features(ams_tcs3410_mode_t feature) {
    switch (feature) {
        case ALS_ONLY_MODE: {
            write_to_sys_fs(0, SYS_FS_ALS_ENABLE);
            break;
        }
        case FLICKER_ONLY_MODE: {
            write_to_sys_fs(0, SYS_FS_FLICKER_ENABLE);
            break;
        }
        case ALS_FLICKER_MODE: {
            write_to_sys_fs(0, SYS_FS_ALS_ENABLE);
            write_to_sys_fs(0, SYS_FS_FLICKER_ENABLE);
            break;
        }
        default: {
            break;
        }
    }

    return;
}

STATUS open_fifo(ams_tcs3410_input_t* pdevice_info) {
    STATUS status = FAILURE;

    if (pdevice_info) {
        pdevice_info->fifo_fd = open(SYS_FS_FIFO_NAME, O_RDONLY);
    }

    if (pdevice_info->fifo_fd > 0) {
        status = SUCCESS;
    }

    return (status);
}

STATUS close_fifo(ams_tcs3410_input_t* pdevice_info) {
    STATUS status = SUCCESS;

    if (pdevice_info->fifo_fd) {
        status = close(pdevice_info->fifo_fd);
        pdevice_info->fifo_fd = -1;
    }

    loggy("Closing fifo...");

    return (status);
}

static float process_fifo_data(ams_tcs3410_input_t* pdevice_info, char* buffer) {
    float cct = 0.0;
    if ((pdevice_info->running_mode == ALS_ONLY_MODE) ||
        (pdevice_info->running_mode == ALS_FLICKER_MODE)) {
        cct = calculate_lux(pdevice_info, buffer);
        /* Move pointers/length past ALS data */
        buffer += ALS_DATA_LENGTH;
        pdevice_info->fifo_data_sz -= ALS_DATA_LENGTH;
    }

    /*if ((pdevice_info->running_mode == FLICKER_ONLY_MODE) ||
        (pdevice_info->running_mode == ALS_FLICKER_MODE)) {
        calculate_freq(pdevice_info, buffer);
    }*/

    /* Create a boundary for each sequence round */
    log_raw("------------------------------------------------------------\n\n");

    return cct;
}

static STATUS open_input_device(ams_tcs3410_input_t* pdevice_info) {
    STATUS status = FAILURE;
    if (pdevice_info) {
        pdevice_info->input_fd = open(INPUT_DEVICE_BASE_NAME, O_RDONLY);
    }

    if (pdevice_info->input_fd > 0) {
        status = SUCCESS;
    }

    return (status);
}

static STATUS close_input_device(ams_tcs3410_input_t* pdevice_info) {
    STATUS status = SUCCESS;

    if (pdevice_info->input_fd) {
        status = close(pdevice_info->input_fd);
        pdevice_info->input_fd = -1;
    }

    loggy("Closing input event file.... Goodbye");

    return (status);
}

static float process_input_event(ams_tcs3410_input_t* pdevice_info, struct input_event* p_event) {
    float cct = 0.0;
    ams_tcs3410_event_state_t prev_state = pdevice_info->event_state;
    switch (p_event->type)  // EV_ABS = 0x3, EV_MSC = 0x04, EV_SYN = 0
    {
        case EV_ABS:  // 0x03
        {
            /* 2 codes defined for TCS3410 */
            if (p_event->code == SINT_EVENT) {
                loggy("SINT Event received");
                /* This event occurs at the end of every sequence round */
                /* The ALS data is the first 36 bytes, 12 bytes for each step */
                /* The remainder of the data is flicker, end bytes and flicker gains */
                /* */
                /* This event signifies the data can be processed.  Depending on the FFT */
                /* alogorithm, more flicker data may be needed. */
                if ((pdevice_info->event_state == IDLE_STATE) ||
                    (pdevice_info->event_state == DATA_GATHER_IN_PROGRESS)) {

                    /* Read the remaining data in the device fifo */
                    read_fifo(pdevice_info, fifo_buffer);

                    disable_features(pdevice_info->running_mode);

                    prev_state = pdevice_info->event_state;
                    pdevice_info->event_state = DATA_PROCESSING;
                    loggy("\tFrom:[%s] To:[%s]: Start processing the data...",
                          event_state_2_str[prev_state],
                          event_state_2_str[pdevice_info->event_state]);

                    cct = process_fifo_data(pdevice_info, fifo_buffer);

                    /* Set up for the next round */
                    pdevice_info->start_next_round = true;
                } else {
                    /* Ignore this event otherwise */
                    loggy("\tFrom:[%s]: To:[null] Ignoring SINT event",
                          event_state_2_str[pdevice_info->event_state]);
                }

                /* Read data from the sys_fs FIFO API */
            } else if (p_event->code == FIFO_DEPTH_EVENT) {
                /* This event occurs when data is retrieved from the device FIFO */
                /* and made available to the end user */
                loggy("FIFO Event received");
                if ((pdevice_info->event_state == IDLE_STATE) ||
                    (pdevice_info->event_state == DATA_GATHER_IN_PROGRESS)) {
                    pdevice_info->event_state = DATA_GATHER_IN_PROGRESS;
                    loggy("\tFrom:[%s] To:[%s]: Monitoring...", event_state_2_str[prev_state],
                          event_state_2_str[pdevice_info->event_state]);
                } else {
                    /* Ignore this event otherwise */
                    loggy("\tFrom:[%s]: To:[null] Ignoring FINT event",
                          event_state_2_str[pdevice_info->event_state]);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return cct;
}

static float read_input_events(ams_tcs3410_input_t* pdevice_info, fd_set* p_fds) {
    struct input_event event;
    float cct = 0.0;
    /* Only one file descriptor is set, but make sure it is the correct one */
    if ((pdevice_info->input_fd > 0) && FD_ISSET(pdevice_info->input_fd, p_fds)) {
        if (read(pdevice_info->input_fd, &event, sizeof(struct input_event)) > 0) {
            cct = process_input_event(pdevice_info, &event);
        } else {
            perror("read(): Failed to read input event: ");
        }
    }
    return cct;
}

static void close_log_file(ams_tcs3410_input_t* pdevice_info) {
    if (pdevice_info->log_fd != NULL) {
        loggy("Closing log file...");
        fclose(pdevice_info->log_fd);
    }
    return;
}

Sensor::Sensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : mIsEnabled(false),
      mSamplingPeriodNs(0),
      mLastSampleTimeNs(0),
      mCallback(callback),
      mMode(OperationMode::NORMAL) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.vendor = "Vendor String";
    mSensorInfo.version = 1;
    constexpr float kDefaultMaxDelayUs = 1000 * 1000;
    mSensorInfo.maxDelay = kDefaultMaxDelayUs;
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
    mRunThread = std::thread(startThread, this);
}

Sensor::~Sensor() {
    // Ensure that lock is unlocked before calling mRunThread.join() or a
    // deadlock will occur.
    {
        std::unique_lock<std::mutex> lock(mRunMutex);
        mStopThread = true;
        mIsEnabled = false;
        mWaitCV.notify_all();
    }
    mRunThread.join();
}

const SensorInfo& Sensor::getSensorInfo() const {
    return mSensorInfo;
}

void Sensor::batch(int32_t samplingPeriodNs) {
    samplingPeriodNs =
            std::clamp(samplingPeriodNs, mSensorInfo.minDelay * 1000, mSensorInfo.maxDelay * 1000);

    if (mSamplingPeriodNs != samplingPeriodNs) {
        mSamplingPeriodNs = samplingPeriodNs;
        // Wake up the 'run' thread to check if a new event should be generated now
        mWaitCV.notify_all();
    }
}

void Sensor::activate(bool enable) {
    if (mIsEnabled != enable) {
        std::unique_lock<std::mutex> lock(mRunMutex);
        mIsEnabled = enable;
        mWaitCV.notify_all();
    }
}

Result Sensor::flush() {
    // Only generate a flush complete event if the sensor is enabled and if the sensor is not a
    // one-shot sensor.
    if (!mIsEnabled || (mSensorInfo.flags & static_cast<uint32_t>(SensorFlagBits::ONE_SHOT_MODE))) {
        return Result::BAD_VALUE;
    }

    // Note: If a sensor supports batching, write all of the currently batched events for the sensor
    // to the Event FMQ prior to writing the flush complete event.
    Event ev;
    ev.sensorHandle = mSensorInfo.sensorHandle;
    ev.sensorType = SensorType::META_DATA;
    ev.u.meta.what = MetaDataEventType::META_DATA_FLUSH_COMPLETE;
    std::vector<Event> evs{ev};
    mCallback->postEvents(evs, isWakeUpSensor());

    return Result::OK;
}

void Sensor::startThread(Sensor* sensor) {
    sensor->run();
}

void Sensor::run() {
    std::unique_lock<std::mutex> runLock(mRunMutex);
    constexpr int64_t kNanosecondsInSeconds = 1000 * 1000 * 1000;

    while (!mStopThread) {
        if (!mIsEnabled || mMode == OperationMode::DATA_INJECTION) {
            mWaitCV.wait(runLock, [&] {
                return ((mIsEnabled && mMode == OperationMode::NORMAL) || mStopThread);
            });
        } else {
            timespec curTime;
            clock_gettime(CLOCK_REALTIME, &curTime);
            int64_t now = (curTime.tv_sec * kNanosecondsInSeconds) + curTime.tv_nsec;
            int64_t nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;

            if (now >= nextSampleTime) {
                mLastSampleTimeNs = now;
                nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;
                mCallback->postEvents(readEvents(), isWakeUpSensor());
            }

            mWaitCV.wait_for(runLock, std::chrono::nanoseconds(nextSampleTime - now));
        }
    }
}

bool Sensor::isWakeUpSensor() {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorFlagBits::WAKE_UP);
}

std::vector<Event> Sensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    event.u.vec3.x = 0;
    event.u.vec3.y = 0;
    event.u.vec3.z = 0;
    event.u.vec3.status = SensorStatus::ACCURACY_HIGH;
    events.push_back(event);
    return events;
}

void Sensor::setOperationMode(OperationMode mode) {
    if (mMode != mode) {
        std::unique_lock<std::mutex> lock(mRunMutex);
        mMode = mode;
        mWaitCV.notify_all();
    }
}

bool Sensor::supportsDataInjection() const {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorFlagBits::DATA_INJECTION);
}

Result Sensor::injectEvent(const Event& event) {
    Result result = Result::OK;
    if (event.sensorType == SensorType::ADDITIONAL_INFO) {
        // When in OperationMode::NORMAL, SensorType::ADDITIONAL_INFO is used to push operation
        // environment data into the device.
    } else if (!supportsDataInjection()) {
        result = Result::INVALID_OPERATION;
    } else if (mMode == OperationMode::DATA_INJECTION) {
        mCallback->postEvents(std::vector<Event>{event}, isWakeUpSensor());
    } else {
        result = Result::BAD_VALUE;
    }
    return result;
}

OnChangeSensor::OnChangeSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : Sensor(sensorHandle, callback), mPreviousEventSet(false) {
    mSensorInfo.flags |= SensorFlagBits::ON_CHANGE_MODE;
}

void OnChangeSensor::activate(bool enable) {
    Sensor::activate(enable);
    if (!enable) {
        mPreviousEventSet = false;
    }
}

std::vector<Event> OnChangeSensor::readEvents() {
    std::vector<Event> events = Sensor::readEvents();
    std::vector<Event> outputEvents;

    for (auto iter = events.begin(); iter != events.end(); ++iter) {
        Event ev = *iter;
        if (ev.u.vec3 != mPreviousEvent.u.vec3 || !mPreviousEventSet) {
            outputEvents.push_back(ev);
            mPreviousEvent = ev;
            mPreviousEventSet = true;
        }
    }
    return outputEvents;
}

ContinuousSensor::ContinuousSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : Sensor(sensorHandle, callback) {
    mSensorInfo.flags |= SensorFlagBits::CONTINUOUS_MODE;
}

AccelSensor::AccelSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : ContinuousSensor(sensorHandle, callback) {
    mSensorInfo.name = "Accel Sensor";
    mSensorInfo.type = SensorType::ACCELEROMETER;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_ACCELEROMETER;
    mSensorInfo.maxRange = 78.4f;  // +/- 8g
    mSensorInfo.resolution = 1.52e-5;
    mSensorInfo.power = 0.001f;        // mA
    mSensorInfo.minDelay = 20 * 1000;  // microseconds
    mSensorInfo.flags |= SensorFlagBits::DATA_INJECTION;
}

std::vector<Event> AccelSensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    event.u.vec3.x = 0;
    event.u.vec3.y = 0;
    event.u.vec3.z = -9.815;
    event.u.vec3.status = SensorStatus::ACCURACY_HIGH;
    events.push_back(event);
    return events;
}

PressureSensor::PressureSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : ContinuousSensor(sensorHandle, callback) {
    mSensorInfo.name = "Pressure Sensor";
    mSensorInfo.type = SensorType::PRESSURE;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_PRESSURE;
    mSensorInfo.maxRange = 1100.0f;     // hPa
    mSensorInfo.resolution = 0.005f;    // hPa
    mSensorInfo.power = 0.001f;         // mA
    mSensorInfo.minDelay = 100 * 1000;  // microseconds
}

MagnetometerSensor::MagnetometerSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : ContinuousSensor(sensorHandle, callback) {
    mSensorInfo.name = "Magnetic Field Sensor";
    mSensorInfo.type = SensorType::MAGNETIC_FIELD;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_MAGNETIC_FIELD;
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;        // mA
    mSensorInfo.minDelay = 20 * 1000;  // microseconds
}

LightSensor::LightSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(sensorHandle, callback) {
    mSensorInfo.name = "Light Sensor";
    mSensorInfo.type = SensorType::LIGHT;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_LIGHT;
    mSensorInfo.maxRange = 43000.0f;
    mSensorInfo.resolution = 10.0f;
    mSensorInfo.power = 0.001f;          // mA
    mSensorInfo.minDelay = 2000 * 1000;  // microseconds

    if (open_input_device(&device_info) == FAILURE) {
        close_input_device(&device_info);
        close_fifo(&device_info);
        disable_features(device_info.running_mode);
        close_log_file(&device_info);
    }

    device_info.run = true;
    device_info.sample_time = read_sample_time();
    device_info.als_nr_samples = read_als_nr_samples();
    device_info.fd_nr_samples = -1;
    device_info.flicker_sample_freq = -1;

    open_fifo(&device_info);

    device_info.event_state = IDLE_STATE;
    memset(fifo_buffer, 0, sizeof(fifo_buffer));

}

std::vector<Event> LightSensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();

    enable_features(device_info.running_mode);

    fd_set fds;
    int max_fd = device_info.input_fd;
    struct timeval timeout;
    int ret;

    FD_ZERO(&fds);
    FD_SET(device_info.input_fd, &fds);
    /* needs to be 1 more than the largets fd in the select cell */
    max_fd++;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    ret = select(max_fd, &fds, NULL, NULL, &timeout);
    if (ret > 0) {
      float cct = read_input_events(&device_info, &fds);

      event.u.vec3.x = cct;
      event.u.vec3.y = 0;
      event.u.vec3.z = cct;
      events.push_back(event);

      if (device_info.start_next_round) {
        device_info.event_state = IDLE_STATE;
        device_info.fifo_data_sz = 0;
        device_info.start_next_round = false;
      }

      if (device_info.run) {
        enable_features(device_info.running_mode);
      }
    }
    return events;
}

ProximitySensor::ProximitySensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(sensorHandle, callback) {
    mSensorInfo.name = "Proximity Sensor";
    mSensorInfo.type = SensorType::PROXIMITY;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_PROXIMITY;
    mSensorInfo.maxRange = 5.0f;
    mSensorInfo.resolution = 1.0f;
    mSensorInfo.power = 0.012f;         // mA
    mSensorInfo.minDelay = 200 * 1000;  // microseconds
    mSensorInfo.flags |= SensorFlagBits::WAKE_UP;
}

GyroSensor::GyroSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : ContinuousSensor(sensorHandle, callback) {
    mSensorInfo.name = "Gyro Sensor";
    mSensorInfo.type = SensorType::GYROSCOPE;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_GYROSCOPE;
    mSensorInfo.maxRange = 1000.0f * M_PI / 180.0f;
    mSensorInfo.resolution = 1000.0f * M_PI / (180.0f * 32768.0f);
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelay = 2.5f * 1000;  // microseconds
}

std::vector<Event> GyroSensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    event.u.vec3.x = 0;
    event.u.vec3.y = 0;
    event.u.vec3.z = 0;
    event.u.vec3.status = SensorStatus::ACCURACY_HIGH;
    events.push_back(event);
    return events;
}

AmbientTempSensor::AmbientTempSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(sensorHandle, callback) {
    mSensorInfo.name = "Ambient Temp Sensor";
    mSensorInfo.type = SensorType::AMBIENT_TEMPERATURE;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_AMBIENT_TEMPERATURE;
    mSensorInfo.maxRange = 80.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelay = 40 * 1000;  // microseconds
}

DeviceTempSensor::DeviceTempSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : ContinuousSensor(sensorHandle, callback) {
    mSensorInfo.name = "Device Temp Sensor";
    mSensorInfo.type = SensorType::TEMPERATURE;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_TEMPERATURE;
    mSensorInfo.maxRange = 80.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelay = 40 * 1000;  // microseconds
}

RelativeHumiditySensor::RelativeHumiditySensor(int32_t sensorHandle,
                                               ISensorsEventCallback* callback)
    : OnChangeSensor(sensorHandle, callback) {
    mSensorInfo.name = "Relative Humidity Sensor";
    mSensorInfo.type = SensorType::RELATIVE_HUMIDITY;
    mSensorInfo.typeAsString = SENSOR_STRING_TYPE_RELATIVE_HUMIDITY;
    mSensorInfo.maxRange = 100.0f;
    mSensorInfo.resolution = 0.1f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelay = 40 * 1000;  // microseconds
}

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
