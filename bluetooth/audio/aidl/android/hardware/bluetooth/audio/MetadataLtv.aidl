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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.AudioContext;

@VintfStability
union MetadataLtv {
    parcelable PreferredAudioContexts {
        AudioContext[] values;
    }

    parcelable StreamingAudioContexts {
        AudioContext[] values;
    }

    parcelable ProgramInfo {
        String title;
    }

    parcelable Language {
        byte[3] iso6393Code;
    }

    parcelable CCIDList {
        int[] values;
    }

    parcelable ParentalRating {
        int value;
    }

    parcelable ProgramInfoURI {
        String url;
    }

    parcelable ExtendedMetadata {
        int type;
        byte[] value;
    }

    parcelable VendorSpecific {
        int companyId;
        byte[] value;
    }

    PreferredAudioContexts preferredAudioContexts;
    StreamingAudioContexts streamingAudioContexts;
    ProgramInfo programInfo;
    Language language;
    CCIDList ccidList;
    ParentalRating parentalRating;
    ProgramInfoURI programInfoURI;
    ExtendedMetadata extendedMetadata;
    VendorSpecific vendorSpecific;
}
