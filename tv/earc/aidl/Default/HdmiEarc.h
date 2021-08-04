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

#pragma once

#include <aidl/android/hardware/tv/earc/BnHdmiEarc.h>

namespace aidl {

namespace android {
    
namespace hardware {

namespace tv {

namespace earc {

namespace impl {

class HdmiEarc : public BnHdmiEarc {
     /**
      * Get the hardware variation of eARC support flag.
      *
      * This used by framework to consider the eARC feature in need.
      *
      * @return The eARC support status related with hardware variation.
      * 		   TRUE if support eARC.
      */
      ::ndk::ScopedAStatus isSupport(bool* _aidl_return) override;

      /**
       * Get the current eARC port id.
       *
       * @return The current eARC port id.
       *         It shall start from "1" which indicates the "hdmi port 1".
       */
      ::ndk::ScopedAStatus getPortId(int32_t* _aidl_return) override;

      /**
       * Get the current eARC status.
       *
       * Since eARC not rely on CEC message, Framework need to get the current eARC
       * hw status to avoid handle the ARC handshake at first.
       *
       * @param in portId The eARC port id from framework.
       *
       * @return The current eARC hw status as defined in EarcStatus.
       */
      ::ndk::ScopedAStatus getStatus(int32_t in_portId, EarcStatus* _aidl_return) override;
      
      /**
       * Get the capability of eARC RX device.
       *
       * The audio capability data structure as defined in hdmi2.1 spec 9.5 and example in
       * Appendix H, Which indicate the audio formats and sample rates that eARC device support.
       * Earc tx shall only send Basic audio or audio that capability indicates it supports.
       *
       * @param in portId The eARC port id from framework.
       *
       * @return The audio capability from device.
       */
      ::ndk::ScopedAStatus getCapability(int32_t in_portId, EarcCapability* _aidl_return) override;
      
      /**
       * Get the eARC latency value.
       *
       * The latency value from device used by audio framework to control/adjust the audio latency feature.
       *
       * @param in portId The eARC port id from framework.
       *
       * @return The latency value from device.
       */
      ::ndk::ScopedAStatus getLatency(int32_t in_portId, int32_t* _aidl_return) override;
      
      /**
       * Control the eARC Audio latency.
       *
       * If support the eARC audio latency feature, Audio framework or others would adjust the latency value
       * according to the which from eARC device and send back.
       *
       * @param in latency The adjusted latency value.
       *
       * @return Result code of the operation. OK if successful, otherwise fail.
       */
      ::ndk::ScopedAStatus controlAudioLatency(int32_t in_latency, Result* _aidl_return) override;
      
      /**
       * Control the eARC feature.
       *
       * Users should control the eARC feature in user interface.
       *
       * @param in control The bool parameter for user to control the eARC feature.
       *
       * @return Result code of the operation. OK if successful, otherwise fail.
       */
      ::ndk::ScopedAStatus controlFeature(EarcControl in_control, Result* _aidl_return) override;
      
      /**
       * Control ARC Enable.
       *
       * @param in enable The bool parameter for CEC HAL to control ARC Enable.
       *
       * @return Result code of the operation. OK if successful, otherwise fail.
       */
      ::ndk::ScopedAStatus enableArc(bool in_enable, Result* _aidl_return) override;
      
      /**
       * Set the eARC callback event.
       *
       * It's used by the framework to receive status change event, capability change event,
       * and latency change event. Only one callback client is supported.
       *
       * @param in callback Callback function to pass eARC event to the system.
       *
       * @return Result code of the operation. OK if successful, otherwise fail.
       */
      ::ndk::ScopedAStatus setCallback(const std::shared_ptr<IHdmiEarcCallback>& in_callback, Result* _aidl_return) override;
};

}  // namespace impl

}  // namespace earc

}  // namespace tv

}  // namespace hardware

}  // namespace android

}  // namespace aidl

