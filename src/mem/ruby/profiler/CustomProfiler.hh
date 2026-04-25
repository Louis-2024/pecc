#ifndef __MEM_RUBY_PROFILER_CUSTOMPROFILER_HH__
#define __MEM_RUBY_PROFILER_CUSTOMPROFILER_HH__

#include "base/statistics.hh"
#include "params/CustomProfiler.hh"
#include "sim/sim_object.hh"

namespace gem5
{

namespace ruby
{

class CustomProfiler : public SimObject
{
    public:
        typedef CustomProfilerParams Params;
        CustomProfiler(const Params &p);

    protected:
        struct CustomProfilerStats : public statistics::Group
        {
            CustomProfilerStats(statistics::Group *parent);

            statistics::Scalar m_num_get_request;
            statistics::Scalar m_num_l1_hit;
            statistics::Scalar m_num_l1_miss;
            statistics::Scalar m_num_upg;
            statistics::Scalar m_num_ctc;
            statistics::Scalar m_num_llc_hit;
            statistics::Scalar m_num_memory_read;
            statistics::Scalar m_num_memory_write;
            statistics::Scalar m_num_back_invalidation;
            statistics::Scalar m_num_back_invalidation_wb;
            statistics::Scalar m_num_put_request;
            statistics::Histogram m_mem_latency_hist;

            statistics::Scalar m_num_writeback_from_l1_to_l2;
            statistics::Scalar m_num_l2_reads;
            statistics::Scalar m_num_l2_writes;

            statistics::Scalar m_num_repetitive_l2_writes;
        } customProfilerStats;

    public:
        // These function increments the associated statistics counter by one 
        // each time they are called
        void profileGetRequest();
        void profileL1Hit();
        void profileL1Miss();
        void profileUpg();
        void profileCacheToCacheTrf();
        void profileLLCHit();
        void profileMemoryRead();
        void profileMemoryWrite();
        void profileBackInvalidation();
        void profileBackInvalidationWB();
        void profilePutRequest();
        void profileMemLatency(Cycles latency);

        void profileWritebackFromL1ToL2();
        void profileL2Reads();
        void profileL2Writes();
        void profileRepetitiveL2Writes();
};


} // namespace ruby


} // namespace gem5

#endif // __MEM_RUBY_PROFILER_CUSTOMPROFILER_HH__
