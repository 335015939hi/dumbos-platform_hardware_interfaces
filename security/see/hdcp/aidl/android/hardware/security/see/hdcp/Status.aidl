/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.security.see.hdcp;

@VintfStability
@Backing(type="int")
enum Status {
    /**
     * The DRM plugin must return OK when an operation completes without any
     * errors.
     */
    OK,
    /**
     * The HdcpAuthControl service must return ERROR_HDCP_LEVEL_NO_PENDING when
     * there is no pending request for a new level.
     */
    ERROR_HDCP_LEVEL_NO_PENDING,
    /**
     * The HdcpAuthControl service must return ERROR_HDCP_LEVEL_INVALID when
     * a requested level is invalid (higher than the max level).
     */
    ERROR_HDCP_LEVEL_INVALID,
    /**
     * The HdcpAuthControl service must return ERROR_HDCP_NOT_SUPPORTED when
     * a level cannot be set requested.
     */
    ERROR_HDCP_NOT_SUPPORTED,
}
