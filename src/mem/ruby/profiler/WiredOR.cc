#include "mem/ruby/profiler/WiredOR.hh"

namespace gem5
{

namespace ruby
{

WiredOR::WiredOR(const Params &p)
    : SimObject(p),
      m_l1Owned(p.num_banks, false),
      m_queuedL1Owned(p.num_banks)
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

bool
WiredOR::enqueueL1OwnedSample(NodeID bank)
{
    bool l1Owned = readAndClearWiredORLine(bank);
    m_queuedL1Owned[bank].push_back(l1Owned);
    return l1Owned;
}

bool
WiredOR::dequeueL1OwnedSample(NodeID bank)
{
    bool l1Owned = m_queuedL1Owned[bank].front();
    m_queuedL1Owned[bank].pop_front();
    return l1Owned;
}

bool
WiredOR::queueEmpty(NodeID bank)
{
    return m_queuedL1Owned[bank].empty();
}

} // namespace ruby

} // namespace gem5
