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

#include <Constants.h>
#include <MockLocation.h>
#include <Utils.h>
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using aidl::android::hardware::gnss::ElapsedRealtime;
using aidl::android::hardware::gnss::GnssClock;
using aidl::android::hardware::gnss::GnssConstellationType;
using aidl::android::hardware::gnss::GnssData;
using aidl::android::hardware::gnss::GnssLocation;
using aidl::android::hardware::gnss::GnssMeasurement;
using aidl::android::hardware::gnss::IGnss;
using aidl::android::hardware::gnss::IGnssDebug;
using aidl::android::hardware::gnss::IGnssMeasurementCallback;
using aidl::android::hardware::gnss::SatellitePvt;
using GnssSvInfo = aidl::android::hardware::gnss::IGnssCallback::GnssSvInfo;
using GnssSvFlags = aidl::android::hardware::gnss::IGnssCallback::GnssSvFlags;

using GnssAgc = aidl::android::hardware::gnss::GnssData::GnssAgc;

GnssData Utils::getMockMeasurement(const bool enableCorrVecOutputs) {
    aidl::android::hardware::gnss::GnssSignalType signalType = {
            .constellation = GnssConstellationType::GLONASS,
            .carrierFrequencyHz = 1.59975e+09,
            .codeType = aidl::android::hardware::gnss::GnssSignalType::CODE_TYPE_C,
    };
    GnssMeasurement measurement = {
            .flags = GnssMeasurement::HAS_AUTOMATIC_GAIN_CONTROL |
                     GnssMeasurement::HAS_CARRIER_FREQUENCY | GnssMeasurement::HAS_CARRIER_PHASE |
                     GnssMeasurement::HAS_CARRIER_PHASE_UNCERTAINTY |
                     GnssMeasurement::HAS_FULL_ISB | GnssMeasurement::HAS_FULL_ISB_UNCERTAINTY |
                     GnssMeasurement::HAS_SATELLITE_ISB |
                     GnssMeasurement::HAS_SATELLITE_ISB_UNCERTAINTY |
                     GnssMeasurement::HAS_SATELLITE_PVT,
            .svid = 13,
            .signalType = signalType,
            .receivedSvTimeInNs = 8195997131077,
            .receivedSvTimeUncertaintyInNs = 15,
            .antennaCN0DbHz = 30.0,
            .basebandCN0DbHz = 26.5,
            .agcLevelDb = 2.3,
            .pseudorangeRateMps = -484.13739013671875,
            .pseudorangeRateUncertaintyMps = 1.0379999876022339,
            .accumulatedDeltaRangeState = GnssMeasurement::ADR_STATE_UNKNOWN,
            .accumulatedDeltaRangeM = 1.52,
            .accumulatedDeltaRangeUncertaintyM = 2.43,
            .multipathIndicator = aidl::android::hardware::gnss::GnssMultipathIndicator::UNKNOWN,
            .state = GnssMeasurement::STATE_CODE_LOCK | GnssMeasurement::STATE_BIT_SYNC |
                     GnssMeasurement::STATE_SUBFRAME_SYNC | GnssMeasurement::STATE_TOW_DECODED |
                     GnssMeasurement::STATE_GLO_STRING_SYNC |
                     GnssMeasurement::STATE_GLO_TOD_DECODED,
            .fullInterSignalBiasNs = 21.5,
            .fullInterSignalBiasUncertaintyNs = 792.0,
            .satelliteInterSignalBiasNs = 233.9,
            .satelliteInterSignalBiasUncertaintyNs = 921.2,
            .satellitePvt =
                    {
                            .flags = SatellitePvt::HAS_POSITION_VELOCITY_CLOCK_INFO |
                                     SatellitePvt::HAS_IONO | SatellitePvt::HAS_TROPO,
                            .satPosEcef = {.posXMeters = 10442993.1153328,
                                           .posYMeters = -19926932.8051666,
                                           .posZMeters = -12034295.0216203,
                                           .ureMeters = 1000.2345678},
                            .satVelEcef = {.velXMps = -478.667183715732,
                                           .velYMps = 1580.68371984114,
                                           .velZMps = -3030.52994449997,
                                           .ureRateMps = 10.2345678},
                            .satClockInfo = {.satHardwareCodeBiasMeters = 1.396983861923e-09,
                                             .satTimeCorrectionMeters = -7113.08964331,
                                             .satClkDriftMps = 0},
                            .ionoDelayMeters = 3.069949602639317e-08,
                            .tropoDelayMeters = 3.882265204404031,
                            .ephemerisSource =
                                    SatellitePvt::SatelliteEphemerisSource::SERVER_LONG_TERM,
                            .timeOfClockSeconds = 12345,
                            .issueOfDataClock = 143,
                            .timeOfEphemerisSeconds = 9876,
                            .issueOfDataEphemeris = 48,
                    },
            .correlationVectors = {}};

    GnssClock clock = {.gnssClockFlags = GnssClock::HAS_FULL_BIAS | GnssClock::HAS_BIAS |
                                         GnssClock::HAS_BIAS_UNCERTAINTY | GnssClock::HAS_DRIFT |
                                         GnssClock::HAS_DRIFT_UNCERTAINTY,
                       .timeNs = 2713545000000,
                       .fullBiasNs = -1226701900521857520,
                       .biasNs = 0.59689998626708984,
                       .biasUncertaintyNs = 47514.989972114563,
                       .driftNsps = -51.757811607455452,
                       .driftUncertaintyNsps = 310.64968328491528,
                       .hwClockDiscontinuityCount = 1,
                       .referenceSignalTypeForIsb = signalType};

    ElapsedRealtime timestamp = {
            .flags = ElapsedRealtime::HAS_TIMESTAMP_NS | ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = ::android::elapsedRealtimeNano(),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1020400};

    if (enableCorrVecOutputs) {
        aidl::android::hardware::gnss::CorrelationVector correlationVector1 = {
                .frequencyOffsetMps = 10,
                .samplingWidthM = 30,
                .samplingStartM = 0,
                .magnitude = {0, 5000, 10000, 5000, 0, 0, 3000, 0}};
        aidl::android::hardware::gnss::CorrelationVector correlationVector2 = {
                .frequencyOffsetMps = 20,
                .samplingWidthM = 30,
                .samplingStartM = -10,
                .magnitude = {0, 3000, 5000, 3000, 0, 0, 1000, 0}};
        measurement.correlationVectors = {correlationVector1, correlationVector2};
        measurement.flags |= GnssMeasurement::HAS_CORRELATION_VECTOR;
    }

    GnssAgc gnssAgc1 = {
            .agcLevelDb = 3.5,
            .constellation = GnssConstellationType::GLONASS,
            .carrierFrequencyHz = (int64_t)kGloG1FreqHz,
    };

    GnssAgc gnssAgc2 = {
            .agcLevelDb = -5.1,
            .constellation = GnssConstellationType::GPS,
            .carrierFrequencyHz = (int64_t)kGpsL1FreqHz,
    };

    GnssData gnssData = {.measurements = {measurement},
                         .clock = clock,
                         .elapsedRealtime = timestamp,
                         .gnssAgcs = std::vector({gnssAgc1, gnssAgc2})};
    return gnssData;
}

GnssLocation Utils::getMockLocation() {
    ElapsedRealtime elapsedRealtime = {
            .flags = ElapsedRealtime::HAS_TIMESTAMP_NS | ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = ::android::elapsedRealtimeNano(),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1020400};
    GnssLocation location = {.gnssLocationFlags = 0xFF,
                             .latitudeDegrees = gMockLatitudeDegrees,
                             .longitudeDegrees = gMockLongitudeDegrees,
                             .altitudeMeters = gMockAltitudeMeters,
                             .speedMetersPerSec = gMockSpeedMetersPerSec,
                             .bearingDegrees = gMockBearingDegrees,
                             .horizontalAccuracyMeters = kMockHorizontalAccuracyMeters,
                             .verticalAccuracyMeters = kMockVerticalAccuracyMeters,
                             .speedAccuracyMetersPerSecond = kMockSpeedAccuracyMetersPerSecond,
                             .bearingAccuracyDegrees = kMockBearingAccuracyDegrees,
                             .timestampMillis = static_cast<int64_t>(
                                     kMockTimestamp + ::android::elapsedRealtimeNano() / 1e6),
                             .elapsedRealtime = elapsedRealtime};
    return location;
}

namespace {
GnssSvInfo getMockSvInfo(int svid, GnssConstellationType type, float cN0DbHz, float basebandCN0DbHz,
                         float elevationDegrees, float azimuthDegrees, long carrierFrequencyHz) {
    GnssSvInfo svInfo = {
            .svid = svid,
            .constellation = type,
            .cN0Dbhz = cN0DbHz,
            .basebandCN0DbHz = basebandCN0DbHz,
            .elevationDegrees = elevationDegrees,
            .azimuthDegrees = azimuthDegrees,
            .carrierFrequencyHz = carrierFrequencyHz,
            .svFlag = (int)GnssSvFlags::USED_IN_FIX | (int)GnssSvFlags::HAS_EPHEMERIS_DATA |
                      (int)GnssSvFlags::HAS_ALMANAC_DATA | (int)GnssSvFlags::HAS_CARRIER_FREQUENCY};
    return svInfo;
}
}  // anonymous namespace

std::vector<GnssSvInfo> Utils::getMockSvInfoList() {
    std::vector<GnssSvInfo> gnssSvInfoList = {
            getMockSvInfo(3, GnssConstellationType::GPS, 32.5, 27.5, 59.1, 166.5, kGpsL1FreqHz),
            getMockSvInfo(5, GnssConstellationType::GPS, 27.0, 22.0, 29.0, 56.5, kGpsL1FreqHz),
            getMockSvInfo(17, GnssConstellationType::GPS, 30.5, 25.5, 71.0, 77.0, kGpsL5FreqHz),
            getMockSvInfo(26, GnssConstellationType::GPS, 24.1, 19.1, 28.0, 253.0, kGpsL5FreqHz),
            getMockSvInfo(5, GnssConstellationType::GLONASS, 20.5, 15.5, 11.5, 116.0, kGloG1FreqHz),
            getMockSvInfo(17, GnssConstellationType::GLONASS, 21.5, 16.5, 28.5, 186.0,
                          kGloG1FreqHz),
            getMockSvInfo(18, GnssConstellationType::GLONASS, 28.3, 25.3, 38.8, 69.0, kGloG1FreqHz),
            getMockSvInfo(10, GnssConstellationType::GLONASS, 25.0, 20.0, 66.0, 247.0,
                          kGloG1FreqHz),
            getMockSvInfo(3, GnssConstellationType::IRNSS, 22.0, 19.7, 35.0, 112.0, kIrnssL5FreqHz),
    };
    return gnssSvInfoList;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
