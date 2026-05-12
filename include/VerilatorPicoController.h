//
// _VerilatorPicoRV32_h_
//
// Copyright (C) 2017-2026 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#ifndef __VERILATOR_PICORV32_CONTROLLER_H__
#define __VERILATOR_PICORV32_CONTROLLER_H__

// -- Standard Headers
#include <cassert>
#include <list>
#include <string>
#include <tuple>
#include <vector>

// -- SST Headers
#include "SST.h"

// -- Rev Headers 
#include "RevCoProc.h"

namespace SST::RevCPU {

// Need to figure out where to define this class actually. In Rev namespace? 
// Should SCM have a separate namespace?
class SCMReg;

struct RevCodelet {
  RevCodelet() = default;
  RevCodelet(uint32_t inst, RevFeature* F, RevRegFile* R, RevMem* M, SCMReg input1, SCMReg input2, SCMReg output) :
    Inst(inst), Feature(F), RegFile(R), Mem(M), InputReg1(input1), InputReg2(input2), OutputReg(output) {}
  RevCodelet(const RevCodelet& rhs) = default;

  uint32_t      Inst = 0;
  RevFeature*   Feature = nullptr;
  RevRegFile*   RegFile = nullptr;
  RevMem*       Mem = nullptr;
  SCMReg        InputReg1;
  SCMReg        InputReg2;
  SCMReg        OutputReg;
};


// ---------------------------------------------------------------
// RevCodeletCoProc
// ---------------------------------------------------------------
class RevCodeletCoProc : public RevCoProc {
public:
  SST_ELI_REGISTER_SUBCOMPONENT(RevCodeletCoProc, "revcpu",
                                "RevCodeletCoProc",
                                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                "RISCV CoProcessor Controller for SCM compute units",
                                SST::RevCPU::RevCoProc);

  // Set up parameters accesible from the python configuration
  // TODO: What params does this subcomponent need? These can be moved to the subcomponent
  SST_ELI_DOCUMENT_PARAMS(
      {"clockFreq", "Sets the clock frequency", "1GHz"},
      {"clockPort", "Sets the internal verilog clock port", "clock"},
      /* // move to subcomponent
      {"useVPI", "Is Verilator VPI used", "false"},
      {"resetVals", "Initial reset values for each labeled port", "port:Val"}, 
      */
  )

  // Should have subcomponent for converter interface to the specific Verilated model 
  // TODO: define RevVerIntf class and uncomment
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    //{"verintf", "Interface to Verilated Model", SST::RevCPU::RevVerIntf}
  )

  // Register any ports used with this element
  SST_ELI_DOCUMENT_PORTS(
      {"clk", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"resetn", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_ready", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_wr", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_wait", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_ready", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_rdata", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rd", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      {"irq", "Output Port", {"SST::VerilatorSST::PortEvent"}},
      // Outputs above: driving to PICO
      // Inputs below: coming from PICO
      {"trap", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_valid", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_instr", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_wstrb", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_read", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_write", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_wstrb", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_valid", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"trace_valid", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_addr", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_wdata", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_addr", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"mem_la_wdata", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_insn", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rs1", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"pcpi_rs2", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"eoi", "Input port", {"SST::VerilatorSST::PortEvent"}},
      {"trace_data", "Input port", {"SST::VerilatorSST::PortEvent"}}, 
  )

  // Add statistics
  // TODO: what statistics do we want to gather at this level?
  // Num instructions fetched? 
  SST_ELI_DOCUMENT_STATISTICS({"PortWrites",
                               "Counts the total number of input port writes",
                               "writes", 1},
                              {"PortReads",
                               "Counts the total number of output port reads",
                               "reads", 1}, )

  /// default constructor
  RevCodeletCoProc(ComponentId_t id, const Params &params);

  /// default destructor
  virtual ~RevCodeletCoProc();

  // Enum for referencing statistics
  enum CoProcStats{
    InstRetired = 0,
  };

  /// RevCodeletCoProc: clock tick function - currently not registeres with SST, called by RevCPU
  virtual bool ClockTick(SST::Cycle_t cycle);

  void registerStats();

  /// RevCodeletCoProc: Enqueue Inst into the InstQ and return
  virtual bool IssueInst(RevFeature *F, RevRegFile *R, RevMem *M, uint32_t Inst);

  /// RevCodeletCoProc: Reset the co-processor by emmptying the InstQ
  virtual bool Reset();

  /// RevCodeletCoProc: Called when the attached RevProc completes simulation. Could be used to
  ///                   also signal to SST that the co-processor is done if ClockTick is registered
  ///                   to SSTCore vs. being driven by RevCPU
  virtual bool Teardown() { return Reset(); }

  /// RevCodeletCoProc: Returns true if instruction queue is empty
  virtual bool IsDone(){ return InstQ.empty();}


private:
  // Private data
  struct RevCoProcInst {
    RevCoProcInst() = default;
    RevCoProcInst(uint32_t inst, RevFeature* F, RevRegFile* R, RevMem* M) :
      Inst(inst), Feature(F), RegFile(R), Mem(M) {}
    RevCoProcInst(const RevCoProcInst& rhs) = default;

    uint32_t      Inst = 0;
    RevFeature*   Feature = nullptr;
    RevRegFile*   RegFile = nullptr;
    RevMem*       Mem = nullptr;
  };

  /// RevCodeletCoProc: Total number of instructions retired
  Statistic<uint64_t>* num_instRetired;

  /// Queue of instructions sent from attached RevProc
  std::queue<RevCoProcInst> InstQ;

  SST::Cycle_t cycleCount;
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
  // TODO: these need to be updated probably. Should only need handle functions 
  // for links driven by the pico, so not clk/reset/etc
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

};

} // namespace SST::RevCPU

#endif
