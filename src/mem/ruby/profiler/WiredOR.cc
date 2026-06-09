#include "mem/ruby/profiler/WiredOR.hh"

namespace gem5
{

namespace ruby
{

WiredOR::WiredOR(const Params &p)
    : SimObject(p),
      m_l1OwnedAddrs(p.num_banks)
{
}

void
WiredOR::addL1OwnedAddr(NodeID bank, Addr addr)
{
    m_l1OwnedAddrs[bank].insert(addr);
}

bool
WiredOR::isL1OwnedAddr(NodeID bank, Addr addr)
{
    return m_l1OwnedAddrs[bank].count(addr) != 0;
}

void
WiredOR::removeL1OwnedAddr(NodeID bank, Addr addr)
{
    m_l1OwnedAddrs[bank].erase(addr);
}

} // namespace ruby

} // namespace gem5
