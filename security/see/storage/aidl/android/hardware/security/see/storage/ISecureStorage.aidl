package android.hardware.security.see.storage;

@Backing(type="int")
enum ErrorCode {
    GENERIC,
    // TODO: Add other error types
}

parcellable Monostate {}

union Result {
    ErrorCode error = ErrorCode.GENERIC;
    Monostate ok;
}

union OpenFileResult {
    ErrorCode error = ErrorCode.GENERIC;
    // TODO: Should we make a FileDescriptor newtype?
    long file_descriptor;
}

union ReadFileResult {
    ErrorCode error = ErrorCode.GENERIC;
    long bytes_read;
}

union WriteFileResult {
    // TODO: Should these all use the same error code type or do we want one per method?
    ErrorCode error = ErrorCode.GENERIC;
    long bytes_written;
}

union OpenDirResult {
    ErrorCode error = ErrorCode.GENERIC;
    // TODO: Should we make a DirHandle newtype?
    long directory_handle;
}

union NextFilenameResult {
    ErrorCode error = ErrorCode.GENERIC;
    Monostate ok;
    Monostate end_of_file;
}

interface ISecureStorageService {
    OpenFileResult OpenFile(in String fileName, int flags, int operatonFlags);
    ReadFileResult ReadFile(long fd, long offset, out byte[] buffer);
    WriteFileResult WriteFile(long fd, long offset, in byte[] buffer, int operatonFlags);
    Result CloseFile(long fd);
    Result DeleteFile(in String fileName, int operationFlags);

    OpenDirResult OpenDir();
    NextFilenameResult DirReadNextFileName(long dirHandle, out byte[] fileName);
    Result CloseDir(long dirHandle)
}
