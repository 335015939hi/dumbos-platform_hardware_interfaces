/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * ALP Filter Type according to A/330 ATSC3.0.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum AudioPresentationContentType {
    UNKNOWN = 0,
    /**
     * A filter to filter signaling data out from input stream, and queue the
     * data to the filter's FMQ (Fast Message Queue).
     */
    MAIN,

    /**
     * A filter to set PTP (Precision Time Protocol) channel from input stream.
     */
    MUSIC_AND_EFFECTS,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    VISUALLY_IMPAIRED,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    HEARING_IMPAIRED,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    DIALOG,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    COMMENTARY,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    EMERGENCY,

    /**
     * A filter to strip out ALP message header and be a data source of another
     * filter.
     */
    VOICEOVER,
}