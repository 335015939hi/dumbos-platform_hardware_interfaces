#include <android/hardware/dumpstate/1.1/IDumpstateDevice.h>
#include <android/hardware/dumpstate/1.1/types.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

namespace {
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

namespace ahd = ::android::hardware::dumpstate;
using IDumpstateDevice1 = ahd::V1_1::IDumpstateDevice;
using DumpstateMode1 = ahd::V1_1::DumpstateMode;
using DumpstateStatus1 = ahd::V1_1::DumpstateStatus;

struct DumpstateDevice : public IDumpstateDevice1 {
    // 1.1
    Return<DumpstateStatus1> dumpstateBoard_1_1(const hidl_handle& handle,
                                                const DumpstateMode1 mode,
                                                uint64_t /*timeoutMillis*/) override {
        if (handle == nullptr || handle->numFds < 1) {
            return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }

        int fd = handle->data[0];
        if (fd < 0) {
            return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }

        switch (mode) {
            case DumpstateMode1::FULL:
                return dumpstateBoardImpl(fd, true);

            case DumpstateMode1::DEFAULT:
                return dumpstateBoardImpl(fd, false);

            case DumpstateMode1::INTERACTIVE:
            case DumpstateMode1::REMOTE:
            case DumpstateMode1::WEAR:
            case DumpstateMode1::CONNECTIVITY:
            case DumpstateMode1::WIFI:
            case DumpstateMode1::PROTO:
                return DumpstateStatus1::UNSUPPORTED_MODE;

            default:
                return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }
    }

    Return<void> setVerboseLoggingEnabled(bool enable) override {
        verboseEnabled = enable;
        return {};
    }

    Return<bool> getVerboseLoggingEnabled() override { return verboseEnabled; }

    // 1.0
    Return<void> dumpstateBoard(const hidl_handle& h) override {
        dumpstateBoard_1_1(h, DumpstateMode1::DEFAULT, 0);
        return {};
    }

    DumpstateStatus1 dumpstateBoardImpl(const int fd, const bool full) {
        static const char veroseStr[] = {'v', 'e', 'r', 'b', 'o', 's', 'e', ':', ' '};

        static const char fullDumpStr[] = {'H', 'e', 'l', 'l', 'o', ' ',
                                           'w', 'o', 'r', 'l', 'd', '!'};
        static const char defaultDumpStr[] = {'t', 'e', 's', 't'};

        if (verboseEnabled) {
            write(fd, veroseStr, sizeof(veroseStr));
        }

        if (full) {
            write(fd, fullDumpStr, sizeof(fullDumpStr));
        } else {
            write(fd, defaultDumpStr, sizeof(defaultDumpStr));
        }

        return DumpstateStatus1::OK;
    }

    bool verboseEnabled = false;
};
}  // namespace

int main(int, char**) {
    using ::android::OK;
    using ::android::sp;
    using ::android::hardware::configureRpcThreadpool;
    using ::android::hardware::joinRpcThreadpool;

    sp<IDumpstateDevice1> dumpstate(new DumpstateDevice);

    // This method MUST be called before interacting with any HIDL interfaces.
    configureRpcThreadpool(1, true);

    if (dumpstate->registerAsService() != OK) {
        ALOGE("Could not register service.");
        return 1;
    }

    joinRpcThreadpool();
    return 0;
}
