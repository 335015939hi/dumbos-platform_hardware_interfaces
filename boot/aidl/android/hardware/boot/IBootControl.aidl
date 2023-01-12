// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.boot;

import android.hardware.boot.BoolResult;
import android.hardware.boot.CommandResult;
import android.hardware.boot.MergeStatus;

// Interface inherits from android.hardware.boot@1.1::IBootControl but AIDL does not support interface inheritance (methods have been flattened).
@VintfStability
interface IBootControl {
    // Adding return type to method instead of out param int slot since there is only one return value.
    /**
     * Returns the active slot to boot into on the next boot. If
     * setActiveBootSlot() has been called, the getter function should return the
     * same slot as the one provided in the last setActiveBootSlot() call.
     * The returned value is always guaranteed to be strictly less than the
     * value returned by getNumberSlots. Slots start at 0 and finish at
     * getNumberSlots() - 1. For instance, a system with A/B must return 0 or 1.
     */
    int getActiveBootSlot();

    // Adding return type to method instead of out param int slot since there is only one return value.
    /**
     * getCurrentSlot() returns the slot number of that the current boot is booted
     * from, for example slot number 0 (Slot A). It is assumed that if the current
     * slot is A, then the block devices underlying B can be accessed directly
     * without any risk of corruption.
     * The returned value is always guaranteed to be strictly less than the
     * value returned by getNumberSlots. Slots start at 0 and finish at
     * getNumberSlots() - 1. The value returned here must match the suffix passed
     * from the bootloader, regardless of which slot is active or successful.
     */
    int getCurrentSlot();

    // Adding return type to method instead of out param int numSlots since there is only one return value.
    /**
     * getNumberSlots() returns the number of available slots.
     * For instance, a system with a single set of partitions must return
     * 1, a system with A/B must return 2, A/B/C -> 3 and so on. A system with
     * less than two slots doesn't support background updates, for example if
     * running from a virtual machine with only one copy of each partition for the
     * purpose of testing.
     */
    int getNumberSlots();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param MergeStatus status since there is only one return value.
    /**
     * Returns whether a snapshot-merge of any dynamic partition is in progress.
     *
     * This function must return the merge status set by the last setSnapshotMergeStatus call and
     * recorded by the bootloader with one exception. If the partitions are being flashed from the
     * bootloader such that the pending merge must be canceled (for example, if the super partition
     * is being flashed), this function must return CANCELLED.
     *
     * @param out success True if the merge status is read successfully, false otherwise.
     * @return Merge status.
     */
    MergeStatus getSnapshotMergeStatus();

    // Adding return type to method instead of out param String slotSuffix since there is only one return value.
    /**
     * getSuffix() returns the string suffix used by partitions that correspond to
     * the slot number passed in as a parameter. The bootloader must pass the
     * suffix of the currently active slot either through a kernel command line
     * property at androidboot.slot_suffix, or the device tree at
     * /firmware/android/slot_suffix.
     * Returns the empty string "" if slot does not match an existing slot.
     */
    String getSuffix(in int slot);

    // Adding return type to method instead of out param BoolResult bootable since there is only one return value.
    /**
     * isSlotBootable() returns if the slot passed in parameter is bootable. Note
     * that slots can be made unbootable by both the bootloader and by the OS
     * using setSlotAsUnbootable.
     * Returns TRUE if the slot is bootable, FALSE if it's not, and INVALID_SLOT
     * if slot does not exist.
     */
    BoolResult isSlotBootable(in int slot);

    // Adding return type to method instead of out param BoolResult successful since there is only one return value.
    /**
     * isSlotMarkedSucessful() returns if the slot passed in parameter has been
     * marked as successful using markBootSuccessful. Note that only the current
     * slot can be marked as successful but any slot can be queried.
     * Returns TRUE if the slot has been marked as successful, FALSE if it has
     * not, and INVALID_SLOT if the slot does not exist.
     */
    BoolResult isSlotMarkedSuccessful(in int slot);

    // Adding return type to method instead of out param CommandResult error since there is only one return value.
    /**
     * markBootSuccessful() marks the current slot as having booted successfully.
     *
     * Returns whether the command succeeded.
     */
    CommandResult markBootSuccessful();

    // Adding return type to method instead of out param CommandResult error since there is only one return value.
    /**
     * setActiveBootSlot() marks the slot passed in parameter as the active boot
     * slot (see getCurrentSlot for an explanation of the "slot" parameter). This
     * overrides any previous call to setSlotAsUnbootable.
     * Returns whether the command succeeded.
     */
    CommandResult setActiveBootSlot(in int slot);

    // Adding return type to method instead of out param CommandResult error since there is only one return value.
    /**
     * setSlotAsUnbootable() marks the slot passed in parameter as
     * an unbootable. This can be used while updating the contents of the slot's
     * partitions, so that the system must not attempt to boot a known bad set up.
     * Returns whether the command succeeded.
     */
    CommandResult setSlotAsUnbootable(in int slot);

    // Adding return type to method instead of out param boolean success since there is only one return value.
    /**
     * Sets whether a snapshot-merge of any dynamic partition is in progress.
     *
     * After the merge status is set to a given value, subsequent calls to
     * getSnapshotMergeStatus must return the set value.
     *
     * The merge status must be persistent across reboots. That is, getSnapshotMergeStatus
     * must return the same value after a reboot if the merge status is not altered in any way
     * (e.g. set by setSnapshotMergeStatus or set to CANCELLED by bootloader).
     *
     * Read/write access to the merge status must be atomic. When the HAL is processing a
     * setSnapshotMergeStatus call, all subsequent calls to getSnapshotMergeStatus must block until
     * setSnapshotMergeStatus has returned.
     *
     * A MERGING state indicates that dynamic partitions are partially comprised by blocks in the
     * userdata partition.
     *
     * When the merge status is set to MERGING, the following operations must be prohibited from the
     * bootloader:
     *  - Flashing or erasing "userdata" or "metadata".
     *
     * The following operations may be prohibited when the status is set to MERGING. If not
     * prohibited, it is recommended that the user receive a warning.
     *  - Changing the active slot (e.g. via "fastboot set_active")
     *
     * @param status Merge status.
     *
     * @return True on success, false otherwise.
     */
    boolean setSnapshotMergeStatus(in MergeStatus status);
}
