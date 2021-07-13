/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.cec@1.0-impl"
#include <android-base/logging.h>

#include <cutils/properties.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/cec.h>
#include <linux/ioctl.h>
#include <poll.h>
#include <pthread.h>
#include <regex>
#include <vector>

#include "HdmiCecDefault.h"

namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V1_0 {
namespace implementation {

using std::find;
using std::regex;
using std::smatch;
using std::stoi;
using std::string;
using std::vector;

static const int CEC_KEYCODE_POWER = 0x40;
static const int CEC_KEYCODE_ROOT_MENU = 0x09;
static const int CEC_KEYCODE_POWER_ON_FUNCTION = 0x6D;

// When set to false, all the CEC commands are discarded. True by default after initialization.
bool mCecEnabled;
/*
 * When set to false, HAL does not wake up the system upon receiving <Image View On> or
 * <Text View On>. True by default after initialization.
 */
bool mWakeupEnabled;
/*
 * Updated when system goes in and out of standby mode.
 * When set to true, Android system is handling CEC commands.
 * When set to false, microprocessor is handling CEC commands.
 * True by default after initialization.
 */
bool mCecControlEnabled;

vector<HdmiCecPort*> mHdmiCecPorts;
pthread_t mEventThread;
sp<IHdmiCecCallback> mCallback;
vector<int> mDeviceTypes;

HdmiCecDefault::HdmiCecDefault() {
    mCecEnabled = false;
    mWakeupEnabled = false;
    mCecControlEnabled = false;
    mCallback = nullptr;
}

HdmiCecDefault::~HdmiCecDefault() {
    release();
}

// Methods from ::android::hardware::tv::cec::V1_0::IHdmiCec follow.
Return<Result> HdmiCecDefault::addLogicalAddress(CecLogicalAddress addr) {
    if (addr < CecLogicalAddress::TV || addr >= CecLogicalAddress::BROADCAST) {
        LOG(ERROR) << "Add logical address failed, Invalid address";
        return Result::FAILURE_INVALID_ARGS;
    }

    struct cec_log_addrs cecLogAddrs;
    int ret = ioctl(mHdmiCecPorts[OUTPUT_PORT_INDEX]->mCecFd, CEC_ADAP_G_LOG_ADDRS, &cecLogAddrs);
    if (ret) {
        LOG(ERROR) << "Add logical address failed, Error = " << strerror(errno);
        return Result::FAILURE_BUSY;
    }

    cecLogAddrs.cec_version = getCecVersion();
    cecLogAddrs.vendor_id = getVendorId();

    unsigned int logAddrType = CEC_LOG_ADDR_TYPE_UNREGISTERED;
    unsigned int allDevTypes = 0;
    unsigned int primDevType = 0xff;
    switch (addr) {
        case CecLogicalAddress::TV:
            primDevType = CEC_OP_PRIM_DEVTYPE_TV;
            logAddrType = CEC_LOG_ADDR_TYPE_TV;
            allDevTypes = CEC_OP_ALL_DEVTYPE_TV;
            break;
        case CecLogicalAddress::RECORDER_1:
        case CecLogicalAddress::RECORDER_2:
        case CecLogicalAddress::RECORDER_3:
            primDevType = CEC_OP_PRIM_DEVTYPE_RECORD;
            logAddrType = CEC_LOG_ADDR_TYPE_RECORD;
            allDevTypes = CEC_OP_ALL_DEVTYPE_RECORD;
            break;
        case CecLogicalAddress::TUNER_1:
        case CecLogicalAddress::TUNER_2:
        case CecLogicalAddress::TUNER_3:
        case CecLogicalAddress::TUNER_4:
            primDevType = CEC_OP_PRIM_DEVTYPE_TUNER;
            logAddrType = CEC_LOG_ADDR_TYPE_TUNER;
            allDevTypes = CEC_OP_ALL_DEVTYPE_TUNER;
            break;
        case CecLogicalAddress::PLAYBACK_1:
        case CecLogicalAddress::PLAYBACK_2:
        case CecLogicalAddress::PLAYBACK_3:
            primDevType = CEC_OP_PRIM_DEVTYPE_PLAYBACK;
            logAddrType = CEC_LOG_ADDR_TYPE_PLAYBACK;
            allDevTypes = CEC_OP_ALL_DEVTYPE_PLAYBACK;
            cecLogAddrs.flags |= CEC_LOG_ADDRS_FL_ALLOW_RC_PASSTHRU;
            break;
        case CecLogicalAddress::AUDIO_SYSTEM:
            primDevType = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM;
            logAddrType = CEC_LOG_ADDR_TYPE_AUDIOSYSTEM;
            allDevTypes = CEC_OP_ALL_DEVTYPE_AUDIOSYSTEM;
            break;
        case CecLogicalAddress::FREE_USE:
            primDevType = CEC_OP_PRIM_DEVTYPE_PROCESSOR;
            logAddrType = CEC_LOG_ADDR_TYPE_SPECIFIC;
            allDevTypes = CEC_OP_ALL_DEVTYPE_SWITCH;
            break;
        case CecLogicalAddress::UNREGISTERED:
            cecLogAddrs.flags |= CEC_LOG_ADDRS_FL_ALLOW_UNREG_FALLBACK;
            break;
    }

    int logAddrIndex = cecLogAddrs.num_log_addrs;

    cecLogAddrs.num_log_addrs += 1;
    cecLogAddrs.log_addr[logAddrIndex] = static_cast<cec_logical_address_t>(addr);
    cecLogAddrs.log_addr_type[logAddrIndex] = logAddrType;
    cecLogAddrs.primary_device_type[logAddrIndex] = primDevType;
    cecLogAddrs.all_device_types[logAddrIndex] = allDevTypes;
    cecLogAddrs.features[logAddrIndex][0] = 0;
    cecLogAddrs.features[logAddrIndex][1] = 0;

    Return<Result> result = Result::SUCCESS;
    for (int i = 0; i < mHdmiCecPorts.size(); i++) {
        ret = ioctl(mHdmiCecPorts[i]->mCecFd, CEC_ADAP_S_LOG_ADDRS, &cecLogAddrs);
        if (ret) {
            LOG(ERROR) << "Add logical address failed for port " << mHdmiCecPorts[i]->mPortId
                       << ", Error = " << strerror(errno);
            result = Result::FAILURE_BUSY;
        }
    }
    return result;
}

Return<void> HdmiCecDefault::clearLogicalAddress() {
    struct cec_log_addrs cecLogAddrs;
    memset(&cecLogAddrs, 0, sizeof(cecLogAddrs));
    for (int i = 0; i < mHdmiCecPorts.size(); i++) {
        int ret = ioctl(mHdmiCecPorts[i]->mCecFd, CEC_ADAP_S_LOG_ADDRS, &cecLogAddrs);
        if (ret) {
            LOG(ERROR) << "Clear logical Address failed for port " << mHdmiCecPorts[i]->mPortId
                       << ", Error = " << strerror(errno);
        }
    }
    return Void();
}

Return<void> HdmiCecDefault::getPhysicalAddress(getPhysicalAddress_cb callback) {
    uint16_t addr;
    int ret = ioctl(mHdmiCecPorts[OUTPUT_PORT_INDEX]->mCecFd, CEC_ADAP_G_PHYS_ADDR, &addr);
    if (ret) {
        LOG(ERROR) << "Get physical address failed, Error = " << strerror(errno);
        callback(Result::FAILURE_INVALID_STATE, addr);
        return Void();
    }
    callback(Result::SUCCESS, addr);
    return Void();
}

Return<SendMessageResult> HdmiCecDefault::sendMessage(const CecMessage& message) {
    if (!mCecEnabled) {
        return SendMessageResult::FAIL;
    }

    struct cec_msg cecMsg;
    memset(&cecMsg, 0, sizeof(cec_msg));

    int initiator = static_cast<cec_logical_address_t>(message.initiator);
    int destination = static_cast<cec_logical_address_t>(message.destination);

    cecMsg.msg[0] = (initiator << 4) | destination;
    for (size_t i = 0; i < message.body.size(); ++i) {
        cecMsg.msg[i + 1] = message.body[i];
    }
    cecMsg.len = message.body.size() + 1;

    int ret = ioctl(mHdmiCecPorts[OUTPUT_PORT_INDEX]->mCecFd, CEC_TRANSMIT, &cecMsg);

    if (ret) {
        LOG(ERROR) << "Send message failed, Error = " << strerror(errno);
        return SendMessageResult::FAIL;
    }

    if (cecMsg.tx_status != CEC_TX_STATUS_OK) {
        LOG(ERROR) << "Send message tx_status = " << cecMsg.tx_status;
    }

    switch (cecMsg.tx_status) {
        case CEC_TX_STATUS_OK:
            return SendMessageResult::SUCCESS;
        case CEC_TX_STATUS_ARB_LOST:
            return SendMessageResult::BUSY;
        case CEC_TX_STATUS_NACK:
            return SendMessageResult::NACK;
        default:
            return SendMessageResult::FAIL;
    }
}

Return<void> HdmiCecDefault::setCallback(const sp<IHdmiCecCallback>& callback) {
    if (mCallback != nullptr) {
        mCallback->unlinkToDeath(this);
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        mCallback->linkToDeath(this, 0 /*cookie*/);
    }
    return Void();
}

Return<int32_t> HdmiCecDefault::getCecVersion() {
    return property_get_int32("ro.hdmi.cec_version", CEC_OP_CEC_VERSION_1_4);
}

Return<uint32_t> HdmiCecDefault::getVendorId() {
    return property_get_int32("ro.hdmi.vendor_id", 0x000c03 /* HDMI LLC vendor ID */);
}

Return<void> HdmiCecDefault::getPortInfo(getPortInfo_cb callback) {
    hidl_vec<HdmiPortInfo> portInfos(mHdmiCecPorts.size());
    for (int i = 0; i < mHdmiCecPorts.size(); i++) {
        uint16_t addr = INVALID_PHYSICAL_ADDRESS;
        int ret = ioctl(mHdmiCecPorts[i]->mCecFd, CEC_ADAP_G_PHYS_ADDR, &addr);
        if (ret) {
            LOG(ERROR) << "Get port info failed for port : " << mHdmiCecPorts[i]->mPortId
                       << ", Error = " << strerror(errno);
        }
        HdmiPortType type = HdmiPortType::INPUT;
        if (!hasDeviceType(CEC_DEVICE_TV) && i == OUTPUT_PORT_INDEX) {
            type = HdmiPortType::OUTPUT;
        }
        portInfos[i] = {.type = type,
                        .portId = mHdmiCecPorts[i]->mPortId,
                        .cecSupported = true,
                        .arcSupported = false,
                        .physicalAddress = addr};
    }
    callback(portInfos);
    return Void();
}

Return<void> HdmiCecDefault::setOption(OptionKey key, bool value) {
    switch (key) {
        case OptionKey::ENABLE_CEC:
            LOG(DEBUG) << "setOption: Enable CEC: " << value;
            mCecEnabled = value;
            break;
        case OptionKey::WAKEUP:
            LOG(DEBUG) << "setOption: WAKEUP: " << value;
            mWakeupEnabled = value;
            break;
        case OptionKey::SYSTEM_CEC_CONTROL:
            LOG(DEBUG) << "setOption: SYSTEM_CEC_CONTROL: " << value;
            mCecControlEnabled = value;
            break;
    }
    return Void();
}

Return<void> HdmiCecDefault::setLanguage(const hidl_string& /*language*/) {
    return Void();
}

Return<void> HdmiCecDefault::enableAudioReturnChannel(int32_t /*portId*/, bool /*enable*/) {
    return Void();
}

Return<bool> HdmiCecDefault::isConnected(int32_t /*portId*/) {
    uint16_t addr;
    int ret = ioctl(mHdmiCecPorts[OUTPUT_PORT_INDEX]->mCecFd, CEC_ADAP_G_PHYS_ADDR, &addr);
    if (ret) {
        LOG(ERROR) << "Is connected failed, Error = " << strerror(errno);
        return false;
    }
    if (addr == CEC_PHYS_ADDR_INVALID) {
        return false;
    }
    return true;
}

// Initialise the cec file descriptor
Return<Result> HdmiCecDefault::init() {
    const char* parentPath = "/dev/";
    DIR* dir = opendir(parentPath);
    regex rgx("^cec([0-9]+)$");

    while (struct dirent* dirEntry = readdir(dir)) {
        string filename = dirEntry->d_name;
        string filepath = parentPath + filename;
        smatch match;
        if (regex_search(filename, match, rgx)) {
            int portId = stoi(match[1].str());
            HdmiCecPort* hdmiCecPort = new HdmiCecPort(portId);
            Result result = hdmiCecPort->init(filepath.c_str());
            if (result != Result::SUCCESS) {
                continue;
            }
            /* thread loop for receiving cec messages and hotplug events*/
            if (pthread_create(&mEventThread, NULL, event_thread, hdmiCecPort)) {
                LOG(ERROR) << "Can't create event thread: " << strerror(errno);
                hdmiCecPort->release();
                continue;
            }
            mHdmiCecPorts.push_back(hdmiCecPort);
        }
    }
    if (mHdmiCecPorts.empty()) {
        return Result::FAILURE_NOT_SUPPORTED;
    }

    mCecEnabled = true;
    mWakeupEnabled = true;
    mCecControlEnabled = true;
    initDeviceType();
    return Result::SUCCESS;
}

Return<void> HdmiCecDefault::release() {
    mCecEnabled = false;
    mWakeupEnabled = false;
    pthread_join(mEventThread, NULL);
    setCallback(nullptr);
    mDeviceTypes.clear();
    for (int i = 0; i < mHdmiCecPorts.size(); i++) {
        delete mHdmiCecPorts[i];
    }
    mHdmiCecPorts.clear();
    return Void();
}

void* HdmiCecDefault::event_thread(void* args) {
    HdmiCecPort* hdmiCecPort = (HdmiCecPort*)args;

    struct pollfd ufds[3] = {
            {hdmiCecPort->mCecFd, POLLIN, 0},
            {hdmiCecPort->mCecFd, POLLERR, 0},
            {hdmiCecPort->mExitFd, POLLIN, 0},
    };

    while (1) {
        ufds[0].revents = 0;
        ufds[1].revents = 0;
        ufds[2].revents = 0;

        int ret = poll(ufds, /* size(ufds) = */ 3, /* timeout = */ -1);

        if (ret <= 0) {
            continue;
        }

        if (ufds[2].revents == POLLIN) { /* Exit */
            break;
        }

        if (ufds[1].revents == POLLERR) { /* CEC Event */
            struct cec_event ev;
            ret = ioctl(hdmiCecPort->mCecFd, CEC_DQEVENT, &ev);

            if (ret) {
                LOG(ERROR) << "CEC_DQEVENT failed, Error = " << strerror(errno);
                continue;
            }

            if (!mCecEnabled) {
                continue;
            }

            if (ev.event == CEC_EVENT_STATE_CHANGE) {
                if (mCallback != nullptr) {
                    HotplugEvent hotplugEvent{
                            .connected = (ev.state_change.phys_addr != CEC_PHYS_ADDR_INVALID),
                            .portId = hdmiCecPort->mPortId};
                    mCallback->onHotplugEvent(hotplugEvent);
                } else {
                    LOG(ERROR) << "No event callback for hotplug";
                }
            }
        }

        if (ufds[0].revents == POLLIN) { /* CEC Driver */
            struct cec_msg msg = {};
            ret = ioctl(hdmiCecPort->mCecFd, CEC_RECEIVE, &msg);

            if (ret) {
                LOG(ERROR) << "CEC_RECEIVE failed, Error = " << strerror(errno);
                continue;
            }

            if (msg.rx_status != CEC_RX_STATUS_OK) {
                LOG(ERROR) << "msg rx_status = " << msg.rx_status;
                continue;
            }

            if (!mCecEnabled) {
                continue;
            }

            if (!mWakeupEnabled && isWakeupMessage(msg)) {
                LOG(DEBUG) << "Filter wakeup message";
                continue;
            }

            if (!mCecControlEnabled && !isTransferableInSleep(msg)) {
                LOG(DEBUG) << "Filter message in standby mode";
                continue;
            }

            if (mCallback != nullptr) {
                size_t length = std::min(msg.len - 1, (uint32_t)MaxLength::MESSAGE_BODY);
                CecMessage cecMessage{
                        .initiator = static_cast<CecLogicalAddress>(msg.msg[0] >> 4),
                        .destination = static_cast<CecLogicalAddress>(msg.msg[0] & 0xf),
                };
                cecMessage.body.resize(length);
                for (size_t i = 0; i < length; ++i) {
                    cecMessage.body[i] = static_cast<uint8_t>(msg.msg[i + 1]);
                }
                mCallback->onCecMessage(cecMessage);
            } else {
                LOG(ERROR) << "no event callback for message";
            }
        }
    }
    return NULL;
}

int HdmiCecDefault::getOpcode(struct cec_msg message) {
    return (static_cast<uint8_t>(message.msg[1]) & 0xff);
}

bool HdmiCecDefault::isWakeupMessage(struct cec_msg message) {
    int opcode = getOpcode(message);
    switch (opcode) {
        case CEC_MESSAGE_TEXT_VIEW_ON:
        case CEC_MESSAGE_IMAGE_VIEW_ON:
            return true;
        default:
            return false;
    }
}

void HdmiCecDefault::initDeviceType() {
    char value[PROPERTY_VALUE_MAX] = {0};
    const char* split = ",";

    property_get(PROPERTY_DEVICE_TYPE, value, "4");

    char* deviceTypes = strtok(value, split);
    while (deviceTypes != NULL) {
        int deviceType;
        if (Result::SUCCESS == strToInt(deviceTypes, &deviceType)) {
            mDeviceTypes.push_back(deviceType);
        }
        deviceTypes = strtok(NULL, split);
    }
}

bool HdmiCecDefault::hasDeviceType(int deviceType) {
    return find(mDeviceTypes.begin(), mDeviceTypes.end(), deviceType) != mDeviceTypes.end();
}

Return<Result> HdmiCecDefault::strToInt(char* string, int* num) {
    char* end;
    long int lnum = strtol(string, &end, 10);
    if (end == string) {
        LOG(ERROR) << "Can't convert string " << string << " to number";
        return Result::FAILURE_INVALID_ARGS;
    }
    *num = (int)lnum;
    return Result::SUCCESS;
}

bool HdmiCecDefault::isTransferableInSleep(struct cec_msg message) {
    int opcode = getOpcode(message);
    switch (opcode) {
        case CEC_MESSAGE_ABORT:
        case CEC_MESSAGE_DEVICE_VENDOR_ID:
        case CEC_MESSAGE_GET_CEC_VERSION:
        case CEC_MESSAGE_GET_MENU_LANGUAGE:
        case CEC_MESSAGE_GIVE_DEVICE_POWER_STATUS:
        case CEC_MESSAGE_GIVE_DEVICE_VENDOR_ID:
        case CEC_MESSAGE_GIVE_OSD_NAME:
        case CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS:
        case CEC_MESSAGE_REPORT_PHYSICAL_ADDRESS:
        case CEC_MESSAGE_REPORT_POWER_STATUS:
        case CEC_MESSAGE_SET_OSD_NAME:
            return true;
        case CEC_MESSAGE_USER_CONTROL_PRESSED:
            return isPowerUICommand(message);
        case CEC_MESSAGE_DECK_CONTROL:
        case CEC_MESSAGE_PLAY:
            return hasDeviceType(CEC_DEVICE_PLAYBACK);
        case CEC_MESSAGE_IMAGE_VIEW_ON:
        case CEC_MESSAGE_TEXT_VIEW_ON:
            return hasDeviceType(CEC_DEVICE_TV);
        case CEC_MESSAGE_SYSTEM_AUDIO_MODE_REQUEST:
            return hasDeviceType(CEC_DEVICE_AUDIO_SYSTEM);
        default:
            return false;
    }
}

int HdmiCecDefault::getFirstParam(struct cec_msg message) {
    return (static_cast<uint8_t>(message.msg[2]) & 0xff);
}

bool HdmiCecDefault::isPowerUICommand(struct cec_msg message) {
    int uiCommand = getFirstParam(message);
    switch (uiCommand) {
        case CEC_KEYCODE_POWER:
        case CEC_KEYCODE_ROOT_MENU:
        case CEC_KEYCODE_POWER_ON_FUNCTION:
            return true;
        default:
            return false;
    }
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android
