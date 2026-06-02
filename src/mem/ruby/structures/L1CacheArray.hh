/*
 * Helper object for protocols that need read-only visibility into all L1
 * cache tag arrays.
 */

#ifndef __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__
#define __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__

#include <vector>

#include "base/types.hh"
#include "mem/ruby/structures/CacheMemory.hh"
#include "params/RubyL1CacheArray.hh"
#include "sim/sim_object.hh"

namespace gem5
{

namespace ruby
{

class L1CacheArray : public SimObject
{
  public:
    typedef RubyL1CacheArrayParams Params;

    L1CacheArray(const Params &p);

    bool isTagPresent(Addr address) const;
    bool isL1ICacheTagPresent(int core, Addr address) const;
    bool isL1DCacheTagPresent(int core, Addr address) const;
    int getNumCores() const { return m_numCores; }

  private:
    bool validCore(int core) const;

    std::vector<CacheMemory *> m_l1iCaches;
    std::vector<CacheMemory *> m_l1dCaches;
    int m_numCores;
};

} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__
