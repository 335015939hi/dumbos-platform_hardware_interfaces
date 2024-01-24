/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for the Socket interface interface to radio
 * (RCP).
 */

#include "lib/spinel/spinel_interface.hpp"
#include "lib/url/url.hpp"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

/**
 * Defines a Socket interface to the Radio Co-processor (RCP)
 *
 */
class SocketInterface : public ot::Spinel::SpinelInterface {
  public:
    /**
     * Initializes the object.
     *
     * @param[in] aRadioUrl  RadioUrl parsed from radio url.
     *
     */
    explicit SocketInterface(const ot::Url::Url& aRadioUrl);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~SocketInterface();

    /**
     * Returns the RCP interface metrics.
     *
     * @return The RCP interface metrics.
     *
     */
    const otRcpInterfaceMetrics* GetRcpInterfaceMetrics(void) const { return &mInterfaceMetrics; }

    /**
     * Indicates whether or not the given interface matches this interface name.
     *
     * @param[in] aInterfaceName A pointer to the interface name.
     *
     * @retval TRUE   The given interface name matches this interface name.
     * @retval FALSE  The given interface name doesn't match this interface
     * name.
     */
    static bool IsInterfaceNameMatch(const char* aInterfaceName) {
        static const char kInterfaceName[] = "spinel+socket";
        return (strncmp(aInterfaceName, kInterfaceName, strlen(kInterfaceName)) == 0);
    }

  private:
    void CheckIfSocketIsOpen(const ot::Url::Url& aRadioUrl);

    enum {
        kMaxSelectTime = 2000,  ///< Maximum wait time in Milliseconds for socket
                                ///< to become writable (see `SendFrame`).
    };

    ReceiveFrameCallback mReceiveFrameCallback;
    void* mReceiveFrameContext;
    RxFrameBuffer* mReceiveFrameBuffer;

    int mSockFd;
    const ot::Url::Url& mRadioUrl;

    otRcpInterfaceMetrics mInterfaceMetrics;

    // Non-copyable, intentionally not implemented.
    SocketInterface(const SocketInterface&);
    SocketInterface& operator=(const SocketInterface&);
};

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
