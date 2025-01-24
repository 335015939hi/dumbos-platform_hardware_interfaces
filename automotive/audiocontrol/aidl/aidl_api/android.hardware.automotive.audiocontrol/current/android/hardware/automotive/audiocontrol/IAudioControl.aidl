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
@VintfStability
interface IAudioControl {
  oneway void onAudioFocusChange(in String usage, in int zoneId, in android.hardware.automotive.audiocontrol.AudioFocusChange focusChange);
  oneway void onDevicesToDuckChange(in android.hardware.automotive.audiocontrol.DuckingInfo[] duckingInfos);
  oneway void onDevicesToMuteChange(in android.hardware.automotive.audiocontrol.MutingInfo[] mutingInfos);
  oneway void registerFocusListener(in android.hardware.automotive.audiocontrol.IFocusListener listener);
  oneway void setBalanceTowardRight(in float value);
  oneway void setFadeTowardFront(in float value);
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
  oneway void onAudioFocusChangeWithMetaData(in android.hardware.audio.common.PlaybackTrackMetadata playbackMetaData, in int zoneId, in android.hardware.automotive.audiocontrol.AudioFocusChange focusChange);
  oneway void setAudioDeviceGainsChanged(in android.hardware.automotive.audiocontrol.Reasons[] reasons, in android.hardware.automotive.audiocontrol.AudioGainConfigInfo[] gains);
  oneway void registerGainCallback(in android.hardware.automotive.audiocontrol.IAudioGainCallback callback);
=======
  oneway void onAudioFocusChangeWithMetaData(in android.hardware.audio.common.PlaybackTrackMetadata playbackMetaData, in int zoneId, in android.hardware.automotive.audiocontrol.AudioFocusChange focusChange);
  oneway void setAudioDeviceGainsChanged(in android.hardware.automotive.audiocontrol.Reasons[] reasons, in android.hardware.automotive.audiocontrol.AudioGainConfigInfo[] gains);
  oneway void registerGainCallback(in android.hardware.automotive.audiocontrol.IAudioGainCallback callback);
  void setModuleChangeCallback(in android.hardware.automotive.audiocontrol.IModuleChangeCallback callback);
  void clearModuleChangeCallback();
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
}
