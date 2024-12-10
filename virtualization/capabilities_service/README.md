The IVmCapabilitiesService HAL is used in a flow to grant a pVM a capability to
issue vendor-specific SMCs. For more information see: TODO(ioffe): link the docs

We provide 2 implementations of this HAL:

* noop - this no-op implementation is used in cuttlefish for mixed build testing
* default - this is a reference implementation that partners can integrate in
  their products.
