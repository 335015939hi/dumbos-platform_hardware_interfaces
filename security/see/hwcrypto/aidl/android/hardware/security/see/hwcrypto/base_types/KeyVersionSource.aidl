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

/*
 * enum KeyVersionSource - Rollback version source.
 *
 * @COMMITTED_VERSION:
 *     Gate the derived key based on the anti-rollback counter that has been
 *     committed to fuses or stored. A component with a version smaller
 *     than this value should never run on the device again. The latest key may
 *     not be available the first few times a new version of the component runs on the
 *     device, because the counter may not be committed immediately. This
 *     version source may not allow versions > 0 on some devices (i.e. rollback
 *     versions cannot be committed).
 * @RUNNING_VERSION:
 *     Gate the derived key based on the anti-rollback version in the signed
 *     image of the component that is currently running. The latest key should be
 *     available immediately, but the component may be rolled back on a
 *     future boot. Care should be taken that everything still works if the image is
 *     rolled back and access to this key is lost. Care should also be taken
 *     that it is not possible to infer this key if it rolls back to a previous version.
 *     For example, storing the latest version of this key in storage
 *     would allow it to be retrieved after rollback.
 */
enum KeyVersionSource {
    COMMITTED_VERSION = 0,
    RUNNING_VERSION = 1,
}
