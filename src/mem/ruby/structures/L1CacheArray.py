from m5.params import *
from m5.SimObject import SimObject


class RubyL1CacheArray(SimObject):
    type = "RubyL1CacheArray"
    cxx_class = "gem5::ruby::L1CacheArray"
    cxx_header = "mem/ruby/structures/L1CacheArray.hh"

    l1i_caches = VectorParam.RubyCache([], "Per-core L1 instruction caches")
    l1d_caches = VectorParam.RubyCache([], "Per-core L1 data caches")
    num_cores = Param.Int(0, "Number of cores represented by the L1 arrays")
