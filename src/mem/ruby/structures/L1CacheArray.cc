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
    L1OwnerResponded.resize(m_numCores, false);
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
L1CacheArray::getL1OwnerResponded(NodeID index) const
{
    return L1OwnerResponded[index];
}

void
L1CacheArray::setL1OwnerResponded(NodeID index, bool val)
{
    L1OwnerResponded[index] = val;
}


bool
L1CacheArray::isTagPresentExceptCore(NodeID excluded_core, Addr address) const
{
    for (int core = 0; core < m_numCores; ++core) {
        if (static_cast<NodeID>(core) == excluded_core) {
            continue;
        }
        AbstractCacheEntry* l1i_entry = m_l1iCaches[core]->lookup(address);
        if (l1i_entry != nullptr && l1i_entry->getValid()) {
            return true;
        }
        AbstractCacheEntry* l1d_entry = m_l1dCaches[core]->lookup(address);
        if (l1d_entry != nullptr && l1d_entry->getValid()) {
            return true;
        }
    }
    return false;
}

bool
L1CacheArray::isTagOwnedExceptCore(NodeID excluded_core, Addr address) const
{
    for (int core = 0; core < m_numCores; ++core) {
        if (static_cast<NodeID>(core) == excluded_core) {
            continue;
        }

        if (getL1OwnerResponded(excluded_core)) {
            return true;
        }

        AbstractCacheEntry* l1i_entry = m_l1iCaches[core]->lookup(address);
        if (l1i_entry != nullptr && l1i_entry->getOwned()) {
            return true;
        }

        AbstractCacheEntry* l1d_entry = m_l1dCaches[core]->lookup(address);
        if (l1d_entry != nullptr && l1d_entry->getOwned()) {
            return true;
        }
    }
    return false;
}

} // namespace ruby
} // namespace gem5
