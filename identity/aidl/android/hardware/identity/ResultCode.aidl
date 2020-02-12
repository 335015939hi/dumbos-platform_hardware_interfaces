/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;

/**
 * All binder calls may return a ServiceSpecificException with the following error codes:
 */
@VintfStability
@Backing(type="int")
enum ResultCode {
    /**
     * Success.
     */
    OK = 0,

    /**
     * The operation failed. This is used as a generic catch-all for errors that don't belong
     * in other categories, including memory/resource allocation failures and I/O errors.
     */
    FAILED = 1,

    /**
     * Unsupported cipher suite.
     */
    CIPHER_SUITE_NOT_SUPPORTED = 2,

    /**
     * The passed data was invalid. This is a generic catch all for errors that don't belong
     * in other categories related to parameter validation.
     */
    INVALID_DATA = 3,

    /**
     * The authToken parameter passed to IIdentityCredential.startRetrieval() is not valid.
     */
    INVALID_AUTH_TOKEN = 4,

    /**
     * The itemsRequest parameter passed to IIdentityCredential.startRetrieval() does not meet
     * the requirements described in the documentation for that method.
     */
    INVALID_ITEMS_REQUEST_MESSAGE = 5,

    /**
     * The readerSignature parameter in IIdentityCredential.startRetrieval() is invalid,
     * doesn't contain an embedded certificate chain, or the signature failed to
     * validate.
     */
    READER_SIGNATURE_CHECK_FAILED = 6,

    /**
     * The sessionTranscript passed to startRetrieval() did not contain the ephmeral public
     * key returned by createEphemeralPublicKey().
     */
    EPHEMERAL_PUBLIC_KEY_NOT_FOUND = 7,

    /**
     * An access condition related to user authentication was not satisfied.
     */
    USER_AUTHENTICATION_FAILED = 8,

    /**
     * An access condition related to reader authentication was not satisfied.
     */
    READER_AUTHENTICATION_FAILED = 9,

    /**
     * The request data element has no access control profiles associated so it cannot be accessed.
     */
    NO_ACCESS_CONTROL_PROFILES = 10,

    /**
     * The requested data element is not in the provided non-empty itemsRequest message.
     */
    NOT_IN_REQUEST_MESSAGE = 11,

    /**
     * The passed-in sessionTranscript doesn't match the previously passed-in sessionTranscript.
     */
    SESSION_TRANSCRIPT_MISMATCH = 12,
}
