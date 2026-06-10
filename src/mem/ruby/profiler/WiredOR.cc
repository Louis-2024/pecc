#include "mem/ruby/profiler/WiredOR.hh"

namespace gem5
{

namespace ruby
{

WiredOR::WiredOR(const Params &p)
    : SimObject(p),
      m_l1OwnedRequests(p.num_banks)
{
}

void
WiredOR::addL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester)
{
    m_l1OwnedRequests[bank].insert({reqID, requester});
}

bool
WiredOR::isL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester)
{
    return m_l1OwnedRequests[bank].count({reqID, requester}) != 0;
}

void
WiredOR::removeL1OwnedRequest(NodeID bank, Cycles reqID, NodeID requester)
{
    m_l1OwnedRequests[bank].erase({reqID, requester});
}

} // namespace ruby

} // namespace gem5
