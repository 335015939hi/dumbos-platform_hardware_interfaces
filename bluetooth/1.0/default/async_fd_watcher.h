#pragma once

#include <mutex>
#include <thread>

using ReadCallback = std::function<void(int)>;

class AsyncFdWatcher {
 public:
  AsyncFdWatcher() = default;
  ~AsyncFdWatcher();

  int WatchFdForNonBlockingReads(int file_descriptor,
                                 const ReadCallback& on_read_fd_ready_callback);
  void StopWatchingFileDescriptor();

 private:
  AsyncFdWatcher(const AsyncFdWatcher&) = delete;
  AsyncFdWatcher& operator=(const AsyncFdWatcher&) = delete;

  int tryStartThread();
  int stopThread();
  int notifyThread();
  void ThreadRoutine();

  std::atomic_bool running_{false};
  std::thread thread_;
  std::mutex internal_mutex_;

  int read_fd_;
  int notification_listen_fd_;
  int notification_write_fd_;
  ReadCallback cb_;
};
