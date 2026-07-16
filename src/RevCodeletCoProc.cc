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
  : RevCoProc(id, params, parent), num_instRetired(0), parent(parent) {

  std::string ClockFreq = params.find<std::string>("clock", "1Ghz");
  cycleCount = 0;
  cycleLimit = params.find<uint64_t>("cycleLimit", 500U);

  registerStats();

  ClockPort     = params.find<std::string>("clockPort", "clock");
  ResetLowPort  = params.find<std::string>("resetLowPort", "resetn");
  ResetHighPort = params.find<std::string>("resetHighPort", "NONE");

  const int NumPorts = params.find<int>( "num_ports", 0 );
  InfoVec.resize( static_cast<size_t>(NumPorts) );
  CuLocalMem.resize(7);
  // Test bench instructions, last slot is just for the single location 
  // that receives non-instruction requests
  CuLocalMem = {0x3fc00093, 0x0000a023, 0x0000a103, 0x00110113, 0x0020a023, 0xff5ff06f, 0};

  output->verbose( CALL_INFO, 1, 0, "Constructing Codelet coprocessor...\n" );
  printf("constructing Codelet coproc\n");
  fflush(stdout);
  
  ModelResetting = false;
  BeenReset = false;
  ResetLength = 8;
  ResetCounter = 0;

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
  output->verbose( CALL_INFO, 1, 0, "Initializing PortMap\n" );
  printf("init portmap\n");
  fflush(stdout);
  // get port information from params list
  params.find_array( "portMap", optList );
  for( size_t i=0; i<optList.size(); i++ ){
    output->verbose( CALL_INFO, 1, 0, "Port map entry: %s\n", optList[i].c_str() );
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
    printf("port state allocated for port %s\n", vstr[0].c_str());
    fflush(stdout);
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
  printf("InitLinkConfig done\n");
  fflush(stdout);
}

void RevCodeletCoProc::RecvPortEvent( SST::Event* ev, unsigned portId ) {
  output->verbose( CALL_INFO, 2, 0, "Received an event\n");
  SST::VerilatorSST::PortEvent* fromPort = static_cast<SST::VerilatorSST::PortEvent*>( ev );
  printf("received event on portId %u\n", portId);
  fflush(stdout);
  if(fromPort) {
    // Writes don't receive responses, and unless we have an inout port, 
    // we won't need to read any write port, so only implement read data receiving
    if(portId < 9) {
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
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'trap' PortEvent is not a WRITE");
      *(PortMap["trap"].second) = fromPort->getPacket();
    } else if (portId == 10) {
    // event from mem_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'memvalid' PortEvent is not a WRITE");
      *(PortMap["mem_valid"].second) = fromPort->getPacket();
    } else if (portId == 11) {
    // event from mem_instr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_instr' PortEvent is not a WRITE");
      *(PortMap["mem_instr"].second) = fromPort->getPacket();
    } else if (portId == 12) {
    // event from mem_wstrb
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_wstrb' PortEvent is not a WRITE");
      *(PortMap["mem_wstrb"].second) = fromPort->getPacket();
    } else if (portId == 13) {
    // event from mem_la_read
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_la_read' PortEvent is not a WRITE");
      *(PortMap["mem_la_read"].second) = fromPort->getPacket();
    } else if (portId == 14) {
    // event from mem_la_write
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_la_write' PortEvent is not a WRITE");
      *(PortMap["mem_la_write"].second) = fromPort->getPacket();
    } else if (portId == 15) {
    // event from mem_la_wstrb
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_la_wstrb' PortEvent is not a WRITE");
      *(PortMap["mem_la_wstrb"].second) = fromPort->getPacket();
    } else if (portId == 16) {
    // event from pcpi_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'pcpi_valid' PortEvent is not a WRITE");
      *(PortMap["pcpi_valid"].second) = fromPort->getPacket();
    } else if (portId == 17) {
    // event from trace_valid
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'trace_valid' PortEvent is not a WRITE");
      *(PortMap["trace_valid"].second) = fromPort->getPacket();
    } else if (portId == 18) {
    // event from mem_addr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_addr' PortEvent is not a WRITE");
      *(PortMap["mem_addr"].second) = fromPort->getPacket();
      printf("updating mem_addr port state: %p, size: %lu\n", PortMap["mem_addr"].second, PortMap["mem_addr"].second->size()); fflush(stdout);
    } else if (portId == 19) {
    // event from mem_wdata
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_wdata' PortEvent is not a WRITE");
      *(PortMap["mem_wdata"].second) = fromPort->getPacket();
    } else if (portId == 20) {
    // event from mem_la_addr
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_la_addr' PortEvent is not a WRITE");
      *(PortMap["mem_la_addr"].second) = fromPort->getPacket();
    } else if (portId == 21) {
    // event from mem_la_wdata
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'mem_la_wdata' PortEvent is not a WRITE");
      *(PortMap["mem_la_wdata"].second) = fromPort->getPacket();
    } else if (portId == 22) {
    // event from pcpi_insn
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'pcpi_insn' PortEvent is not a WRITE");
      *(PortMap["pcpi_insn"].second) = fromPort->getPacket();
    } else if (portId == 23) {
    // event from pcpi_rs1
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'pcpi_rs1' PortEvent is not a WRITE");
      *(PortMap["pcpi_rs1"].second) = fromPort->getPacket();
    } else if (portId == 24) {
    // event from pcpi_rs2
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'pcpi_rs2' PortEvent is not a WRITE");
      *(PortMap["pcpi_rs2"].second) = fromPort->getPacket();
    } else if (portId == 25) {
    // event from eoi
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'eoi' PortEvent is not a WRITE");
      printf("event from eoi port\n");
      printf("address of eoi port map data vector: %p\n", PortMap["port25"].second);
      fflush(stdout);
      *(PortMap["eoi"].second) = fromPort->getPacket();
    } else if (portId == 26) {
    // event from trace_data
      assert(fromPort->getAction() == VerilatorSST::PortEventAction::WRITE && "Error: 'trace_data' PortEvent is not a WRITE");
      *(PortMap["trace_data"].second) = fromPort->getPacket();
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

bool RevCodeletCoProc::ResetModel() {
  bool result;
  if (ResetHighPort == "NONE" && ResetLowPort != "NONE") {
    output->verbose( CALL_INFO, 1, 0, "Using ResetLowPort: %s\n", ResetLowPort.c_str() );
    ActiveResetPort = ResetLowPort;
    ActiveResetValue = 0;
  } else if (ResetLowPort == "NONE") {
    output->verbose( CALL_INFO, 1, 0, "Using ResetHighPort: %s\n", ResetHighPort.c_str() );
    ActiveResetPort = ResetHighPort; 
    ActiveResetValue = 1;
  } else {
    return false;
  }
  // this is also probably a good place to set stable inputs 
  // for startup. We can hardcode in the meantime, but later 
  // stable reset inputs should be passed as a parameter
  result  = writeToPort<uint8_t>("mem_ready", 0);
  result &= writeToPort<uint8_t>("pcpi_wr", 0);
  result &= writeToPort<uint8_t>("pcpi_wait", 0);
  result &= writeToPort<uint8_t>("pcpi_ready", 0);
  result &= writeToPort<uint8_t>("mem_rdata", 0);
  result &= writeToPort<uint8_t>("pcpi_rd", 0);
  result &= writeToPort<uint8_t>("irq", 0);
  result &= writeToPort<uint8_t>(ActiveResetPort, ActiveResetValue);
  //result &= writeToPort<uint8_t>(ClockPort, 1);
  //result &= writeToPort<uint8_t>(ClockPort, 0);
  //result &= writeToPort<uint8_t>(ClockPort, 1);
  // Since we call it in the clockTick function, let's rely on that 
  // to flip the clock for a couple cycles instead
  output->verbose( CALL_INFO, 1, 0, "Verilated model resetting . . .\n" );
  return result;
}

uint32_t RevCodeletCoProc::fourByteConverter( const std::vector<uint8_t>& eventData ) {
  uint32_t tmp = eventData.at(0);
  printf("in fourbyte converter\n"); fflush(stdout);
  tmp +=  (static_cast<uint32_t>(eventData.at(1)) << 8);
  tmp +=  (static_cast<uint32_t>(eventData.at(2)) << 16);
  tmp +=  (static_cast<uint32_t>(eventData.at(3)) << 24);
  return tmp;
}

// Checks if port values indicate the CU is requesting an instruction
bool RevCodeletCoProc::CheckInstRqst(){
  uint8_t  curr_mem_valid = (*(PortMap["mem_valid"].second))[0];
  uint8_t  curr_mem_instr = (*(PortMap["mem_instr"].second))[0];
  uint8_t  curr_mem_wstrb = (*(PortMap["mem_wstrb"].second))[0];
  // mem_wstrb should be 0 on a read, which instruction fetch is
  return curr_mem_valid && curr_mem_instr && !curr_mem_wstrb;
}

// Serves Cu instruction request
bool RevCodeletCoProc::ServeInstRqst(){
  output->verbose( CALL_INFO, 1, 0, "Attempting to serve pico instruction read\n");
  // divide by 4 because the addresses are byte addresses
  uint32_t curr_mem_addr  = fourByteConverter(*(PortMap[ "mem_addr"].second)) / 4;
  if (curr_mem_addr > 3) {
    // bounds checking
    return false;
  }
  uint32_t inst = CuLocalMem[curr_mem_addr];
  bool result = writeToPort<uint32_t>("mem_rdata", inst);
  return result && writeToPort<uint8_t>("mem_ready", 1);
}

bool RevCodeletCoProc::CheckDataRead() {
  uint8_t  curr_mem_valid = (*(PortMap["mem_valid"].second))[0];
  uint8_t  curr_mem_instr = (*(PortMap["mem_instr"].second))[0];
  uint8_t  curr_mem_wstrb = (*(PortMap["mem_wstrb"].second))[0];
  return curr_mem_valid && !curr_mem_instr && !curr_mem_wstrb;
}

bool RevCodeletCoProc::ServeDataRead() {
  output->verbose( CALL_INFO, 1, 0, "Attempting to serve pico data read\n");
  uint32_t curr_mem_addr  = fourByteConverter((*PortMap[ "mem_addr"].second)) / 4;
  // TODO: do we implement this with a fake local data memory, or is it worth it 
  // to hook up to call to RevMem here?
  // Problem: don't have RevMem reference: leave it for later. Basic testbench
  // only accesses address 0x3fc for data, 0x0-0x14 for instructions
  if (curr_mem_addr == 255) {
    bool result = writeToPort<uint32_t>("mem_rdata", CuLocalMem[6]);
    // TODO: with setting mem_ready here, we need to probably make sure to lower it 
    // in the clock tick function if valid went low
    return result && writeToPort<uint8_t>("mem_ready", 1);
  }   
  return false;
}

bool RevCodeletCoProc::CheckDataWrite() {
  uint8_t  curr_mem_valid = (*(PortMap["mem_valid"].second))[0];
  uint8_t  curr_mem_instr = (*(PortMap["mem_instr"].second))[0];
  uint8_t  curr_mem_wstrb = (*(PortMap["mem_wstrb"].second))[0];
  return curr_mem_valid && !curr_mem_instr && curr_mem_wstrb;
}

bool RevCodeletCoProc::ServeDataWrite() {
  // no RevMem pointer -- "write" it somewhere local
  output->verbose( CALL_INFO, 1, 0, "Attempting to serve pico data write\n");
  uint32_t curr_mem_addr  = fourByteConverter((*PortMap[ "mem_addr"].second)) / 4;
  if (curr_mem_addr == 255) {
    // TODO: with setting mem_ready here, we need to probably make sure to lower it 
    // in the clock tick function if valid went low
    CuLocalMem[7] = fourByteConverter((*PortMap["mem_wdata"].second));
    return writeToPort<uint8_t>("mem_ready", 1);
  }   
  return false;
}

// TODO: this won't work with VLWIDE ports (bigger than 64 bit, and maybe 63 bit...) yet
template<typename T>
bool RevCodeletCoProc::writeToPort(std::string portName, T data) {
  PortDef writePortDef = PortMap[portName].first;
  if (!writePortDef.Write || (sizeof(T) < writePortDef.Size)) {
    // there may be a couple sizes here that still incorrectly 
    // write instead of returning false. Needs testing
    return false;
  }
  std::vector<uint8_t> packetData;
  for (unsigned i=0; i<writePortDef.Size; i++) {
     packetData.push_back( (data >> (i*8)) & 255 );
  }
  // NOTE: Update port state here? Does it matter
  if (PortMap[portName].second != nullptr) {
    *(PortMap[portName].second) = packetData;
    VerilatorSST::PortEvent * toSend = new VerilatorSST::PortEvent(packetData);
    Links[writePortDef.PortId]->send(toSend);
    return true;
  } else {
    printf("WARNING: %s has nullptr port data!\n", portName.c_str());
    fflush(stdout);
    return false;
  }
}

// send read requests to all readable ports; the handler will update 
// the PortStates with the returned values
void RevCodeletCoProc::UpdatePortState() {
  output->verbose( CALL_INFO, 1, 0, "Attempting to update port state array\n" );
  for ( const auto &portMapEntry : PortMap ) {
    PortState currState = portMapEntry.second; // port state and def for this port
    PortDef thisDef = currState.first; // port def info (is it readable?)
    if ( thisDef.Read ) {
      VerilatorSST::PortEvent * toSend = new VerilatorSST::PortEvent();
      Links[thisDef.PortId]->send(toSend);
    }
  }
}

bool RevCodeletCoProc::ClockTick(SST::Cycle_t cycle){
  output->verbose( CALL_INFO, 1, 0, "Codelet coprocessor cycle %llu (cycle limit: %llu) ...\n", cycle, cycleLimit );
  Done = cycle > cycleLimit;
  if (!Done) {
    printf("ClockTick checkpoint 1!\n"); fflush(stdout);
    bool error = false;
    // This has to go before reset is actually triggered, so it doesn't immediately
    // skip a cycle of the reset counter after reset is triggered
    printf("BeenReset: %s; ModelResetting: %s\n", BeenReset ? "true" : "false", ModelResetting ? "true" : "false"); fflush(stdout);
    if ( ModelResetting  ) {
      if ( ResetCounter < ResetLength ) {
        // means model is still resetting, and should continue
        ResetCounter++;
      } else {
        printf("Finishing resetting!\n"); fflush(stdout);
        // done resetting!
        if ( ActiveResetValue ){
          error |= !writeToPort<uint8_t>(ActiveResetPort, 0);
        } else {
          error |= !writeToPort<uint8_t>(ActiveResetPort, 1);
        }
        BeenReset = true;
        ModelResetting = false;
      }
    }
    printf("BeenReset: %s; ModelResetting: %s\n", BeenReset ? "true" : "false", ModelResetting ? "true" : "false"); fflush(stdout);
    if (!BeenReset && !ModelResetting) {
      error |= ResetModel();
      ModelResetting = true;
      printf("BeenReset: %s; ModelResetting: %s\n", BeenReset ? "true" : "false", ModelResetting ? "true" : "false"); fflush(stdout);
    }
    printf("ClockTick checkpoint 2!\n"); fflush(stdout);
    printf("ClockTick checkpoint 3!\n"); fflush(stdout);
    if (error) {
      output->fatal(CALL_INFO, -1,
                    "Error in ClockTick after Reset at cycle %llu\n",
                    cycle );
    }
    //output->verbose( CALL_INFO, 1, VerilatorSST::VerboseMasking::INIT, "Port map entry: %s\n", optList[i].c_str() );
    error |= !writeToPort<uint8_t>(ClockPort, 0);
    error |= !writeToPort<uint8_t>(ClockPort, 1);
    if (error) {
      output->fatal(CALL_INFO, -1,
                    "Error in ClockTick after clock cycle at cycle %llu\n",
                    cycle );
    }
    printf("ClockTick checkpoint 4!\n"); fflush(stdout);
    if (CheckInstRqst()) {
      error |= !ServeInstRqst();
    } else if (CheckDataRead()) {
      error |= !ServeDataRead();
    } else if (CheckDataWrite()){
      error |= !ServeDataWrite();
    }
    if (error) {
      output->fatal(CALL_INFO, -1,
                    "Error in ClockTick after inst check at cycle %llu\n",
                    cycle );
    }
    
    UpdatePortState();
    uint8_t  curr_mem_valid = (*(PortMap["mem_valid"].second))[0];
    // if valid is low, there is no transaction (or a prior transaction has finished) so 
    // we need to lower mem_ready 
    if (curr_mem_valid == 0) {
      error |= !writeToPort<uint8_t>("mem_ready", 0);
    }
    printf("ClockTick checkpoint 5!\n"); fflush(stdout);

    if (error) {
      output->fatal(CALL_INFO, -1,
                    "Error in ClockTick at cycle %llu\n",
                    cycle );
    }

  } else {
    parent->SetCodeletDone();
  }
  return true;
}

}  // namespace SST:RevCPU
