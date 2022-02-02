///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.automotive.audiocontrol;
@Backing(type="int") @VintfStability
enum Reasons {
  CYBER_MASTER_MUTE = 1,
  REMOTE_MUTE = 2,
  TCU_MUTE = 4,
  ADAS_DUCKING = 8,
  NAV_DUCKING = 16,
  CAR_PLAY_ALT_DUCKING = 32,
  THERMAL_LIMITATION = 64,
  SUSPEND_EXIT_VOL_LIMITATION = 128,
  EXTERNAL_AMP_VOL_FEEDBACK = 256,
  OTHER = -2147483648,
}
