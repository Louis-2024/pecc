/*
 * Helper object for protocols that need read-only visibility into all L1
 * cache tag arrays.
 */

#ifndef __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__
#define __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__

#include <vector>

#include "base/types.hh"
#include "mem/ruby/common/TypeDefines.hh"
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

    bool isTagPresentExceptCore(NodeID excluded_core, Addr address) const;
    bool isTagOwnedExceptCore(NodeID excluded_core, Addr address) const;
    int getNumCores() const { return m_numCores; }
    bool getL1OwnerResponded(NodeID index) const;
    void setL1OwnerResponded(NodeID index, bool val);

    std::vector<CacheMemory *> m_l1iCaches;
    std::vector<CacheMemory *> m_l1dCaches;
    int m_numCores;

    std::vector<bool> L1OwnerResponded;
};

} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_STRUCTURES_L1CACHEARRAY_HH__
