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

RevCodeletCoProc::~RevCodeletCoProc() {
  // delete the packet "state" space we created for each port
  for ( auto it = PortMap.begin(); it != PortMap.end(); ++it  ) {
    delete it->second.second;
  }
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
    // also add space for where the current value will be stored
    PortDef port_info( static_cast<uint32_t>(portId), static_cast<uint32_t>(portSize), portIsWriteable, portIsReadable );
    PortState entry(port_info, new std::vector<uint8_t>(portSize));
    PortMap[vstr[0]] = entry;
    //PortMap[vstr[0]] = PortDef( static_cast<uint32_t>(portId), static_cast<uint32_t>(portSize), portIsWriteable, portIsReadable ); 
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
    // Writes don't receive responses, and unless we have an inout port, 
    // we won't need to read any write port, so only implement read data receiving
    if(portId < 10) {
      // This should not be entered, since these are write ports
      output->fatal( CALL_INFO, -1, "Error: response from port associated with writing only\n" );
    // event from clk
    // event from resetn
    // event from mem_ready
    // event from pcpi_wr
    // event from pcpi_wait
    // event from pcpi_ready
    // event from mem_rdata
    // event from pcpi_rd
    // event from irq
    } else if (portId == 9) {
    // event from trap
      // trap and the rest are READ_PORTs
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'trap' PortEvent is not a READ");
      *(PortMap["port9"].second) = fromPort->getPacket();
    } else if (portId == 10) {
    // event from mem_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'memvalid' PortEvent is not a READ");
      *(PortMap["port10"].second) = fromPort->getPacket();
    } else if (portId == 11) {
    // event from mem_instr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_instr' PortEvent is not a READ");
      *(PortMap["port11"].second) = fromPort->getPacket();
    } else if (portId == 12) {
    // event from mem_wstrb
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_wstrb' PortEvent is not a READ");
      *(PortMap["port12"].second) = fromPort->getPacket();
    } else if (portId == 13) {
    // event from mem_la_read
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_la_read' PortEvent is not a READ");
      *(PortMap["port13"].second) = fromPort->getPacket();
    } else if (portId == 14) {
    // event from mem_la_write
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_la_write' PortEvent is not a READ");
      *(PortMap["port14"].second) = fromPort->getPacket();
    } else if (portId == 15) {
    // event from mem_la_wstrb
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_la_wstrb' PortEvent is not a READ");
      *(PortMap["port15"].second) = fromPort->getPacket();
    } else if (portId == 16) {
    // event from pcpi_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'pcpi_valid' PortEvent is not a READ");
      *(PortMap["port16"].second) = fromPort->getPacket();
    } else if (portId == 17) {
    // event from trace_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'trace_valid' PortEvent is not a READ");
      *(PortMap["port17"].second) = fromPort->getPacket();
    } else if (portId == 18) {
    // event from mem_addr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_addr' PortEvent is not a READ");
      *(PortMap["port18"].second) = fromPort->getPacket();
    } else if (portId == 19) {
    // event from mem_wdata
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_wdata' PortEvent is not a READ");
      *(PortMap["port19"].second) = fromPort->getPacket();
    } else if (portId == 20) {
    // event from mem_la_addr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_la_addr' PortEvent is not a READ");
      *(PortMap["port20"].second) = fromPort->getPacket();
    } else if (portId == 21) {
    // event from mem_la_wdata
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'mem_la_wdata' PortEvent is not a READ");
      *(PortMap["port21"].second) = fromPort->getPacket();
    } else if (portId == 22) {
    // event from pcpi_insn
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'pcpi_insn' PortEvent is not a READ");
      *(PortMap["port22"].second) = fromPort->getPacket();
    } else if (portId == 23) {
    // event from pcpi_rs1
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'pcpi_rs1' PortEvent is not a READ");
      *(PortMap["port23"].second) = fromPort->getPacket();
    } else if (portId == 24) {
    // event from pcpi_rs2
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'pcpi_rs2' PortEvent is not a READ");
      *(PortMap["port24"].second) = fromPort->getPacket();
    } else if (portId == 25) {
    // event from eoi
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'eoi' PortEvent is not a READ");
      *(PortMap["port25"].second) = fromPort->getPacket();
    } else if (portId == 26) {
    // event from trace_data
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::READ && "Error: 'trace_data' PortEvent is not a READ");
      *(PortMap["port26"].second) = fromPort->getPacket();
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
