/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Copyright  (C) 2020  MediaTek Inc. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "HdmiEarc.h"

namespace aidl {

namespace android {

namespace hardware {

namespace tv {

namespace earc {

namespace impl {

#define UNUSED(expr) do { (void)(expr); } while(0)

::ndk::ScopedAStatus HdmiEarc::isSupport(bool* _aidl_return) {
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::getPortId(int32_t* _aidl_return) {
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::getStatus(int32_t in_portId, EarcStatus* _aidl_return) {
    UNUSED(in_portId);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::getCapability(int32_t in_portId,EarcCapability* _aidl_return) {
    UNUSED(in_portId);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::getLatency(int32_t in_portId, int32_t* _aidl_return) {
    UNUSED(in_portId);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::controlAudioLatency(int32_t in_latency, Result* _aidl_return) {
    UNUSED(in_latency);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::controlFeature(EarcControl in_control, Result* _aidl_return) {
    UNUSED(in_control);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::enableArc(bool in_enable, Result* _aidl_return) {
    UNUSED(in_enable);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HdmiEarc::setCallback(const std::shared_ptr<IHdmiEarcCallback>& in_callback, Result* _aidl_return) {
    UNUSED(in_callback);
    _aidl_return = nullptr;
    return ndk::ScopedAStatus::ok();
}

}  // namespace impl

}  // namespace earc

}  // namespace tv

}  // namespace hardware

}  // namespace android

}  // namespace aidl
