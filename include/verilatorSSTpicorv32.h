//
// _verilatorSSTSubcomponent_h_
//
// Copyright (C) 2017-2024 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#ifndef _picorv32_SUBCOMPONENT_H_
#define _picorv32_SUBCOMPONENT_H_

// -- Standard Headers
#include <cassert>
#include <list>
#include <string>
#include <tuple>
#include <vector>

// -- SST Headers
#include "SST.h"

// -- Verilator Headers
#include "VTop.h"
#include "verilated.h"
#include "verilated_vpi.h"
#include "verilatorSSTAPI.h"

namespace SST::VerilatorSST {

typedef void (*DirectWriteFunc)(VTop *, const std::vector<uint8_t> &);
typedef std::vector<uint8_t> (*DirectReadFunc)(VTop *);

// Type to hold necessary Verilator port information
typedef std::tuple<std::string, VPortType,
                   unsigned, // width
                   unsigned, // depth
                   DirectWriteFunc, DirectReadFunc,
                   SST::Statistics::Statistic<uint64_t> *,
                   SST::Statistics::Statistic<uint64_t> *>
    PortEntry;

// ---------------------------------------------------------------
// VerilatorSSTpicorv32
// ---------------------------------------------------------------
class VerilatorSSTpicorv32 : public VerilatorSSTBase {
public:
  SST_ELI_REGISTER_SUBCOMPONENT(VerilatorSSTpicorv32, "verilatorsstpicorv32",
                                "VerilatorSSTpicorv32",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                "Verilator SST picorv32 Wrapper",
                                SST::VerilatorSST::VerilatorSSTBase)

  // Set up parameters accesible from the python configuration
  SST_ELI_DOCUMENT_PARAMS(
      {"useVPI", "Is Verilator VPI used", "false"},
      {"clockFreq", "Sets the clock frequency", "1GHz"},
      {"clockPort", "Sets the internal verilog clock port", "clock"},
      {"resetVals", "Initial reset values for each labeled port", "port:Val"}, )

  // Register any subcomponents used by this element
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()

  // Register any ports used with this element
  SST_ELI_DOCUMENT_PORTS(
      {"clk", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"resetn", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_ready", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_wr", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_wait", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_ready", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_rdata", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rd", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"irq", "Input Port", {"SST::VerilatorSST::PortEvent"}},
      {"trap", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_valid", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_instr", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_wstrb", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_read", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_write", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_wstrb", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_valid", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"trace_valid", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_addr", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_wdata", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_addr", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_wdata", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_insn", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rs1", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rs2", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"eoi", "Output port", {"SST::VerilatorSST::PortEvent"}},
      {"trace_data", "Output port", {"SST::VerilatorSST::PortEvent"}}, )

  // Add statistics
  SST_ELI_DOCUMENT_STATISTICS({"PortWrites",
                               "Counts the total number of input port writes",
                               "writes", 1},
                              {"PortReads",
                               "Counts the total number of output port reads",
                               "reads", 1}, )

  /// default constructor
  VerilatorSSTpicorv32(ComponentId_t id, const Params &params);

  /// default destructor
  virtual ~VerilatorSSTpicorv32();

  /// initialization function
  virtual void init(unsigned int phase) override;

  /// setup function
  virtual void setup() override;

  /// finish function
  virtual void finish() override;

  /// clock tick function
  virtual bool clock(SST::Cycle_t cycle) override;

  /// get the current clock tick from verilator
  virtual uint64_t getCurrentTick() override;

  /// determine if the target port is valid
  virtual bool isNamedPort(std::string PortName) override;

  /// retrieve the number of configured ports
  virtual unsigned getNumPorts() override;

  /// retrieve a vector of all the port names
  virtual const std::vector<std::string> getPortsNames() override;

  /// retrieve the port type of the target port
  virtual bool getPortType(std::string PortName,
                           SST::VerilatorSST::VPortType &direction) override;

  /// retrieve the port width of the target port
  virtual bool getPortWidth(std::string PortName, unsigned &Width) override;

  /// retrieve the port depth of the target port
  virtual bool getPortDepth(std::string PortName, unsigned &Depth) override;

  /// retrieve the port reset value of the target port
  virtual bool getResetVal(std::string PortName, uint64_t &Val) override;

  /// write to the target port
  virtual void writePort(std::string portName,
                         const std::vector<uint8_t> &packet) override;

  /// write to the target port at the target clock cycle
  virtual void writePortAtTick(std::string portName,
                               const std::vector<uint8_t> &packet,
                               uint64_t tick) override;

  /// read from the target port
  virtual std::vector<uint8_t> readPort(std::string portName) override;

private:
  // Private data
  bool UseVPI;                      ///< Is the verilator VPI interface used?
  VerilatedContext *ContextP;       ///< verilated context for the module
  VTop *Top;                        ///< top module
  std::list<QueueEntry> WriteQueue; ///< port write queue
  // Generated links for each port
  SST::Link *link_clk;
  SST::Link *link_resetn;
  SST::Link *link_mem_ready;
  SST::Link *link_pcpi_wr;
  SST::Link *link_pcpi_wait;
  SST::Link *link_pcpi_ready;
  SST::Link *link_mem_rdata;
  SST::Link *link_pcpi_rd;
  SST::Link *link_irq;
  SST::Link *link_trap;
  SST::Link *link_mem_valid;
  SST::Link *link_mem_instr;
  SST::Link *link_mem_wstrb;
  SST::Link *link_mem_la_read;
  SST::Link *link_mem_la_write;
  SST::Link *link_mem_la_wstrb;
  SST::Link *link_pcpi_valid;
  SST::Link *link_trace_valid;
  SST::Link *link_mem_addr;
  SST::Link *link_mem_wdata;
  SST::Link *link_mem_la_addr;
  SST::Link *link_mem_la_wdata;
  SST::Link *link_pcpi_insn;
  SST::Link *link_pcpi_rs1;
  SST::Link *link_pcpi_rs2;
  SST::Link *link_eoi;
  SST::Link *link_trace_data;

  // Private functions

  /// Check for write packets in the queue that need to be performed this tick
  void pollWriteQueue();

  /// Initializes the internal reset values for each port from the parameter
  /// list
  void initResetValues(const Params &params);

  /// Splits a parameter array into tokens of std::string values
  void splitStr(const std::string &s, char c, std::vector<std::string> &v);

  /// VPI Read of Port
  std::vector<uint8_t> readPortVPI(std::string PortName);

  /// VPI Write of Port
  void writePortVPI(std::string PortName, const std::vector<uint8_t> &Packet);

  /// check all inout __en bits match isEnabled argument
  bool verifyInoutEnabledIs(const bool isEnabled, const std::string portName);

  static void DirectWriteclk(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadclk(VTop *);
  static void DirectWriteresetn(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadresetn(VTop *);
  static void DirectWritemem_ready(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_ready(VTop *);
  static void DirectWritepcpi_wr(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_wr(VTop *);
  static void DirectWritepcpi_wait(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_wait(VTop *);
  static void DirectWritepcpi_ready(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_ready(VTop *);
  static void DirectWritemem_rdata(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_rdata(VTop *);
  static void DirectWritepcpi_rd(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_rd(VTop *);
  static void DirectWriteirq(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadirq(VTop *);
  static void DirectWritetrap(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadtrap(VTop *);
  static void DirectWritemem_valid(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_valid(VTop *);
  static void DirectWritemem_instr(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_instr(VTop *);
  static void DirectWritemem_wstrb(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_wstrb(VTop *);
  static void DirectWritemem_la_read(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_la_read(VTop *);
  static void DirectWritemem_la_write(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_la_write(VTop *);
  static void DirectWritemem_la_wstrb(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_la_wstrb(VTop *);
  static void DirectWritepcpi_valid(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_valid(VTop *);
  static void DirectWritetrace_valid(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadtrace_valid(VTop *);
  static void DirectWritemem_addr(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_addr(VTop *);
  static void DirectWritemem_wdata(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_wdata(VTop *);
  static void DirectWritemem_la_addr(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_la_addr(VTop *);
  static void DirectWritemem_la_wdata(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadmem_la_wdata(VTop *);
  static void DirectWritepcpi_insn(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_insn(VTop *);
  static void DirectWritepcpi_rs1(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_rs1(VTop *);
  static void DirectWritepcpi_rs2(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadpcpi_rs2(VTop *);
  static void DirectWriteeoi(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadeoi(VTop *);
  static void DirectWritetrace_data(VTop *, const std::vector<uint8_t> &);
  static std::vector<uint8_t> DirectReadtrace_data(VTop *);

  void handle_clk(SST::Event *ev);
  void handle_resetn(SST::Event *ev);
  void handle_mem_ready(SST::Event *ev);
  void handle_pcpi_wr(SST::Event *ev);
  void handle_pcpi_wait(SST::Event *ev);
  void handle_pcpi_ready(SST::Event *ev);
  void handle_mem_rdata(SST::Event *ev);
  void handle_pcpi_rd(SST::Event *ev);
  void handle_irq(SST::Event *ev);
  void handle_trap(SST::Event *ev);
  void handle_mem_valid(SST::Event *ev);
  void handle_mem_instr(SST::Event *ev);
  void handle_mem_wstrb(SST::Event *ev);
  void handle_mem_la_read(SST::Event *ev);
  void handle_mem_la_write(SST::Event *ev);
  void handle_mem_la_wstrb(SST::Event *ev);
  void handle_pcpi_valid(SST::Event *ev);
  void handle_trace_valid(SST::Event *ev);
  void handle_mem_addr(SST::Event *ev);
  void handle_mem_wdata(SST::Event *ev);
  void handle_mem_la_addr(SST::Event *ev);
  void handle_mem_la_wdata(SST::Event *ev);
  void handle_pcpi_insn(SST::Event *ev);
  void handle_pcpi_rs1(SST::Event *ev);
  void handle_pcpi_rs2(SST::Event *ev);
  void handle_eoi(SST::Event *ev);
  void handle_trace_data(SST::Event *ev);

  // Private data
  std::string clockPort; ///< verilator named clock port

  ///< Map of port indices to reset values
  std::vector<PortReset> ResetVals;

  ///< Vector of port descriptor tuples
  std::vector<PortEntry> Ports = {
      {"clk", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWriteclk,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadclk, nullptr,
       nullptr},
      {"resetn", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWriteresetn,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadresetn, nullptr,
       nullptr},
      {"mem_ready", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_ready,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_ready, nullptr,
       nullptr},
      {"pcpi_wr", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_wr,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_wr, nullptr,
       nullptr},
      {"pcpi_wait", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_wait,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_wait, nullptr,
       nullptr},
      {"pcpi_ready", SST::VerilatorSST::VPortType::V_INPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_ready,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_ready, nullptr,
       nullptr},
      {"mem_rdata", SST::VerilatorSST::VPortType::V_INPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_rdata,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_rdata, nullptr,
       nullptr},
      {"pcpi_rd", SST::VerilatorSST::VPortType::V_INPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_rd,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_rd, nullptr,
       nullptr},
      {"irq", SST::VerilatorSST::VPortType::V_INPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWriteirq,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadirq, nullptr,
       nullptr},
      {"trap", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritetrap,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadtrap, nullptr,
       nullptr},
      {"mem_valid", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_valid,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_valid, nullptr,
       nullptr},
      {"mem_instr", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_instr,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_instr, nullptr,
       nullptr},
      {"mem_wstrb", SST::VerilatorSST::VPortType::V_OUTPUT, 4, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_wstrb,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_wstrb, nullptr,
       nullptr},
      {"mem_la_read", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_la_read,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_la_read, nullptr,
       nullptr},
      {"mem_la_write", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_la_write,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_la_write, nullptr,
       nullptr},
      {"mem_la_wstrb", SST::VerilatorSST::VPortType::V_OUTPUT, 4, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_la_wstrb,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_la_wstrb, nullptr,
       nullptr},
      {"pcpi_valid", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_valid,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_valid, nullptr,
       nullptr},
      {"trace_valid", SST::VerilatorSST::VPortType::V_OUTPUT, 1, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritetrace_valid,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadtrace_valid, nullptr,
       nullptr},
      {"mem_addr", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_addr,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_addr, nullptr,
       nullptr},
      {"mem_wdata", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_wdata,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_wdata, nullptr,
       nullptr},
      {"mem_la_addr", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_la_addr,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_la_addr, nullptr,
       nullptr},
      {"mem_la_wdata", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritemem_la_wdata,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadmem_la_wdata, nullptr,
       nullptr},
      {"pcpi_insn", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_insn,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_insn, nullptr,
       nullptr},
      {"pcpi_rs1", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_rs1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_rs1, nullptr,
       nullptr},
      {"pcpi_rs2", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritepcpi_rs2,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadpcpi_rs2, nullptr,
       nullptr},
      {"eoi", SST::VerilatorSST::VPortType::V_OUTPUT, 32, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWriteeoi,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadeoi, nullptr,
       nullptr},
      {"trace_data", SST::VerilatorSST::VPortType::V_OUTPUT, 36, 1,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectWritetrace_data,
       SST::VerilatorSST::VerilatorSSTpicorv32::DirectReadtrace_data, nullptr,
       nullptr},
  };

  ///< Map of port names to descriptors
  const std::map<std::string, unsigned> PortMap = {
      {"clk", 0},           {"resetn", 1},       {"mem_ready", 2},
      {"pcpi_wr", 3},       {"pcpi_wait", 4},    {"pcpi_ready", 5},
      {"mem_rdata", 6},     {"pcpi_rd", 7},      {"irq", 8},
      {"trap", 9},          {"mem_valid", 10},   {"mem_instr", 11},
      {"mem_wstrb", 12},    {"mem_la_read", 13}, {"mem_la_write", 14},
      {"mem_la_wstrb", 15}, {"pcpi_valid", 16},  {"trace_valid", 17},
      {"mem_addr", 18},     {"mem_wdata", 19},   {"mem_la_addr", 20},
      {"mem_la_wdata", 21}, {"pcpi_insn", 22},   {"pcpi_rs1", 23},
      {"pcpi_rs2", 24},     {"eoi", 25},         {"trace_data", 26},
  };
};

} // namespace SST::VerilatorSST

#endif
