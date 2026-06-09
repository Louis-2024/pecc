#include "mem/ruby/profiler/WiredOR.hh"

namespace gem5
{

namespace ruby
{

WiredOR::WiredOR(const Params &p)
    : SimObject(p), m_l1Owned(false)
{
}

void
WiredOR::setWiredORLine(bool l1Owned)
{
    m_l1Owned = m_l1Owned || l1Owned;
}

bool
WiredOR::readAndClearWiredORLine()
{
    bool l1Owned = m_l1Owned;
    m_l1Owned = false;
    return l1Owned;
}

} // namespace ruby

} // namespace gem5
