#include "Graph.h"

namespace android {
namespace hardware {
namespace tests {
namespace pointer {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::pointer::V1_0::IGraph follow.
Return<void> Graph::passANode(const IGraph::Node& n)  {
    // TODO implement
    return Void();
}

Return<void> Graph::passAGraph(const IGraph::Graph& g)  {
    // TODO implement
    return Void();
}

Return<void> Graph::passTwoGraphs(::android::hardware::tests::pointer::V1_0::IGraph::Graph const* g1, ::android::hardware::tests::pointer::V1_0::IGraph::Graph const* g2)  {
    // TODO implement
    return Void();
}

Return<void> Graph::giveAGraph(giveAGraph_cb _hidl_cb)  {
    // TODO implement
    return Void();
}

Return<void> Graph::passAGamma(const IGraph::Gamma& c)  {
    // TODO implement
    return Void();
}

Return<void> Graph::passASimpleRef(::android::hardware::tests::pointer::V1_0::IGraph::Alpha const* a)  {
    // TODO implement
    return Void();
}

Return<void> Graph::passASimpleRefS(::android::hardware::tests::pointer::V1_0::IGraph::Theta const* s)  {
    // TODO implement
    return Void();
}

Return<void> Graph::giveASimpleRef(giveASimpleRef_cb _hidl_cb)  {
    // TODO implement
    return Void();
}

Return<int32_t> Graph::getErrors()  {
    // TODO implement
    return int32_t {};
}


IGraph* HIDL_FETCH_IGraph(const char* /* name */) {
    return new Graph();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace pointer
}  // namespace tests
}  // namespace hardware
}  // namespace android
