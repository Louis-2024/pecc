import argparse

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath

addToPath("../")

from common import Options
from ruby import Ruby


parser = argparse.ArgumentParser(
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)
Options.addNoISAOptions(parser)
Ruby.define_options(parser)
args = parser.parse_args()

# INCL/EXCL require TimingSimpleCPU in their Ruby protocol setup even though
# this configuration drives the protocol through RubyTester instead of CPUs.
args.cpu_type = "TimingSimpleCPU"

check_flush = buildEnv["PROTOCOL"] == "MOESI_hammer"

tester = RubyTester(
    check_flush=check_flush,
    checks_to_complete=1048576,
    wakeup_frequency=10,
    num_cpus=args.num_cpus,
    deadlock_threshold=5000000,
)

system = System(
    cpu=tester,
    mem_ranges=[AddrRange(args.mem_size)],
)

system.voltage_domain = VoltageDomain(voltage=args.sys_voltage)
system.clk_domain = SrcClockDomain(
    clock="2GHz",
    voltage_domain=system.voltage_domain,
)

cpu_list = [system.cpu] * args.num_cpus
Ruby.create_system(args, False, system, cpus=cpu_list)

system.ruby.clk_domain = SrcClockDomain(
    clock="4GHz",
    voltage_domain=system.voltage_domain,
)
system.ruby.randomization = False

assert args.num_cpus == len(system.ruby._cpu_ports)

tester.num_cpus = len(system.ruby._cpu_ports)

for ruby_port in system.ruby._cpu_ports:
    if ruby_port.support_data_reqs and ruby_port.support_inst_reqs:
        tester.cpuInstDataPort = ruby_port.in_ports
    elif ruby_port.support_data_reqs:
        tester.cpuDataPort = ruby_port.in_ports
    elif ruby_port.support_inst_reqs:
        tester.cpuInstPort = ruby_port.in_ports

    ruby_port.no_retry_on_stall = True
    ruby_port.using_ruby_tester = True

root = Root(full_system=False, system=system)
root.system.mem_mode = "timing"

m5.ticks.setGlobalFrequency("0.25ns")
m5.instantiate()

exit_event = m5.simulate(args.abs_max_tick)
print("Exiting @ tick", m5.curTick(), "because", exit_event.getCause())
