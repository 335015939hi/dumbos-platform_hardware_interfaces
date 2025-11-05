/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.contexthub;

import android.hardware.contexthub.MotionState;

@VintfStability
parcelable MinimumTimeSinceState {
    MotionState state;
    int minimumTimeSinceStateMs;
}

@VintfStability
parcelable MotionEvent {
    // The previous motion state of the device.
    MotionState previousState;

    // The current motion state of the device.
    MotionState currentState;

    // The duration in milliseconds the device has been in the current motion state.
    @nullable int durationMs;
}

@VintfStability
parcelable MotionSubscription {
    // The target motion state of the subscription.
    MotionState targetState;

    // The time in milliseconds the device must be in the target state before the
    // subscription is considered satisfied.
    int dwellTimeMs;

    // An optional list of criteria for throttling the reporting of motion events.
    // Each MinimumTimeSinceState instance represents a state and the minimum amount of time
    // since the device has been in that state in order for the subscription to be satisfied.
    // All criteria must be satisfied simultaneously for an event to be reported to the client.

    // When an event matching this subscription is reported to the client, it will not be reported
    // again until the current state has changed.

    // If a single MotionState is listed more than once, the largest minimumTimeSinceStateMs will be
    // used. Listing the targetState in the eventCriteria list is invalid and will be ignored.
    @nullable MinimumTimeSinceState eventCriteria[];

    // TODO: Should dwellTime be generalized into this list as well?
    // TODO: How can we reduce complexity around this?
}
