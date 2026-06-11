#ifndef __MEM_RUBY_PROFILER_WIREDOR_HH__
#define __MEM_RUBY_PROFILER_WIREDOR_HH__

#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/types.hh"
#include "mem/ruby/common/TypeDefines.hh"
#include "params/WiredOR.hh"
#include "sim/sim_object.hh"

namespace gem5
{

namespace ruby
{

struct L1RequestHash
{
    size_t operator()(const std::pair<Cycles, NodeID> &request) const {
        return std::hash<uint64_t>()(static_cast<uint64_t>(request.first)) ^ (std::hash<NodeID>()(request.second) << 1);
    }
};

class WiredOR : public SimObject
{
    public:
        typedef WiredORParams Params;
        WiredOR(const Params &p);

        void addL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester);
        bool isL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester);
        void removeL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester);

        void addL1HeldRequest(NodeID bank, Cycles reqID, NodeID requester);
        bool isL1HeldRequest(NodeID bank, Cycles reqID, NodeID requester);
        void removeL1HeldRequest(NodeID bank, Cycles reqID, NodeID requester);

    private:
        std::vector<std::unordered_set<std::pair<Cycles, NodeID>, L1RequestHash>> m_l1OwnedRequests;
        std::vector<std::unordered_set<std::pair<Cycles, NodeID>, L1RequestHash>> m_l1HeldRequests;
};

} // namespace ruby

} // namespace gem5

#endif // __MEM_RUBY_PROFILER_WIREDOR_HH__
