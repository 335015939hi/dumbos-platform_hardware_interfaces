/******************************************************************************
 *
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
 */

#ifndef __VEHICLE_MANAGER_FUZZER_H__
#define __VEHICLE_MANAGER_FUZZER_H__

#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/VmsUtils.h>

#include <VehicleHalTestUtils.h>
#include <fuzzer/FuzzedDataProvider.h>

namespace vehicleManagerFuzzer {

using ::android::sp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::automotive::vehicle::V2_0::DiagnosticFloatSensorIndex;
using ::android::hardware::automotive::vehicle::V2_0::DiagnosticIntegerSensorIndex;
using ::android::hardware::automotive::vehicle::V2_0::kCustomComplexProperty;
using ::android::hardware::automotive::vehicle::V2_0::kVehicleProperties;
using ::android::hardware::automotive::vehicle::V2_0::MockedVehicleCallback;
using ::android::hardware::automotive::vehicle::V2_0::recyclable_ptr;
using ::android::hardware::automotive::vehicle::V2_0::StatusCode;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeFlags;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeOptions;
using ::android::hardware::automotive::vehicle::V2_0::VehicleAreaConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleHal;
using ::android::hardware::automotive::vehicle::V2_0::VehicleHalManager;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleProperty;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyAccess;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyChangeMode;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyStore;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyType;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValue;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValuePool;
using ::android::hardware::automotive::vehicle::V2_0::VmsMessageType;
using ::android::hardware::automotive::vehicle::V2_0::vms::createAvailabilityRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::createBaseVmsMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::createPublisherIdRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::createStartSessionMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::createSubscriptionsRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::getAvailableLayers;
using ::android::hardware::automotive::vehicle::V2_0::vms::getSequenceNumberForAvailabilityState;
using ::android::hardware::automotive::vehicle::V2_0::vms::getSequenceNumberForSubscriptionsState;
using ::android::hardware::automotive::vehicle::V2_0::vms::hasServiceNewlyStarted;
using ::android::hardware::automotive::vehicle::V2_0::vms::isAvailabilitySequenceNumberNewer;
using ::android::hardware::automotive::vehicle::V2_0::vms::isSequenceNumberNewer;
using ::android::hardware::automotive::vehicle::V2_0::vms::isValidVmsMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseData;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseMessageType;
using ::android::hardware::automotive::vehicle::V2_0::vms::parsePublisherIdResponse;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseStartSessionMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayer;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayerAndPublisher;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayerOffering;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsOffers;

constexpr const char kCarMake[] = "Default Car";
constexpr int kRetriableAttempts = 3;
constexpr VehicleProperty kVehicleProp[] = {VehicleProperty::INVALID,
                                            VehicleProperty::HVAC_FAN_SPEED,
                                            VehicleProperty::INFO_MAKE,
                                            VehicleProperty::DISPLAY_BRIGHTNESS,
                                            VehicleProperty::INFO_FUEL_CAPACITY,
                                            VehicleProperty::HVAC_SEAT_TEMPERATURE};
constexpr DiagnosticIntegerSensorIndex kDiagnosticIntIndex[] = {
        DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS,
        DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON,
        DiagnosticIntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT,
        DiagnosticIntegerSensorIndex::FUEL_TYPE};
constexpr DiagnosticFloatSensorIndex kDiagnosticFloatIndex[] = {
        DiagnosticFloatSensorIndex::CALCULATED_ENGINE_LOAD,
        DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::THROTTLE_POSITION};
constexpr size_t kVehiclePropArrayLength = std::size(kVehicleProp);
constexpr size_t kIntSensorArrayLength = std::size(kDiagnosticIntIndex);
constexpr size_t kFloatSensorArrayLength = std::size(kDiagnosticFloatIndex);
constexpr VmsMessageType kAvailabilityMessageType[] = {VmsMessageType::AVAILABILITY_CHANGE,
                                                       VmsMessageType::AVAILABILITY_RESPONSE};
constexpr VmsMessageType kSubscriptionMessageType[] = {VmsMessageType::SUBSCRIPTIONS_CHANGE,
                                                       VmsMessageType::SUBSCRIPTIONS_RESPONSE};

class MockedVehicleHal : public VehicleHal {
  public:
    MockedVehicleHal() {
        mConfigs.assign(std::begin(kVehicleProperties), std::end(kVehicleProperties));
    }

    std::vector<VehiclePropConfig> listProperties() override { return mConfigs; }

    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override;

    StatusCode set(const VehiclePropValue& propValue) override {
        if (toInt(VehicleProperty::MIRROR_FOLD) == propValue.prop && mirrorFoldAttemptsLeft-- > 0) {
            return StatusCode::TRY_AGAIN;
        }

        mValues[makeKey(propValue)] = propValue;
        return StatusCode::OK;
    }

    StatusCode subscribe(int32_t /* property */, float /* sampleRate */) override {
        return StatusCode::OK;
    }

    StatusCode unsubscribe(int32_t /* property */) override { return StatusCode::OK; }

    void sendPropEvent(recyclable_ptr<VehiclePropValue> value) { doHalEvent(std::move(value)); }

    void sendHalError(StatusCode error, int32_t property, int32_t areaId) {
        doHalPropertySetError(error, property, areaId);
    }

  public:
    int fuelCapacityAttemptsLeft = kRetriableAttempts;
    int mirrorFoldAttemptsLeft = kRetriableAttempts;

  private:
    int64_t makeKey(const VehiclePropValue& v) const { return makeKey(v.prop, v.areaId); }

    int64_t makeKey(int32_t prop, int32_t area) const {
        return (static_cast<int64_t>(prop) << 32) | area;
    }

  private:
    std::vector<VehiclePropConfig> mConfigs;
    std::unordered_map<int64_t, VehiclePropValue> mValues;
};

class VehicleHalManagerFuzzer {
  public:
    VehicleHalManagerFuzzer() {
        mHal.reset(new MockedVehicleHal);
        mManager.reset(new VehicleHalManager(mHal.get()));
        mObjectPool = mHal->getValuePool();
    }
    ~VehicleHalManagerFuzzer() {
        mManager.reset(nullptr);
        mHal.reset(nullptr);
        mObjectPool = nullptr;
        if (mFuzzedDataProvider) {
            delete mFuzzedDataProvider;
        }
    }
    void process(const uint8_t* data, size_t size);

  private:
    FuzzedDataProvider* mFuzzedDataProvider = nullptr;
    VehiclePropValue mActualValue = VehiclePropValue{};
    StatusCode mActualStatusCode = StatusCode::OK;

    VehiclePropValuePool* mObjectPool = nullptr;
    std::unique_ptr<MockedVehicleHal> mHal;
    std::unique_ptr<VehicleHalManager> mManager;

    void invokeDebug();
    void invokePropConfigs();
    void invokeSubscribe();
    void invokeSetAndGetValues();
    void invokeObd2SensorStore();
    void invokeVmsUtils();
    void invokeVehiclePropStore();
    void invokeWatchDogClient();
    void invokeGetSubscribedLayers(VmsMessageType type);
    void invokeGet(int32_t property, int32_t areaId);
};

}  // namespace vehicleManagerFuzzer

#endif  // __VEHICLE_MANAGER_FUZZER_H__
