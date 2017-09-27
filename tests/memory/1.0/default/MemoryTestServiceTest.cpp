#include <android/hardware/tests/memory/1.0/IMemoryTest.h>
#include <hidl/HidlSupport.h>
#include <hidl/LegacySupport.h>
#include <utils/RefBase.h>
#include <iostream>

using namespace std;
using ::android::hardware::hidl_memblk;
using ::android::hardware::registerPassthroughServiceImplementation;
using ::android::hardware::tests::memory::V1_0::IMemoryTest;
using ::android::sp;

#define ASSERT(cond)                                                                              \
    do {                                                                                          \
        if (!(cond)) {                                                                            \
            cerr << __func__ << ":" << __LINE__ << " condition:" << #cond << " failed\n" << endl; \
            cleanup();                                                                            \
            exit(EXIT_FAILURE);                                                                   \
        }                                                                                         \
    } while (0)

static void cleanup(void);

// Pipe is a object used for IPC between parent process and child process.
// This IPC class is widely used in binder/hwbinder tests.
// The common usage is the main process to create the Pipe and forks.
// Both parent and child hold a object. Each recv() on one side requires
// a send() on the other side to unblock.
class Pipe : public virtual android::RefBase {
   public:
    static tuple<android::sp<Pipe>, android::sp<Pipe>> createPipePair();
    Pipe(Pipe&& rval);
    ~Pipe();
    inline void signal() {
        bool val = true;
        send(val);
    }
    inline void wait() {
        bool val = false;
        recv(val);
    }

    // write a data struct
    template <typename T>
    int send(const T& v) {
        return write(fd_write_, &v, sizeof(T));
    }
    // read a data struct
    template <typename T>
    int recv(T& v) {
        return read(fd_read_, &v, sizeof(T));
    }

   private:
    int fd_read_;   // file descriptor to read
    int fd_write_;  // file descriptor to write
    Pipe(int read_fd, int write_fd) : fd_read_{read_fd}, fd_write_{write_fd} {}
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe& operator=(const Pipe&&) = delete;
};

tuple<sp<Pipe>, sp<Pipe>> Pipe::createPipePair() {
    int a[2];
    int b[2];

    int error1 = pipe(a);
    int error2 = pipe(b);
    ASSERT(error1 >= 0);
    ASSERT(error2 >= 0);

    return make_tuple(new Pipe(a[0], b[1]), new Pipe(b[0], a[1]));
}

Pipe::Pipe(Pipe&& rval) noexcept {
    fd_read_ = rval.fd_read_;
    fd_write_ = rval.fd_write_;
    rval.fd_read_ = 0;
    rval.fd_write_ = 0;
}

Pipe::~Pipe() {
    if (fd_read_) {
        close(fd_read_);
    }
    if (fd_write_) {
        close(fd_write_);
    }
}

static vector<sp<Pipe>> children;

/**
 *  Start a MemoryTest process and register as an IMemoryTest service.
 *  The IMemoryTest simulates a hardware interface that pass hidl_memory.
 */
static void startMemoryTest() {
    auto pipe_pair = Pipe::createPipePair();
    pid_t pid = fork();
    if (pid) {
        sp<Pipe> service = get<0>(pipe_pair);
        service->wait();
        cout << "MemoryTest: initialized" << endl;
        children.push_back(service);
    } else {
        sp<Pipe> main = get<1>(pipe_pair);
        if (registerPassthroughServiceImplementation<IMemoryTest>("memory") != ::android::OK) {
            cerr << "Failed to register service IMemoryTest" << endl;
            exit(-1);
        }
        main->signal();
        main->wait();
        exit(0);
    }
}

static void test1() {
    sp<IMemoryTest> mt = IMemoryTest::getService("memory");
    ASSERT(mt != nullptr);
    hidl_memblk memblk("abc", 1234, 5678, 0xabcd), memblkb;
    mt->setMemBlk(memblk);
    mt->getMemBlk([&memblkb](const hidl_memblk& _memblk) { memblkb = _memblk; });
    ASSERT(memblkb.mHeapID == 1234);
    ASSERT(memblkb.mSize == 5678);
    ASSERT(memblkb.mOffset == 0xabcd);
    ASSERT(memblkb.mType == "abc");
}

static void cleanup(void) {
    for (sp<Pipe> child : children) {
        int status;
        child->signal();
        wait(&status);
    }
}

int main() {
    setenv("TREBLE_TESTING_OVERRIDE", "true", true);
    startMemoryTest();
    test1();
    cleanup();
    cout << "OK" << endl;
    return 0;
}
