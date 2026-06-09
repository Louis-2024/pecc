#include "mem/ruby/profiler/WiredOR.hh"

namespace gem5
{

namespace ruby
{

WiredOR::WiredOR(const Params &p)
    : SimObject(p), m_l1Owned(p.num_banks, false)
{
}

void
WiredOR::setWiredORLine(NodeID bank, bool l1Owned)
{
    m_l1Owned[bank] = m_l1Owned[bank] || l1Owned;
}

bool
WiredOR::readAndClearWiredORLine(NodeID bank)
{
    bool l1Owned = m_l1Owned[bank];
    m_l1Owned[bank] = false;
    return l1Owned;
}

} // namespace ruby

} // namespace gem5
