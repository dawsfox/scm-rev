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

  std::string ClockPort = params.find<std::string>("clockPort", "clock");

  const int NumPorts = params.find<int>( "num_ports", 0 );
  InfoVec.resize( static_cast<size_t>(NumPorts) );

  InitLinkConfig( params );
  InitPortMap( params );
  // clock is called by RevCPU instead?
  //This would be used to register the clock with SST Core
  /*registerClock( ClockFreq,
    new Clock::Handler<RevCodeletCoProc>(this, &RevCodeletCoProc::ClockTick));
    output->output("Registering subcomponent RevCodeletCoProc with frequency=%s\n", ClockFreq.c_str());*/
}

void RevCodeletCoProc::splitStr(const std::string& s,
                                char c,
                                std::vector<std::string>& v){
  std::string::size_type i = 0;
  std::string::size_type j = s.find(c);
  v.clear();

  if( (j==std::string::npos) && (s.length() > 0) ){
    v.push_back(s);
    return ;
  }

  while (j != std::string::npos) {
    v.push_back(s.substr(i, j-i));
    i = ++j;
    j = s.find(c, j);
    if (j == std::string::npos)
      v.push_back(s.substr(i, s.length()));
  }
}

void RevCodeletCoProc::InitPortMap( const SST::Params& params ) {
  std::vector<std::string> optList;
  output->verbose( CALL_INFO, 1, VerilatorSST::VerboseMasking::INIT, "Initializing PortMap\n" );
  // get port information from params list
  params.find_array( "portMap", optList );
  for( size_t i=0; i<optList.size(); i++ ){
    output->verbose( CALL_INFO, 1, VerilatorSST::VerboseMasking::INIT, "Port map entry: %s\n", optList[i].c_str() );
    std::vector<std::string> vstr;
    const std::string s = optList[i];
    splitStr(s, ':', vstr);
    if( vstr.size() != 4 ){
      output->fatal(CALL_INFO, -1,
                    "Error in reading value from portMap parameter:%s\n",
                    s.c_str() );
    }
    // store ID, size, type, and acceptable operations for the port
    const long unsigned portId = std::stoul( vstr[1] );
    const long unsigned portSize = std::stoul( vstr[2] );
    const long unsigned portType = std::stoul( vstr[3] );
    const bool portIsWriteable = (static_cast<uint8_t>(portType) & static_cast<uint8_t>(VerilatorSST::VPortType::V_INPUT)) > 0;
    const bool portIsReadable = (static_cast<uint8_t>(portType) & static_cast<uint8_t>(VerilatorSST::VPortType::V_OUTPUT)) > 0;
    // put the port info in the map and the info vector
    PortMap[vstr[0]] = PortDef( static_cast<uint32_t>(portId), static_cast<uint32_t>(portSize), portIsWriteable, portIsReadable ); 
    InfoVec[portId].PortId = static_cast<uint32_t>(portId);
    InfoVec[portId].Size = static_cast<uint32_t>(portSize); 
    InfoVec[portId].Write = portIsWriteable;
    InfoVec[portId].Read = portIsReadable;
  }
}

void RevCodeletCoProc::InitLinkConfig( const SST::Params& params ) {
  const unsigned NumPorts = params.find<unsigned>( "num_ports", 0 );
  if ( NumPorts > 0 ) {
    Links = new SST::Link *[NumPorts];
    // configure a link for each port
    for (unsigned i=0; i<NumPorts; i++) {
      char PortName[8];
      std::snprintf(PortName, 7, "port%zu", static_cast<size_t>(i) );
      Links[i] = configureLink( PortName, "0ns", new Event::Handler2<RevCodeletCoProc, &RevCodeletCoProc::RecvPortEvent, unsigned>( this,  i ) );
      if ( Links[i] == nullptr ) {
        output->fatal( CALL_INFO, -1, "Error: Link for port %s failed to be configured\n", PortName );
      }
    }
  } else {
    output->fatal( CALL_INFO, -1, "Error: initialized with no links\n" );
  }
}

void RevCodeletCoProc::RecvPortEvent( SST::Event* ev, unsigned portId ) {
  output->verbose( CALL_INFO, 2, 0, "Received an event\n");
  SST::VerilatorSST::PortEvent* fromPort = static_cast<SST::VerilatorSST::PortEvent*>( ev );
  if(fromPort) {

    if(portId == 0) {
    // event from clk
    } else if (portId == 1) {
    // event from resetn
    } else if (portId == 2) {
    // event from mem_ready
    } else if (portId == 3) {
    // event from pcpi_wr
    } else if (portId == 4) {
    // event from pcpi_wait
    } else if (portId == 5) {
    // event from pcpi_ready
    } else if (portId == 6) {
    // event from mem_rdata
    } else if (portId == 7) {
    // event from pcpi_rd
    } else if (portId == 8) {
    // event from irq
    } else if (portId == 9) {
    // event from trap
    } else if (portId == 10) {
    // event from mem_valid
    } else if (portId == 11) {
    // event from mem_instr
    } else if (portId == 12) {
    // event from mem_wstrb
    } else if (portId == 13) {
    // event from mem_la_read
    } else if (portId == 14) {
    // event from mem_la_write
    } else if (portId == 15) {
    // event from mem_la_wstrb
    } else if (portId == 16) {
    // event from pcpi_valid
    } else if (portId == 17) {
    // event from trace_valid
    } else if (portId == 18) {
    // event from mem_addr
    } else if (portId == 19) {
    // event from mem_wdata
    } else if (portId == 20) {
    // event from mem_la_addr
    } else if (portId == 21) {
    // event from mem_la_wdata
    } else if (portId == 22) {
    // event from pcpi_insn
    } else if (portId == 23) {
    // event from pcpi_rs1
    } else if (portId == 24) {
    // event from pcpi_rs2
    } else if (portId == 25) {
    // event from eoi
    } else if (portId == 26) {
    // event from trace_data
    } else {
      output->fatal( CALL_INFO, -1, "Error: portId not recognized\n" );
    }
  delete fromPort;
  return;
  }
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
