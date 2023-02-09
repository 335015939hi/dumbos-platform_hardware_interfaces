/*
 * Copyright 2023 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto.base_types;

/**
 * enum VersionedComponent - Enum describing the different components which version can be used for
 *                           version binding. Note that currently only SeeVersion is supported for
 *                           secure HAL operations. Other listed values represent components that
 *                           keymaster can bind versions to.
 *
 * @SeeVersion:
 *      Secure Excecution Environment version.
 * @BootPatchLevel:
 *      Boot partition version.
 * @VendorPatchLevel:
 *      Vendor partition version.
 * @OsVersion:
 *      System partition version.
 * @OsPatchLevel:
 *      Android patch version partition version, representing the year and month of the
 *      last update to the system.
 */
@Backing(type="byte")
enum VersionedComponent {
    SEE_VERSION = 0,
    BOOT_PATCH_LEVEL = 1,
    VENDOR_PATCH_LEVEL = 2,
    OS_VERSION = 3,
    OS_PATCH_LEVEL = 4,
}
