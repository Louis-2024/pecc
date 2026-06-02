/*
 * Helper object for protocols that need read-only visibility into all L1
 * cache tag arrays.
 */

#include "mem/ruby/structures/L1CacheArray.hh"

#include "base/logging.hh"

namespace gem5
{

namespace ruby
{

L1CacheArray::L1CacheArray(const Params &p)
    : SimObject(p),
      m_l1iCaches(p.l1i_caches.begin(), p.l1i_caches.end()),
      m_l1dCaches(p.l1d_caches.begin(), p.l1d_caches.end()),
      m_numCores(p.num_cores)
{
    fatal_if(m_numCores < 0, "RubyL1CacheArray num_cores must be non-negative");
    fatal_if(m_l1iCaches.size() != static_cast<size_t>(m_numCores),
             "RubyL1CacheArray has %zu L1I caches for %d cores",
             m_l1iCaches.size(), m_numCores);
    fatal_if(m_l1dCaches.size() != static_cast<size_t>(m_numCores),
             "RubyL1CacheArray has %zu L1D caches for %d cores",
             m_l1dCaches.size(), m_numCores);

    for (int core = 0; core < m_numCores; ++core) {
        fatal_if(m_l1iCaches[core] == nullptr,
                 "RubyL1CacheArray L1I cache pointer %d is null", core);
        fatal_if(m_l1dCaches[core] == nullptr,
                 "RubyL1CacheArray L1D cache pointer %d is null", core);
    }
}

bool
L1CacheArray::validCore(int core) const
{
    return core >= 0 && core < m_numCores;
}

bool
L1CacheArray::isTagPresent(Addr address) const
{
    for (int core = 0; core < m_numCores; ++core) {
        if (m_l1iCaches[core]->isTagPresent(address) ||
            m_l1dCaches[core]->isTagPresent(address)) {
            return true;
        }
    }

    return false;
}

bool
L1CacheArray::isL1ICacheTagPresent(int core, Addr address) const
{
    return validCore(core) && m_l1iCaches[core]->isTagPresent(address);
}

bool
L1CacheArray::isL1DCacheTagPresent(int core, Addr address) const
{
    return validCore(core) && m_l1dCaches[core]->isTagPresent(address);
}

} // namespace ruby
} // namespace gem5
