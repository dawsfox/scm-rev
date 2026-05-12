#
# Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
# All Rights Reserved
# contact@tactcomplabs.com
#
# See LICENSE in the top level directory for licensing details
#
# rev-test-ex1.py
#

import os
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

max_addr_gb = 1

# For ease of defining verilator-side ports
class PortDef:
    """ Wrapper class to make port definitions cleaner """
    def __init__(self):
        self.PortList = [ ]
        self.PortId = 0
        self.PortNames = [ ]

    # portName is a string, portSize is an int (measured in bytes), portDir
    # should use READ_PORT, WRITE_PORT, or INOUT_PORT globals
    def addPort(self, portName, portSize, portDir):
        tmp = f"{portName}:{self.PortId}:{portSize}:{portDir}"
        self.PortList.append(tmp)
        self.PortNames.append(portName)
        self.PortId = self.PortId + 1

    def getPortMap(self):
        return(self.PortList)

    def getNumPorts(self):
        return( len(self.PortList) )

    def getPortName(self, index):
        return( self.PortNames[index] )

picoPorts = PortDef()
picoPorts.addPort("clk", 1, WRITE_PORT)
picoPorts.addPort("resetn", 1, WRITE_PORT)
picoPorts.addPort("mem_ready", 1, WRITE_PORT)
picoPorts.addPort("pcpi_wr", 1, WRITE_PORT)
picoPorts.addPort("pcpi_wait", 1, WRITE_PORT)
picoPorts.addPort("pcpi_ready", 1, WRITE_PORT)
picoPorts.addPort("mem_rdata", 4, WRITE_PORT)
picoPorts.addPort("pcpi_rd", 4, WRITE_PORT)
picoPorts.addPort("irq", 4, WRITE_PORT)
picoPorts.addPort("trap", 1, READ_PORT)
picoPorts.addPort("mem_valid", 1, READ_PORT)
picoPorts.addPort("mem_instr", 1, READ_PORT)
picoPorts.addPort("mem_wstrb", 1, READ_PORT)
picoPorts.addPort("mem_la_read", 1, READ_PORT)
picoPorts.addPort("mem_la_write", 1, READ_PORT)
picoPorts.addPort("mem_la_wstrb", 1, READ_PORT)
picoPorts.addPort("pcpi_valid", 1, READ_PORT)
picoPorts.addPort("trace_valid", 1, READ_PORT)
picoPorts.addPort("mem_addr", 4, READ_PORT)
picoPorts.addPort("mem_wdata", 4, READ_PORT)
picoPorts.addPort("mem_la_addr", 4, READ_PORT)
picoPorts.addPort("mem_la_wdata", 4, READ_PORT)
picoPorts.addPort("pcpi_insn", 4, READ_PORT)
picoPorts.addPort("pcpi_rs1", 4, READ_PORT)
picoPorts.addPort("pcpi_rs2", 4, READ_PORT)
picoPorts.addPort("eoi", 4, READ_PORT)
picoPorts.addPort("trace_data", 5, READ_PORT)

# Define the simulation components
comp_cpu = sst.Component("cpu", "revcpu.RevCPU")
comp_cpu.addParams({
        "verbose" : 6,                                # Verbosity
        "numCores" : 1,                               # Number of cores
        "clock" : "1.0GHz",                           # Clock
        "memSize" : 1024*1024*1024,                   # Memory size in bytes
        "machine" : "[0:RV64GC]",                      # Core:Config; RV64GC for core 0
        "enableCoProc" : 1,
        "startAddr" : "[0:0x00000000]",               # Starting address for core 0
        "memCost" : "[0:1:10]",                       # Memory loads required 1-10 cycles
        "program" : os.getenv("REV_EXE", "coproc_ex.exe"),  # Target executable
        "splash" : 1                                  # Display the splash message
})
comp_cpu.enableAllStatistics()

subcomp_codelet_intf = comp_cpu.setSubComponent("co_proc", "revcpu.RevCodeletCoProc")
subcomp_codelet_intf.addParams({
    "clock" : "1.0GHz",
    "verbose" : 6
    })

"""
tester = sst.Component("vtestLink0", "verilatortestlink.VerilatorTestLink")
tester.addParams({
    "verbose" : verbosity,
    "verboseMask" : verbosityMask,
    "clockFreq" : "1GHz",
    "num_ports" : picoPorts.getNumPorts(),
    "portMap" : picoPorts.getPortMap(),
    "testFile" : testFile,
    "testOps" : testScheme.getTest(),
    "numCycles" : numCycles
})
"""

# VerilatorComponent just holds the subcomponent
verilatorsst = sst.Component("vsst", "verilatorcomponent.VerilatorComponent")
verilatorsst.addParams({
    "numCycles" : numCycles
})
# subcomponent contains the actual verilated module
model = verilatorsst.setSubComponent("model", "verilatorsstpicorv32.VerilatorSSTpicorv32")
model.addParams({
    "useVPI" : 0,
    "clockFreq" : "2.0GHz",
    "clockPort" : "clk"
})

Links = [ ]
# connect each verilator subcomponent port with a port on the codelet(coproc) interface
for i in range(picoPorts.getNumPorts()):
    #Links.append( sst.Link( f"link{i}" ) )
    Links.append( sst.Link( f"pico_{picoPorts.getPortName( i )}" ) )
    Links[i].connect( ( model, picoPorts.getPortName( i ), "0ps" ), ( subcomp_codelet_intf, picoPorts.getPortName( i ), "0ps" ) )


sst.setStatisticOutput("sst.statOutputCSV")
sst.enableAllStatisticsForAllComponents()

# EOF
