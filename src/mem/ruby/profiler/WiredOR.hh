#ifndef __MEM_RUBY_PROFILER_WIREDOR_HH__
#define __MEM_RUBY_PROFILER_WIREDOR_HH__

#include <unordered_set>
#include <vector>

#include "mem/ruby/common/TypeDefines.hh"
#include "params/WiredOR.hh"
#include "sim/sim_object.hh"

namespace gem5
{

namespace ruby
{

class WiredOR : public SimObject
{
  public:
    typedef WiredORParams Params;
    WiredOR(const Params &p);

    void addL1OwnedAddr(NodeID bank, Addr addr);
    bool isL1OwnedAddr(NodeID bank, Addr addr);
    void removeL1OwnedAddr(NodeID bank, Addr addr);

  private:
    std::vector<std::unordered_set<Addr>> m_l1OwnedAddrs;
};

} // namespace ruby

} // namespace gem5

#endif // __MEM_RUBY_PROFILER_WIREDOR_HH__
