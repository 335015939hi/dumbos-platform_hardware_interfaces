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

#define LOG_TAG "AudioAlsaTest"

#include <android-base/logging.h>
#include <gtest/gtest.h>
// #if defined(__ANDROID__)
// #include "debuggerd/handler.h"
// #endif

#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <vector>

enum { TIMED_RUN_TIMEOUT_SEC, TIMED_RUN_EXEC_AND_ARGS };
using TimedRunParam = std::tuple<int, std::vector<std::string>>;

class ProcessTimedRunner : public ::testing::TestWithParam<TimedRunParam> {};

namespace {

static int gChildPid = 0;
static bool gDebuggerSignalSent = false;

void handleTimeout(int) {
    LOG(ERROR) << "Timeout. Sending signal 35 to child PID " << gChildPid;
#if defined(__ANDROID__)
    sigqueue(gChildPid, 35 /*DEBUGGER_SIGNAL*/, {.sival_int = 0});
#endif
    gDebuggerSignalSent = true;
}

class ScopedSignalHandler {
  public:
    ScopedSignalHandler(int signalNumber, sighandler_t handler)
        : mSignalNumber(signalNumber), mPreviousHandler(signal(signalNumber, handler)) {}
    ~ScopedSignalHandler() { signal(mSignalNumber, mPreviousHandler); }
    const int mSignalNumber;
    sighandler_t mPreviousHandler;
};

class ScopedAlarm {
  public:
    explicit ScopedAlarm(int timeoutSec) { alarm(timeoutSec); }
    ~ScopedAlarm() { alarm(0); }
};

}  // namespace

TEST_P(ProcessTimedRunner, RunProcess) {
    pid_t pid = fork();
    ASSERT_GE(pid, 0);

    int status;
    if (pid == 0) {
        // Child process
        const auto& p = std::get<TIMED_RUN_EXEC_AND_ARGS>(GetParam());
        std::vector<char*> args;
        std::transform(p.begin(), p.end(), std::back_inserter(args),
                       [](const auto& s) { return const_cast<char*>(s.c_str()); });
        args.push_back(nullptr);
        execvp(args[0], args.data());
        LOG(FATAL) << "Error executing '" << args[0] << "'";
        exit(1);
    } else {
        // Parent process
        gChildPid = pid;
        gDebuggerSignalSent = false;
        ScopedSignalHandler handler(SIGALRM, handleTimeout);
        ScopedAlarm alarm(std::get<TIMED_RUN_TIMEOUT_SEC>(GetParam()));
        wait(&status);
    }
    if (WIFEXITED(status)) {
        EXPECT_EQ(0, WEXITSTATUS(status));
        // Note that the child terminated due to debugger signal still exits with code 0.
        EXPECT_FALSE(gDebuggerSignalSent)
                << "The child process timed out, see logcat for the stack";
    } else {
        FAIL() << "The child process did not exit normally";
    }
}

INSTANTIATE_TEST_SUITE_P(AlsaCapturePlayTest, ProcessTimedRunner,
                         ::testing::Values(std::make_tuple(3, std::vector<std::string>{
                                                                      "/system/bin/tinycap",
                                                                      "/data/local/tmp/test.wav",
                                                                      "-T", "1"})));

namespace {

class TestExecutionTracer : public ::testing::EmptyTestEventListener {
  public:
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        TraceTestState("Started", test_info);
    }
    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        TraceTestState("Completed", test_info);
    }
    void OnTestPartResult(const ::testing::TestPartResult& result) override { LOG(INFO) << result; }

  private:
    static void TraceTestState(const std::string& state, const ::testing::TestInfo& test_info) {
        LOG(INFO) << state << " " << test_info.test_suite_name() << "::" << test_info.name();
    }
};

}  // namespace

int main(int argc, char** argv) {
    android::base::SetMinimumLogSeverity(android::base::DEBUG);
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    return RUN_ALL_TESTS();
}
