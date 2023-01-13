/*
 * Copyright (C) 2023 Google Inc. All Rights Reserved.
 */

#pragma once
#include <grpc++/grpc++.h>
#include <src/core/lib/iomgr/socket_mutator.h>
#include <string>

namespace android::hardware::automotive::remoteaccess {

class BindToDeviceSocketMutator : public grpc_socket_mutator {
  public:
    BindToDeviceSocketMutator(const char* interface_name);

    bool MutateFd(int fd);

  private:
    std::string mIfname;
};

}  // namespace android::hardware::automotive::remoteaccess