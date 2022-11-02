# KeyMint HAL

This directory contains the HAL definition for KeyMint. KeyMint provides
cryptographic services in a hardware-isolated environment.

Note that the Remote Key Provisioning (RKP) HAL used to also be defined in this
directory. As of Android U, the RKP HAL has been moved to a different directory
(../rkp). This move is ABI compatible, as the interfaces have been maintained.
The build is split so that the generated code may be built with different
options.
