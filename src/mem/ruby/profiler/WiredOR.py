from m5.params import *
from m5.SimObject import SimObject


class WiredOR(SimObject):
    type = 'WiredOR'
    cxx_class = 'gem5::ruby::WiredOR'
    cxx_header = 'mem/ruby/profiler/WiredOR.hh'
