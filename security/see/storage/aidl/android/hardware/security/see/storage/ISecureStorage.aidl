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
package android.hardware.security.see.storage;

import android.hardware.security.see.storage.FileProperties;
import android.hardware.security.see.storage.OpenOptions;
import android.hardware.security.see.storage.result.NextFilenameResult;
import android.hardware.security.see.storage.result.OpenDirResult;
import android.hardware.security.see.storage.result.OpenFileResult;
import android.hardware.security.see.storage.result.ReadFileResult;
import android.hardware.security.see.storage.result.Result;
import android.hardware.security.see.storage.result.WriteFileResult;

interface ISecureStorage {
    OpenFileResult OpenFile(
            in String fileName, in FileProperties fileProperties, in OpenOptions options);
    ReadFileResult ReadFile(long fd, long offset);
    WriteFileResult WriteFile(long fd, long offset, in byte[] buffer);
    Result CloseFile(long fd);
    Result DeleteFile(in String fileName, in FileProperties fileProperties);

    OpenDirResult OpenDir(in FileProperties fileProperties);
    NextFilenameResult DirReadNextFileName(long dirHandle);
    Result CloseDir(long dirHandle);
}
