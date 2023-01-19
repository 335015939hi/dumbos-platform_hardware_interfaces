# DRM HAL

This is the underlying HAL implementation for `MediaDrm`/`MediaCrypto` (and
their NDK counterparts).

## AIDL error handling

Starting in **Android U (14)**, `libmediadrm` in `frameworks/av` understands extra
error details from **AIDL** DRM HALs passed through the binder exception message
as a json string. The supported fields are:
* `cdmError` (*int*)
* `oemError` (*int*)
* `context` (*int*)
* `errorMessage` (*str*)

The errors details will be reported to apps through the java interface
`android.media.MediaDrmThrowable`. Please see the javadoc of `MediaDrmThrowable`
for detailed definitions of each field above.

## Plugin-vendor-specific VTS modules

TODO(b/266091099)

For now please see `./1.0/vts/doc/Drm_Vendor_Modules_v1.pdf`.
