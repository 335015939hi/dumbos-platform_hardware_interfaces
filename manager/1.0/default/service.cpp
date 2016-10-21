#define LOG_TAG "android.hardware.manager@1.0-service"

#include <utils/Log.h>

#include <inttypes.h>
#include <unistd.h>

#include <cutils/properties.h>
#include <hidl/IServiceManager.h>
#include <hidl/Status.h>
#include <hwbinder/IInterface.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

// libutils:
using android::BAD_TYPE;
using android::Looper;
using android::LooperCallback;
using android::OK;
using android::sp;
using android::status_t;
using android::String16;

// libbinder:
using android::hardware::BBinder;
using android::hardware::BnInterface;
using android::hardware::defaultServiceManager;
using android::hardware::IBinder;
using android::hardware::IInterface;
using android::hardware::IPCThreadState;
using android::hardware::Parcel;
using android::hardware::ProcessState;

// Standard library
using std::string;
using std::unique_ptr;
using std::vector;

class BinderCallback : public LooperCallback {
public:
    BinderCallback() {}
    ~BinderCallback() override {}

    int handleEvent(int /* fd */, int /* events */, void* /* data */) override {
        IPCThreadState::self()->handlePolledCommands();
        return 1;  // Continue receiving callbacks.
    }
};

int main() {
    android::sp<ServiceManager> service = new ServiceManager();

    service->registerAsService("manager");

    sp<Looper> looper(Looper::prepare(0 /* opts */));

    int binder_fd = -1;

    IPCThreadState::self()->setupPolling(&binder_fd);
    if (binder_fd < 0) {
    // hwservicemanager is a critical service; until support for /dev/hwbinder
    // is checked in for all devices, prevent it from exiting; if it were to
    // exit, it would get restarted again and fail again several times,
    // eventually causing the device to boot into recovery mode.
    // TODO: revert
    while (true) {
      sleep(UINT_MAX);
    }
    return -1;
    }

    sp<BinderCallback> cb(new BinderCallback);
    if (looper->addFd(binder_fd, Looper::POLL_CALLBACK, Looper::EVENT_INPUT, cb,
                    nullptr) != 1) {
    ALOGE("Failed to add binder FD to Looper");
    return -1;
    }

    // Tell IPCThreadState we're the service manager
    IPCThreadState::self()->setTheContextObject(service);
    // Then tell binder kernel
    ioctl(binder_fd, BINDER_SET_CONTEXT_MGR, 0);

    // TODO(smoreland) update property name
    int rc = property_set("hwservicemanager.ready", "true");
    if (rc) {
    ALOGE("Failed to set \"hwservicemanager.ready\" (error %d). "\
          "HAL services will not launch!\n", rc);
    }

    while (true) {
        looper->pollAll(-1 /* timeoutMillis */);
    }

    return 0;
}