#ifndef __MEM_RUBY_PROFILER_WIREDOR_HH__
#define __MEM_RUBY_PROFILER_WIREDOR_HH__

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

    void setWiredORLine(bool l1Owned);
    bool readAndClearWiredORLine();

  private:
    bool m_l1Owned;
};

} // namespace ruby

} // namespace gem5

#endif // __MEM_RUBY_PROFILER_WIREDOR_HH__
