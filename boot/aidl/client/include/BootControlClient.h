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

#ifndef __BOOT_CONTROL_CLIENT_H_
#define __BOOT_CONTROL_CLIENT_H_

#include <aidl/android/hardware/boot/IBootControl.h>
#include <aidl/android/hardware/boot/MergeStatus.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/hardware/boot/1.0/IBootControl.h>
#include <android/hardware/boot/1.1/IBootControl.h>
#include <android/hardware/boot/1.2/IBootControl.h>

#include <stdint.h>

#include <memory>
#include <optional>

namespace android::hal {

struct CommandResult {
    bool success;
    std::string errMsg;
    constexpr bool IsOk() const { return success; }
};

enum class BootControlVersion { BOOTCTL_V1_0, BOOTCTL_V1_1, BOOTCTL_V1_2, BOOTCTL_AIDL };

class BootControlClient {
  public:
    using MergeStatus = aidl::android::hardware::boot::MergeStatus;
    virtual ~BootControlClient() = default;
    virtual BootControlVersion GetVersion() const = 0;
    // Return the number of update slots in the system. A system will normally
    // have two slots, named "A" and "B" in the documentation, but sometimes
    // images running from other media can have only one slot, like some USB
    // image. Systems with only one slot won't be able to update.
    [[nodiscard]] virtual int32_t GetNumSlots() const = 0;

    // Return the slot where we are running the system from. On success, the
    // result is a number between 0 and GetNumSlots() - 1. Otherwise, log an error
    // and return kInvalidSlot.
    [[nodiscard]] virtual int32_t GetCurrentSlot() const = 0;

    // Return string suffix for input slot. Usually, for slot 0 the suffix is _a, and for slot 1 the
    // suffix is _b.
    [[nodiscard]] virtual std::string GetSuffix(int32_t slot) const = 0;

    // Returns whether the passed |slot| is marked as bootable. Returns false if
    // the slot is invalid.
    [[nodiscard]] virtual std::optional<bool> IsSlotBootable(int32_t slot) const = 0;

    // Mark the specified slot unbootable. No other slot flags are modified.
    // Returns true on success.
    [[nodiscard]] virtual CommandResult MarkSlotUnbootable(int32_t slot) = 0;

    // Set the passed |slot| as the preferred boot slot. Returns whether it
    // succeeded setting the active slot. If succeeded, on next boot the
    // bootloader will attempt to load the |slot| marked as active. Note that this
    // method doesn't change the value of GetCurrentSlot() on the current boot.
    // Return true if operation succeeded.
    [[nodiscard]] virtual CommandResult SetActiveBootSlot(int32_t slot) = 0;

    // Check if |slot| is marked boot successfully. Return empty optional if the RPC call failed.
    [[nodiscard]] virtual std::optional<bool> IsSlotMarkedSuccessful(int32_t slot) const = 0;

    // Mark boot as successful. Return an error message if operation failed.
    [[nodiscard]] virtual CommandResult MarkBootSuccessful() = 0;

    // Added in IBootControl v1.1
    // Return the current merge status.
    [[nodiscard]] virtual MergeStatus getSnapshotMergeStatus() const = 0;

    // Set snapshot merge status, return true if succeeded.
    [[nodiscard]] virtual CommandResult SetSnapshotMergeStatus(MergeStatus status) = 0;

    // Added in IBootControl v1.2
    // Get the active slot. In other words, the slot which will be used on
    // next system reboot. This should match the |slot| parameter of last
    // successful call to |SetActiveBootSlot|.
    // Return 0xFFFFFFFF if underlying HAL doesn't support this operation.
    [[nodiscard]] virtual int32_t GetActiveBootSlot() const = 0;

    [[nodiscard]] static std::unique_ptr<BootControlClient> WaitForService();
};

class BootControlClientAidl final : public BootControlClient {
    using IBootControl = ::aidl::android::hardware::boot::IBootControl;

  public:
    explicit BootControlClientAidl(std::shared_ptr<IBootControl> module);
    ~BootControlClientAidl();

    BootControlVersion GetVersion() const override;
    void onBootControlServiceDied();
    virtual int32_t GetNumSlots() const;
    int32_t GetCurrentSlot() const;
    MergeStatus getSnapshotMergeStatus() const;
    std::string GetSuffix(int32_t slot) const;
    std::optional<bool> IsSlotBootable(int32_t slot) const;
    CommandResult MarkSlotUnbootable(int32_t slot);

    CommandResult SetActiveBootSlot(int slot);
    int GetActiveBootSlot() const;

    // Check if |slot| is marked boot successfully.
    std::optional<bool> IsSlotMarkedSuccessful(int slot) const;

    CommandResult MarkBootSuccessful();

    CommandResult SetSnapshotMergeStatus(aidl::android::hardware::boot::MergeStatus merge_status);

  private:
    const std::shared_ptr<IBootControl> module_;
    AIBinder_DeathRecipient* boot_control_death_recipient;
    static void onBootControlServiceDied(void* client) {
        BootControlClientAidl* self = static_cast<BootControlClientAidl*>(client);
        self->onBootControlServiceDied();
    }
};

using namespace android::hardware::boot;

class BootControlClientHIDL final : public BootControlClient {
  public:
    BootControlClientHIDL(android::sp<V1_0::IBootControl> module_v1,
                          android::sp<V1_1::IBootControl> module_v1_1,
                          android::sp<V1_2::IBootControl> module_v1_2);

    BootControlVersion GetVersion() const override;
    int32_t GetNumSlots() const;

    int32_t GetCurrentSlot() const;
    std::string GetSuffix(int32_t slot) const;

    std::optional<bool> IsSlotBootable(int32_t slot) const;

    CommandResult MarkSlotUnbootable(int32_t slot);

    CommandResult SetActiveBootSlot(int32_t slot);

    CommandResult MarkBootSuccessful();

    std::optional<bool> IsSlotMarkedSuccessful(int32_t slot) const;

    MergeStatus getSnapshotMergeStatus() const;

    CommandResult SetSnapshotMergeStatus(MergeStatus merge_status);

    int32_t GetActiveBootSlot() const;

  private:
    android::sp<V1_0::IBootControl> module_v1_;
    android::sp<V1_1::IBootControl> module_v1_1_;
    android::sp<V1_2::IBootControl> module_v1_2_;
};

}  // namespace android::hal

#endif
