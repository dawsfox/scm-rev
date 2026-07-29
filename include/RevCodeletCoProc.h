//
// _RevCodeletCoProc_h_
//
// Copyright (C) 2017-2026 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#ifndef __REV_CODELET_CO_PROC_H__
#define __REV_CODELET_CO_PROC_H__

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

#include "verilatorSSTAPI.h"

namespace SST::RevCPU {

// Need to figure out where to define this class actually. In Rev namespace? 
// Should SCM have a separate namespace?
/*
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
*/

// struct used to define characteristics of each exposed port
struct PortDef {
  uint32_t PortId;
  uint32_t Size;
  bool Write;
  bool Read;

  // Default constructor
  PortDef() : PortId( 0 ), Size( 0 ), Write( false ), Read ( false ) { }

  // Full constructor
  PortDef( uint32_t PortId, uint32_t Size, bool Write, bool Read ) :
              PortId( PortId ), Size( Size ), Write( Write ), Read( Read ) { }
};

/* Save this for later, getting ahead of myself
struct PortState {
  PortDef PortInfo;
  std::vector<uint8_t> * PortData;
  // here is where a pointer to a converter would go
}
*/

typedef std::pair<PortDef, std::vector<uint8_t>*> PortState;

// ---------------------------------------------------------------
// RevCodeletCoProc
// ---------------------------------------------------------------
class RevCodeletCoProc final : public RevCoProc {
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
      {"clockPort", "Name of the internal verilog clock port", "clock"},
      {"resetLowPort", "Name of the internal reset (active low) port if there is one", "NONE"},
      {"resetHighPort", "Name of the internal reset (active high) port if there is one", "NONE"},
      {"num_ports",   "Number of ports",          "0"},
      { "verbose", "Set the verbosity of output for the attached co-processor", "0" },
      {"portMap",     "portname:id:size:direction pairings",     "" },
      /* // move to subcomponent
      {"useVPI", "Is Verilator VPI used", "false"},
      {"resetVals", "Initial reset values for each labeled port", "port:Val"}, 
      */
  );

  // Should have subcomponent for converter interface to the specific Verilated model 
  // TODO: define RevVerIntf class and uncomment
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    //{"verintf", "Interface to Verilated Model", SST::RevCPU::RevVerIntf}
  );

  // Register any ports used with this element
  SST_ELI_DOCUMENT_PORTS(
    {"port%(num_ports)d",
      "Ports which connect to tested verilated subcomponents.",
      {"SST::VerilatorSST::PortEvent", ""}
    }
  );

  // Add statistics
  // TODO: what statistics do we want to gather at this level?
  // Num instructions fetched? 
  SST_ELI_DOCUMENT_STATISTICS({"PortWrites",
                               "Counts the total number of input port writes",
                               "writes", 1},
                              {"PortReads",
                               "Counts the total number of output port reads",
                               "reads", 1}, 
                              {"InstRetired", "Counts the total number of instructions retired", 
                              "instructions", 1},
                              );

  /// default constructor
  RevCodeletCoProc( ComponentId_t id, Params& params, RevCore* parent );

  /// default destructor
  ~RevCodeletCoProc() final;

  /// RevCodeletCoProc: disallow copying and assignment
  RevCodeletCoProc( const RevCodeletCoProc& )            = delete;
  RevCodeletCoProc& operator=( const RevCodeletCoProc& ) = delete;

  // Enum for referencing statistics
  enum CoProcStats{
    InstRetired = 0,
  };

  /// RevCodeletCoProc: clock tick function - currently not registeres with SST, called by RevCPU
  bool ClockTick(SST::Cycle_t cycle) final;

  void registerStats();

  /// RevCodeletCoProc: Enqueue Inst into the InstQ and return
  bool IssueInst(const RevFeature *F, RevRegFile *R, RevMem *M, uint32_t Inst) final;

  /// RevCodeletCoProc: Reset the co-processor by emmptying the InstQ
  bool Reset() final;

  /// RevCodeletCoProc: Called when the attached RevProc completes simulation. Could be used to
  ///                   also signal to SST that the co-processor is done if ClockTick is registered
  ///                   to SSTCore vs. being driven by RevCPU
  bool Teardown() final { 
    printf("CodeletCoProc teardown occurring\n");
    fflush(stdout);
    return Reset(); 
  }

  /// RevCodeletCoProc: We don't use instruction queue, so 
  /// this is based on upper cycle limit -- later we can adjust
  bool IsDone() final { return Done;}

  void InitPortMap( const SST::Params& params );

  void InitLinkConfig( const SST::Params& params );

  void RecvPortEvent( SST::Event* ev, unsigned portId );

  // Splits a parameter array into tokens of std::string values
  void splitStr(const std::string& s, char c, std::vector<std::string>& v);

  uint32_t fourByteConverter( const std::vector<uint8_t>& eventData );



private:
  // Private data
  struct RevCoProcInst {
    RevCoProcInst( uint32_t inst, const RevFeature* F, RevRegFile* R, RevMem* M )
      : Inst( inst ), Feature( F ), RegFile( R ), Mem( M ) {}

    uint32_t const          Inst;
    const RevFeature* const Feature;
    RevRegFile* const       RegFile;
    RevMem* const           Mem;
  };

  void UpdatePortState();

  template<typename T>
  bool writeToPort(std::string portName, T data);

  bool ManageModelReset();

  bool ResetModel();
  bool CheckInstRqst();
  bool ServeInstRqst();
  bool CheckDataRead();
  bool ServeDataRead();
  bool CheckDataWrite();
  bool ServeDataWrite();

  /// RevCodeletCoProc: Total number of instructions retired
  Statistic<uint64_t>* num_instRetired;

  /// Queue of instructions sent from attached RevProc
  std::queue<RevCoProcInst> InstQ;

  SST::Cycle_t cycleCount;
  uint64_t cycleLimit;
  bool Done;

  SST::Link** Links;
  //std::map<std::string, PortDef> PortMap; // access port characteristics based on name
  std::map<std::string, PortState> PortMap;
  std::vector<PortDef> InfoVec;           // access port characteristics by ID number

  // Private functions
  // TODO: these need to be updated probably. Should only need handle functions 
  // for links driven by the pico, so not clk/reset/etc

  // Private data
  std::string ClockPort; ///< verilator named clock ports
  std::string ResetLowPort;
  std::string ResetHighPort;
  std::string ActiveResetPort;
  uint8_t ActiveResetValue;
  RevCore* parent;


  std::vector<uint32_t> CuLocalMem;
  // set high when reset is applied; cleared when it's dropped
  bool ModelResetting;
  bool BeenReset; // stays high after initial reset
  int ResetLength; // number of cycles to hold reset
  int ResetCounter;
  bool MemReqServiced = false;

}; // class RevCodeletCoProc

} // namespace SST::RevCPU

#endif
