#ifndef __MEM_RUBY_PROFILER_WIREDOR_HH__
#define __MEM_RUBY_PROFILER_WIREDOR_HH__

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

    void setWiredORLine(NodeID bank, bool l1Owned);
    bool readAndClearWiredORLine(NodeID bank);

  private:
    std::vector<bool> m_l1Owned;
};

} // namespace ruby

} // namespace gem5

#endif // __MEM_RUBY_PROFILER_WIREDOR_HH__
