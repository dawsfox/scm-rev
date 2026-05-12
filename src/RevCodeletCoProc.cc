//
// _RevCodeletCoProc_cc_
//
// Copyright (C) 2017-2024 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#include "RevCodeletCoProc.h"

namespace SST::RevCPU {


// ---------------------------------------------------------------
// RevCodeletCoProc
// ---------------------------------------------------------------
RevCodeletCoProc::RevCodeletCoProc(ComponentId_t id, Params& params, RevCore* parent)
  : RevCoProc(id, params, parent), num_instRetired(0) {

  std::string ClockFreq = params.find<std::string>("clock", "1Ghz");
  cycleCount = 0;

  registerStats();

  //This would be used to register the clock with SST Core
  /*registerClock( ClockFreq,
    new Clock::Handler<RevCodeletCoProc>(this, &RevCodeletCoProc::ClockTick));
    output->output("Registering subcomponent RevCodeletCoProc with frequency=%s\n", ClockFreq.c_str());*/
}

// TODO: What does issuing an instruction here look like? Presumably sending the instruction to 
// the pico via native memory interface, but does the Pico need to request it?
bool RevCodeletCoProc::IssueInst(const RevFeature *F, RevRegFile *R, RevMem *M, uint32_t Inst){
  RevCoProcInst inst = RevCoProcInst(Inst, F, R, M);
  std::cout << "CoProc instruction issued: " << std::hex << Inst << std::dec << std::endl;
  //parent->ExternalDepSet(CreatePasskey(), F->GetHartToExecID(), 7, false);
  InstQ.push(inst);
  return true;
}

void RevCodeletCoProc::registerStats(){
  num_instRetired = registerStatistic<uint64_t>("InstRetired");
}

bool RevCodeletCoProc::Reset(){
  InstQ = {};
  return true;
}

bool RevCodeletCoProc::ClockTick(SST::Cycle_t cycle){
  if(!InstQ.empty()){
    uint32_t inst = InstQ.front().Inst;
    //parent->ExternalDepClear(CreatePasskey(), InstQ.front().Feature->GetHartToExecID(), 7, false);
    num_instRetired->addData(1);
    parent->ExternalStallHart(CreatePasskey(), 0);
    InstQ.pop();
    std::cout << "CoProcessor to execute instruction: " << std::hex << inst << std::endl;
    cycleCount = cycle;
  }
    if((cycle - cycleCount) > 500){
      parent->ExternalReleaseHart(CreatePasskey(), 0);
    }
  return true;
}

}  // namespace SST:RevCPU
