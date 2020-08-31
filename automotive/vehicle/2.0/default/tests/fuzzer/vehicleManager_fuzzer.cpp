/******************************************************************************
 *
 * Copyright (C) 2020 The Android Open Source Project
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

#include <utils/SystemClock.h>
#include <vhal_v2_0/Obd2SensorStore.h>
#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/VmsUtils.h>
#include <vhal_v2_0/WatchdogClient.h>

#include <VehicleHalTestUtils.h>
#include <fuzzer/FuzzedDataProvider.h>

using namespace android::hardware::automotive::vehicle::V2_0;
using aidl::android::automotive::watchdog::TimeoutLength;
using VehicleProperty = ::android::hardware::automotive::vehicle::V2_0::VehicleProperty;
using DiagnosticIntegerSensorIndex =
        android::hardware::automotive::vehicle::V2_0::DiagnosticIntegerSensorIndex;
using DiagnosticFloatSensorIndex =
        android::hardware::automotive::vehicle::V2_0::DiagnosticFloatSensorIndex;
using ::android::hardware::hidl_vec;
using namespace android;
using namespace std::placeholders;

const char kCarMake[] = "Default Car";
const int kRetriableAttempts = 3;
const VehicleProperty kVehicleProp[] = {VehicleProperty::INVALID,
                                        VehicleProperty::HVAC_FAN_SPEED,
                                        VehicleProperty::INFO_MAKE,
                                        VehicleProperty::DISPLAY_BRIGHTNESS,
                                        VehicleProperty::INFO_FUEL_CAPACITY,
                                        VehicleProperty::HVAC_SEAT_TEMPERATURE};
const DiagnosticIntegerSensorIndex kDiagnosticIntIndex[] = {
        DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS,
        DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON,
        DiagnosticIntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT,
        DiagnosticIntegerSensorIndex::FUEL_TYPE};
const DiagnosticFloatSensorIndex kDiagnosticFloatIndex[] = {
        DiagnosticFloatSensorIndex::CALCULATED_ENGINE_LOAD,
        DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::THROTTLE_POSITION};
const size_t kVehiclePropArrayLength = std::size(kVehicleProp);
const size_t kIntSensorArrayLength = std::size(kDiagnosticIntIndex);
const size_t kFloatSensorArrayLength = std::size(kDiagnosticFloatIndex);
const VmsMessageType kAvailabilityMessageType[] = {VmsMessageType::AVAILABILITY_CHANGE,
                                                   VmsMessageType::AVAILABILITY_RESPONSE};
const VmsMessageType kSubscriptionMessageType[] = {VmsMessageType::SUBSCRIPTIONS_CHANGE,
                                                   VmsMessageType::SUBSCRIPTIONS_RESPONSE};

class MockedVehicleHal : public VehicleHal {
  public:
    MockedVehicleHal() {
        mConfigs.assign(std::begin(kVehicleProperties), std::end(kVehicleProperties));
    }

    std::vector<VehiclePropConfig> listProperties() override { return mConfigs; }

    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override {
        *outStatus = StatusCode::OK;
        VehiclePropValuePtr pValue;
        auto property = static_cast<VehicleProperty>(requestedPropValue.prop);
        int32_t areaId = requestedPropValue.areaId;

        switch (property) {
            case VehicleProperty::INFO_MAKE:
                pValue = getValuePool()->obtainString(kCarMake);
                break;
            case VehicleProperty::INFO_FUEL_CAPACITY:
                if (fuelCapacityAttemptsLeft-- > 0) {
                    // Emulate property not ready yet.
                    *outStatus = StatusCode::TRY_AGAIN;
                } else {
                    pValue = getValuePool()->obtainFloat(42.42);
                }
                break;
            default:
                if (requestedPropValue.prop == kCustomComplexProperty) {
                    pValue = getValuePool()->obtainComplex();
                    pValue->value.int32Values = hidl_vec<int32_t>{10, 20};
                    pValue->value.int64Values = hidl_vec<int64_t>{30, 40};
                    pValue->value.floatValues = hidl_vec<float_t>{1.1, 2.2};
                    pValue->value.bytes = hidl_vec<uint8_t>{1, 2, 3};
                    pValue->value.stringValue = kCarMake;
                    break;
                }
                auto key = makeKey(toInt(property), areaId);
                pValue = getValuePool()->obtain(mValues[key]);
        }

        if (*outStatus == StatusCode::OK && pValue.get() != nullptr) {
            pValue->prop = toInt(property);
            pValue->areaId = areaId;
            pValue->timestamp = elapsedRealtimeNano();
        }

        return pValue;
    }

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
    void init();
    void deInit();
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
    void invokeGet(int32_t property, int32_t areaId) {
        VehiclePropValue requestedValue{};
        requestedValue.prop = property;
        requestedValue.areaId = areaId;
        mActualValue = VehiclePropValue{};  // reset previous values

        StatusCode refStatus;
        VehiclePropValue refValue;
        bool called = false;
        mManager->get(requestedValue, [&refStatus, &refValue, &called](
                                              StatusCode status, const VehiclePropValue& value) {
            refStatus = status;
            refValue = value;
            called = true;
        });

        mActualValue = refValue;
        mActualStatusCode = refStatus;
    }
};

void VehicleHalManagerFuzzer::init() {
    mHal.reset(new MockedVehicleHal);
    mManager.reset(new VehicleHalManager(mHal.get()));
    mObjectPool = mHal->getValuePool();
}

void VehicleHalManagerFuzzer::deInit() {
    mManager.reset(nullptr);
    mHal.reset(nullptr);
    mObjectPool = nullptr;
    delete mFuzzedDataProvider;
}

void VehicleHalManagerFuzzer::process(const uint8_t* data, size_t size) {
    mFuzzedDataProvider = new FuzzedDataProvider(data, size);
    invokeDebug();
    invokePropConfigs();
    invokeSubscribe();
    invokeSetAndGetValues();
    invokeObd2SensorStore();
    invokeVmsUtils();
    invokeVehiclePropStore();
    invokeWatchDogClient();
}

void VehicleHalManagerFuzzer::invokeDebug() {
    android::hardware::hidl_string debugOption = mFuzzedDataProvider->PickValueInArray(
            {"--help", "--list", "--get", "--set", "", "invalid"});
    android::hardware::hidl_handle fd = {};
    native_handle_t* const nativeHandle1 = native_handle_create(1 /*numFds*/, 0 /*numInts*/);
    fd.setTo(nativeHandle1, false /* shouldOwn */);

    const hidl_vec<android::hardware::hidl_string> optionsVector1{};
    const hidl_vec<android::hardware::hidl_string> optionsVector2{debugOption};
    mManager->debug(fd, optionsVector1);
    mManager->debug(fd, optionsVector2);
}

void VehicleHalManagerFuzzer::invokePropConfigs() {
    int32_t vehicleProp1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    hidl_vec<int32_t> properties = {vehicleProp1, vehicleProp2};
    bool called = false;

    mManager->getPropConfigs(properties,
                             [&called](StatusCode status, const hidl_vec<VehiclePropConfig>& c) {
                                 (void)status;
                                 (void)c;
                                 called = true;
                             });

    mManager->getPropConfigs({toInt(kVehicleProp[abs(vehicleProp1) % kVehiclePropArrayLength])},
                             [&called](StatusCode status, const hidl_vec<VehiclePropConfig>& c) {
                                 (void)status;
                                 (void)c;
                                 called = true;
                             });

    mManager->getAllPropConfigs([&called](const hidl_vec<VehiclePropConfig>& propConfigs) {
        (void)propConfigs;
        called = true;
    });
}

void VehicleHalManagerFuzzer::invokeSubscribe() {
    int32_t vehicleProp1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp3 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    const auto prop1 = toInt(kVehicleProp[abs(vehicleProp1) % kVehiclePropArrayLength]);
    sp<MockedVehicleCallback> cb = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> options = {
            SubscribeOptions{.propId = prop1, .flags = SubscribeFlags::EVENTS_FROM_CAR}};

    mManager->subscribe(cb, options);

    auto unsubscribedValue = mObjectPool->obtain(VehiclePropertyType::INT32);
    unsubscribedValue->prop = toInt(kVehicleProp[abs(vehicleProp2) % kVehiclePropArrayLength]);

    mHal->sendPropEvent(std::move(unsubscribedValue));
    cb->getReceivedEvents();
    cb->waitForExpectedEvents(0);

    auto subscribedValue = mObjectPool->obtain(VehiclePropertyType::INT32);
    subscribedValue->prop = toInt(kVehicleProp[abs(vehicleProp2) % kVehiclePropArrayLength]);
    subscribedValue->value.int32Values[0] = INT32_MAX;

    cb->reset();
    VehiclePropValue actualValue(*subscribedValue.get());
    mHal->sendPropEvent(std::move(subscribedValue));
    cb->waitForExpectedEvents(1);
    mManager->unsubscribe(cb, prop1);

    sp<MockedVehicleCallback> cb2 = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> options2 = {
            SubscribeOptions{
                    .propId = toInt(kVehicleProp[abs(vehicleProp3) % kVehiclePropArrayLength]),
                    .flags = SubscribeFlags::EVENTS_FROM_CAR},
    };

    mManager->subscribe(cb2, options2);

    mHal->sendHalError(StatusCode::TRY_AGAIN,
                       toInt(kVehicleProp[abs(vehicleProp3) % kVehiclePropArrayLength]),
                       0 /* area id*/);
}

void VehicleHalManagerFuzzer::invokeSetAndGetValues() {
    uint32_t vehicleProp1 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);
    uint32_t vehicleProp2 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);
    uint32_t vehicleProp3 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);

    invokeGet(kCustomComplexProperty, 0);
    invokeGet(toInt(kVehicleProp[vehicleProp2]), 0);
    invokeGet(toInt(kVehicleProp[vehicleProp1]), 0);

    auto expectedValue =
            mHal->getValuePool()->obtainInt32(mFuzzedDataProvider->ConsumeIntegral<int32_t>());
    mHal->getValuePool()->obtainInt64(mFuzzedDataProvider->ConsumeIntegral<int64_t>());
    mHal->getValuePool()->obtainFloat(mFuzzedDataProvider->ConsumeFloatingPoint<float>());
    mHal->getValuePool()->obtainBoolean(mFuzzedDataProvider->ConsumeBool());
    expectedValue->prop = toInt(kVehicleProp[vehicleProp2]);
    expectedValue->areaId = 0;

    mManager->set(*expectedValue.get());
    invokeGet(toInt(kVehicleProp[vehicleProp2]), 0);
    expectedValue->prop = toInt(kVehicleProp[vehicleProp3]);
    mManager->set(*expectedValue.get());
    expectedValue->prop = toInt(VehicleProperty::INVALID);
    mManager->set(*expectedValue.get());
}

void VehicleHalManagerFuzzer::invokeObd2SensorStore() {
    uint32_t diagnosticIntIndex =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kIntSensorArrayLength - 1);
    int32_t diagnosticIntValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    uint32_t diagnosticFloatIndex =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kFloatSensorArrayLength - 1);
    float diagnosticFloatValue = mFuzzedDataProvider->ConsumeFloatingPoint<float>();

    std::unique_ptr<Obd2SensorStore> sensorStore(
            new Obd2SensorStore(kIntSensorArrayLength, kFloatSensorArrayLength));
    if (sensorStore) {
        sensorStore->setIntegerSensor(kDiagnosticIntIndex[diagnosticIntIndex], diagnosticIntValue);
        sensorStore->setFloatSensor(kDiagnosticFloatIndex[diagnosticFloatIndex],
                                    diagnosticFloatValue);
        sensorStore->getIntegerSensors();
        sensorStore->getFloatSensors();
        sensorStore->getSensorsBitmask();
        static std::vector<std::string> sampleDtcs = {"P0070",
                                                      "P0102"
                                                      "P0123"};
        for (auto&& dtc : sampleDtcs) {
            auto freezeFrame = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
            sensorStore->fillPropValue(dtc, freezeFrame.get());
            freezeFrame->prop = (int)VehicleProperty::OBD2_FREEZE_FRAME;
        }
    }
}

void VehicleHalManagerFuzzer::invokeVmsUtils() {
    bool availabilityMsgType = mFuzzedDataProvider->ConsumeBool();
    bool subscriptionMsgType = mFuzzedDataProvider->ConsumeBool();
    int32_t intValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    vms::VmsLayer layer(1, 0, 2);
    auto message = createSubscribeMessage(layer);
    vms::isValidVmsMessage(*message);
    message = createUnsubscribeMessage(layer);

    vms::VmsOffers offers = {intValue, {vms::VmsLayerOffering(vms::VmsLayer(1, 0, 2))}};
    message = createOfferingMessage(offers);
    std::vector<vms::VmsLayer> dependencies = {vms::VmsLayer(2, 0, 2), vms::VmsLayer(3, 0, 3)};
    std::vector<vms::VmsLayerOffering> offering = {vms::VmsLayerOffering(layer, dependencies)};
    offers = {intValue, offering};
    message = createOfferingMessage(offers);

    message = vms::createAvailabilityRequest();
    message = vms::createSubscriptionsRequest();

    std::string bytes = "dummy";
    const vms::VmsLayerAndPublisher layer_and_publisher(vms::VmsLayer(2, 0, 1), intValue);
    message = createDataMessageWithLayerPublisherInfo(layer_and_publisher, bytes);
    vms::parseData(*message);
    createSubscribeToPublisherMessage(layer_and_publisher);
    createUnsubscribeToPublisherMessage(layer_and_publisher);

    std::string pub_bytes = "pub_id";
    message = vms::createPublisherIdRequest(pub_bytes);
    message = vms::createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::PUBLISHER_ID_RESPONSE), intValue};
    vms::parsePublisherIdResponse(*message);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kSubscriptionMessageType[subscriptionMsgType]), intValue};
    vms::getSequenceNumberForSubscriptionsState(*message);

    message->value.int32Values = hidl_vec<int32_t>{toInt(kSubscriptionMessageType[0]), intValue};
    vms::isSequenceNumberNewer(*message, intValue + 1);
    invokeGetSubscribedLayers(kSubscriptionMessageType[subscriptionMsgType]);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), 0};
    vms::hasServiceNewlyStarted(*message);
    message = vms::createStartSessionMessage(intValue, intValue + 1);
    vms::parseMessageType(*message);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), intValue};
    vms::isAvailabilitySequenceNumberNewer(*message, intValue + 1);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), intValue};
    vms::getSequenceNumberForAvailabilityState(*message);
    message = vms::createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 0, -1};
    vms::parseStartSessionMessage(*message, -1, 0, &new_service_id);
}

void VehicleHalManagerFuzzer::invokeGetSubscribedLayers(VmsMessageType type) {
    vms::VmsOffers offers = {
            123,
            {vms::VmsLayerOffering(vms::VmsLayer(1, 0, 1), {vms::VmsLayer(4, 1, 1)}),
             vms::VmsLayerOffering(vms::VmsLayer(2, 0, 1))}};
    auto message = vms::createBaseVmsMessage(16);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,  // sequence number
                                                   2,     // number of layers
                                                   1,     // number of associated layers
                                                   1,     // layer 1
                                                   0,           1,
                                                   4,  // layer 2
                                                   1,           1,
                                                   2,  // associated layer
                                                   0,           1,
                                                   2,    // number of publisher IDs
                                                   111,  // publisher IDs
                                                   123};
    vms::isValidVmsMessage(*message);
    vms::getSubscribedLayers(*message, offers);
    vms::getAvailableLayers(*message);
}

void VehicleHalManagerFuzzer::invokeVehiclePropStore() {
    bool shouldWriteStatus = mFuzzedDataProvider->ConsumeBool();
    int32_t vehicleProp = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    auto store = std::make_unique<VehiclePropertyStore>();
    VehiclePropConfig config{
            .prop = vehicleProp,
            .access = VehiclePropertyAccess::READ,
            .changeMode = VehiclePropertyChangeMode::STATIC,
            .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
    };
    store->registerProperty(config);
    VehiclePropValue propValue{};
    propValue.prop = vehicleProp;
    propValue.areaId = 0;
    store->writeValue(propValue, shouldWriteStatus);
    store->readAllValues();
    store->getAllConfigs();
    store->getConfigOrNull(vehicleProp);
    store->readValuesForProperty(vehicleProp);
    store->readValueOrNull(propValue);
    store->readValueOrNull(propValue.prop, propValue.areaId, 0);
    store->removeValuesForProperty(vehicleProp);
    store->removeValue(propValue);
    store->getConfigOrDie(vehicleProp);
}

void VehicleHalManagerFuzzer::invokeWatchDogClient() {
    auto service = new VehicleHalManager(mHal.get());
    sp<Looper> looper(Looper::prepare(mFuzzedDataProvider->ConsumeBool() /* opts */));
    std::shared_ptr<WatchdogClient> watchdogClient =
            ndk::SharedRefBase::make<WatchdogClient>(looper, service);
    if (watchdogClient->initialize()) {
        watchdogClient->checkIfAlive(-1, TimeoutLength::TIMEOUT_NORMAL);
        watchdogClient->prepareProcessTermination();
    }
    delete service;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    VehicleHalManagerFuzzer vmFuzzer;
    vmFuzzer.init();
    vmFuzzer.process(data, size);
    vmFuzzer.deInit();
    return 0;
}
