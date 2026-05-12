//
// _verilatorSSTSubcomponent_cpp_
//
// Copyright (C) 2017-2024 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#include "verilatorSSTSubcomponent.h"
#include "Signal.h"

using namespace SST::VerilatorSST;

// ---------------------------------------------------------------
// VerilatorSSTBase
// ---------------------------------------------------------------
VerilatorSSTBase::VerilatorSSTBase(std::string DerivedName, ComponentId_t id,
                                   const Params &params)
    : SubComponent(id), output(nullptr) {
  verbosity = params.find<uint32_t>("verbose");
  std::string outStr = "[" + DerivedName + " @f:@l:time=@t]: ";
  output = new SST::Output(outStr, verbosity, 0, SST::Output::STDOUT);
}

VerilatorSSTBase::~VerilatorSSTBase() { delete output; }

// ---------------------------------------------------------------
// VerilatorSSTpicorv32
// ---------------------------------------------------------------
VerilatorSSTpicorv32::VerilatorSSTpicorv32(ComponentId_t id,
                                           const Params &params)
    : VerilatorSSTBase("picorv32", id, params), UseVPI(false) {

  UseVPI = params.find<bool>("useVPI", false);
  const std::string clockFreq = params.find<std::string>("clockFreq", "1GHz");

  clockPort = params.find<std::string>("clockPort", "NullPort");
  if (!isNamedPort(clockPort)) {
    output->fatal(CALL_INFO, -1, "Could not find clock port with name=%s\n",
                  clockPort.c_str());
  }

  // init verilator interfaces
  ContextP = new VerilatedContext();
  ContextP->threads(1);
  ContextP->debug(VL_DEBUG);
  ContextP->randReset(2);
  ContextP->traceEverOn(true);
  const char *empty{};
  ContextP->commandArgs(0, &empty);
  Top = new VTop(ContextP, "");
#if VL_DEBUG == 1
  ContextP->internalsDump();
#endif

  // attempt to build the reset value tables
  initResetValues(params);
  link_clk = configureLink("clk", "0ns",
                           new Event::Handler<VerilatorSSTpicorv32>(
                               this, &VerilatorSSTpicorv32::handle_clk));
  if (nullptr == link_clk) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_clk\n");
  }
  link_resetn = configureLink("resetn", "0ns",
                              new Event::Handler<VerilatorSSTpicorv32>(
                                  this, &VerilatorSSTpicorv32::handle_resetn));
  if (nullptr == link_resetn) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_resetn\n");
  }
  link_mem_ready =
      configureLink("mem_ready", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_ready));
  if (nullptr == link_mem_ready) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_ready\n");
  }
  link_pcpi_wr =
      configureLink("pcpi_wr", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_wr));
  if (nullptr == link_pcpi_wr) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_wr\n");
  }
  link_pcpi_wait =
      configureLink("pcpi_wait", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_wait));
  if (nullptr == link_pcpi_wait) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_wait\n");
  }
  link_pcpi_ready =
      configureLink("pcpi_ready", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_ready));
  if (nullptr == link_pcpi_ready) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_ready\n");
  }
  link_mem_rdata =
      configureLink("mem_rdata", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_rdata));
  if (nullptr == link_mem_rdata) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_rdata\n");
  }
  link_pcpi_rd =
      configureLink("pcpi_rd", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_rd));
  if (nullptr == link_pcpi_rd) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_rd\n");
  }
  link_irq = configureLink("irq", "0ns",
                           new Event::Handler<VerilatorSSTpicorv32>(
                               this, &VerilatorSSTpicorv32::handle_irq));
  if (nullptr == link_irq) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_irq\n");
  }
  link_trap = configureLink("trap", "0ns",
                            new Event::Handler<VerilatorSSTpicorv32>(
                                this, &VerilatorSSTpicorv32::handle_trap));
  if (nullptr == link_trap) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_trap\n");
  }
  link_mem_valid =
      configureLink("mem_valid", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_valid));
  if (nullptr == link_mem_valid) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_valid\n");
  }
  link_mem_instr =
      configureLink("mem_instr", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_instr));
  if (nullptr == link_mem_instr) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_instr\n");
  }
  link_mem_wstrb =
      configureLink("mem_wstrb", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_wstrb));
  if (nullptr == link_mem_wstrb) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_wstrb\n");
  }
  link_mem_la_read =
      configureLink("mem_la_read", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_la_read));
  if (nullptr == link_mem_la_read) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_la_read\n");
  }
  link_mem_la_write =
      configureLink("mem_la_write", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_la_write));
  if (nullptr == link_mem_la_write) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_la_write\n");
  }
  link_mem_la_wstrb =
      configureLink("mem_la_wstrb", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_la_wstrb));
  if (nullptr == link_mem_la_wstrb) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_la_wstrb\n");
  }
  link_pcpi_valid =
      configureLink("pcpi_valid", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_valid));
  if (nullptr == link_pcpi_valid) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_valid\n");
  }
  link_trace_valid =
      configureLink("trace_valid", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_trace_valid));
  if (nullptr == link_trace_valid) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_trace_valid\n");
  }
  link_mem_addr =
      configureLink("mem_addr", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_addr));
  if (nullptr == link_mem_addr) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_addr\n");
  }
  link_mem_wdata =
      configureLink("mem_wdata", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_wdata));
  if (nullptr == link_mem_wdata) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_wdata\n");
  }
  link_mem_la_addr =
      configureLink("mem_la_addr", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_la_addr));
  if (nullptr == link_mem_la_addr) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_la_addr\n");
  }
  link_mem_la_wdata =
      configureLink("mem_la_wdata", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_mem_la_wdata));
  if (nullptr == link_mem_la_wdata) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_mem_la_wdata\n");
  }
  link_pcpi_insn =
      configureLink("pcpi_insn", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_insn));
  if (nullptr == link_pcpi_insn) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_insn\n");
  }
  link_pcpi_rs1 =
      configureLink("pcpi_rs1", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_rs1));
  if (nullptr == link_pcpi_rs1) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_rs1\n");
  }
  link_pcpi_rs2 =
      configureLink("pcpi_rs2", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_pcpi_rs2));
  if (nullptr == link_pcpi_rs2) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_pcpi_rs2\n");
  }
  link_eoi = configureLink("eoi", "0ns",
                           new Event::Handler<VerilatorSSTpicorv32>(
                               this, &VerilatorSSTpicorv32::handle_eoi));
  if (nullptr == link_eoi) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_eoi\n");
  }
  link_trace_data =
      configureLink("trace_data", "0ns",
                    new Event::Handler<VerilatorSSTpicorv32>(
                        this, &VerilatorSSTpicorv32::handle_trace_data));
  if (nullptr == link_trace_data) {
    output->fatal(CALL_INFO, -1,
                  "Error: was unable to configureLink link_trace_data\n");
  }

  // register the clock
  registerClock(clockFreq, new Clock::Handler<VerilatorSSTpicorv32>(
                               this, &VerilatorSSTpicorv32::clock));

  // register statistics
  for (auto &portEntry : Ports) {
    if ((static_cast<uint8_t>(std::get<V_TYPE>(portEntry)) &
         static_cast<uint8_t>(VPortType::V_INPUT)) > 0) {
      std::get<V_WRITE_STAT>(portEntry) = registerStatistic<uint64_t>(
          "PortWrites", std::get<V_NAME>(portEntry));
    }

    if ((static_cast<uint8_t>(std::get<V_TYPE>(portEntry)) &
         static_cast<uint8_t>(VPortType::V_OUTPUT)) > 0) {
#if ENABLE_INOUT_HANDLING
      const std::string portName = std::get<V_NAME>(portEntry);
      unsigned isInoutEn = portName.find("__en") != std::string::npos;
      unsigned isInoutOut = portName.find("__out") != std::string::npos;
      if (isInoutEn || isInoutOut)
        continue;
      std::get<V_READ_STAT>(portEntry) =
          registerStatistic<uint64_t>("PortReads", portName);
#else
      std::get<V_READ_STAT>(portEntry) =
          registerStatistic<uint64_t>("PortReads", std::get<V_NAME>(portEntry));
#endif
    }
  }
}

VerilatorSSTpicorv32::~VerilatorSSTpicorv32() {
  delete Top; // ContextP will be handled by Top's deletion
}

void VerilatorSSTpicorv32::splitStr(const std::string &s, char c,
                                    std::vector<std::string> &v) {
  std::string::size_type i = 0;
  std::string::size_type j = s.find(c);
  v.clear();

  if ((j == std::string::npos) && (s.length() > 0)) {
    v.push_back(s);
    return;
  }

  while (j != std::string::npos) {
    v.push_back(s.substr(i, j - i));
    i = ++j;
    j = s.find(c, j);
    if (j == std::string::npos)
      v.push_back(s.substr(i, s.length()));
  }
}

void VerilatorSSTpicorv32::pollWriteQueue() {
  uint64_t currTick = getCurrentTick();
  for (auto it = WriteQueue.begin(); it != WriteQueue.end();) {
    auto ele = *it;
    if (ele.AtTick == currTick) {
      if (UseVPI) {
        writePortVPI(ele.PortName, ele.Packet);
      } else {
        unsigned Idx = PortMap.at(ele.PortName);
        DirectWriteFunc Func = std::get<V_WRITEFUNC>(Ports[Idx]);
        (*Func)(Top, ele.Packet);
      }
      it = WriteQueue.erase(it);
    } else {
      it++;
    }
  }
}

void VerilatorSSTpicorv32::initResetValues(const Params &params) {
  std::vector<std::string> optList;
  params.find_array("resetVals", optList);

  for (unsigned i = 0; i < optList.size(); i++) {
    std::vector<std::string> vstr;
    std::string s = optList[i];
    splitStr(s, ':', vstr);

    if (vstr.size() != 2) {
      output->fatal(CALL_INFO, -1,
                    "Error in reading reset value from parameter list:%s\n",
                    s.c_str());
    }

    if (!isNamedPort(vstr[0])) {
      output->fatal(CALL_INFO, -1,
                    "Error in reading reset value: %s is not a named port\n",
                    vstr[0].c_str());
    }

    long unsigned val = std::stoul(vstr[1]);
    ResetVals.push_back(std::make_pair(vstr[0], val));
  }
}

void VerilatorSSTpicorv32::init(unsigned int phase) {
  for (auto ele : ResetVals) {
    std::vector<uint8_t> d;
    // convert uint64 to byte vector
    output->verbose(CALL_INFO, 1, 0, "initializing port %s to %" PRIu64 "\n",
                    ele.first.data(), ele.second);
    for (int i = 0; i < 8; i++) {
      uint8_t tmp = (ele.second >> (i * 8)) & 255;
      d.push_back(tmp);
    }
    writePort(ele.first, d);
  }
}

void VerilatorSSTpicorv32::setup() {}

void VerilatorSSTpicorv32::finish() { Top->final(); }

bool VerilatorSSTpicorv32::clock(SST::Cycle_t cycle) {
  // do nothing; link interface in use
  return false;
}

bool VerilatorSSTpicorv32::isNamedPort(std::string PortName) {

  for (unsigned i = 0; i < Ports.size(); i++) {
    if (std::get<V_NAME>(Ports[i]) == PortName) {
      return true;
    }
  }

  return false;
}

unsigned VerilatorSSTpicorv32::getNumPorts() { return Ports.size(); }

const std::vector<std::string> VerilatorSSTpicorv32::getPortsNames() {
  std::vector<std::string> Names;

  for (unsigned i = 0; i < Ports.size(); i++) {
    Names.push_back(std::get<V_NAME>(Ports[i]));
  }

  return Names;
}

bool VerilatorSSTpicorv32::getPortType(
    std::string PortName, SST::VerilatorSST::VPortType &direction) {
  unsigned Idx = 0;
  try {
    Idx = PortMap.at(PortName);
  } catch (const std::out_of_range &oor) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
    return false;
  }

  direction = std::get<V_TYPE>(Ports[Idx]);
  return true;
}

bool VerilatorSSTpicorv32::getPortWidth(std::string PortName, unsigned &Width) {
  unsigned Idx = 0;
  try {
    Idx = PortMap.at(PortName);
  } catch (const std::out_of_range &oor) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
    return false;
  }

  Width = std::get<V_WIDTH>(Ports[Idx]);
  return true;
}

bool VerilatorSSTpicorv32::getPortDepth(std::string PortName, unsigned &Depth) {
  unsigned Idx = 0;
  try {
    Idx = PortMap.at(PortName);
  } catch (const std::out_of_range &oor) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
    return false;
  }

  Depth = std::get<V_DEPTH>(Ports[Idx]);
  return true;
}

bool VerilatorSSTpicorv32::getResetVal(std::string PortName, uint64_t &Val) {
  unsigned Idx = 0;
  try {
    Idx = PortMap.at(PortName);
  } catch (const std::out_of_range &oor) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
    return false;
  }

  for (unsigned i = 0; i < ResetVals.size(); i++) {
    if (ResetVals[i].first == PortName) {
      Val = ResetVals[i].second;
      return true;
    }
  }

  return false;
}

uint64_t VerilatorSSTpicorv32::getCurrentTick() { return ContextP->time(); }

std::vector<uint8_t> VerilatorSSTpicorv32::readPortVPI(std::string PortName) {
  vpiHandle vh1 = vpi_handle_by_name((PLI_BYTE8 *)PortName.data(), NULL);
  assert(vh1 && "vpi should return a handle (port not found)");

  auto vpiTypeVal = vpi_get(vpiType, vh1);
  auto vpiSizeVal = vpi_get(vpiSize, vh1);

  if (vpiTypeVal == vpiReg) {
    s_vpi_value val{vpiVectorVal};
    vpi_get_value(vh1, &val);

    auto signalFactory = SignalFactory(vpiSizeVal, 1);
    auto signalPtr = signalFactory(val);
    const std::vector<uint8_t> &d = signalPtr->getUIntVector(true);
    delete signalPtr;
    return d;
  }

  if (vpiTypeVal == vpiMemory) {
    vpiHandle iter = vpi_iterate(vpiMemoryWord, vh1);
    assert(iter);

    SignalFactory *signalFactory;
    Signal *signalPtr = nullptr;
    auto i = 0;
    while (auto rowHandle = vpi_scan(iter)) {
      if (i == 0) {
        const auto rowSizeBits = vpi_get(vpiSize, rowHandle);
        signalFactory = new SignalFactory(rowSizeBits, vpiSizeVal);
      }

      s_vpi_value row{SIGNAL_VPI_FORMAT};
      vpi_get_value(rowHandle, &row);
      signalPtr = (*signalFactory)(row);

      vpi_free_object(rowHandle);
      i++;
    }

    const std::vector<uint8_t> &d = signalPtr->getUIntVector(true);
    delete signalPtr;
    delete signalFactory;
    return d;
  }

  assert(false && "unsupported vpiType");
  std::vector<uint8_t> d;
  return d;
}

void VerilatorSSTpicorv32::writePortVPI(std::string PortName,
                                        const std::vector<uint8_t> &Packet) {
  vpiHandle vh1 = vpi_handle_by_name(PortName.data(), NULL);
  assert(vh1 && "vpi should return a handle (port not found)");

  auto vpiTypeVal = vpi_get(vpiType, vh1);
  auto vpiSizeVal = vpi_get(vpiSize, vh1);

  auto vpiDirVal = vpi_get(vpiDirection, vh1);
  assert(vpiDirVal == vpiInput && "port must be an input, inout not supported");
  unsigned Width;
  unsigned Depth;
  getPortWidth(PortName, Width);
  getPortDepth(PortName, Depth);
  Signal toWrite(Width, Depth, Packet, true);
  if (vpiTypeVal == vpiReg) {
    t_vpi_value val = toWrite.getVpiValue(0);
    vpi_put_value(vh1, &val, NULL, vpiNoDelay);
    return;
  }

  if (vpiTypeVal == vpiMemory) {
    assert(vpiSizeVal == Depth && "port depth must match signal depth");
    vpiHandle iter = vpi_iterate(vpiMemoryWord, vh1);
    assert(iter);

    int i = 0;
    while (auto rowHandle = vpi_scan(iter)) {
      auto rowSizeBits = vpi_get(vpiSize, rowHandle);
      assert(rowSizeBits == Width && "row width must match signal width");

      t_vpi_value val = toWrite.getVpiValue(i);
      vpi_put_value(rowHandle, &val, NULL, 0);

      vpi_free_object(rowHandle);
      i++;
    }

    return;
  }

  assert(false && "unsupported vpiType");
}

void VerilatorSSTpicorv32::writePort(std::string PortName,
                                     const std::vector<uint8_t> &Packet) {
  // sanity check
  if (!isNamedPort(PortName)) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
  }

// inout ports must be disabled before writing
#if ENABLE_INOUT_HANDLING
  VPortType portType;
  getPortType(PortName, portType);
  if (portType == VPortType::V_INOUT) {
    if (!ENABLE_INOUT_HANDLING) {
      output->fatal(
          CALL_INFO, -1,
          "inout port (%s) cannot be written, inout handling is disabled\n",
          PortName.c_str());
    }

    if (!verifyInoutEnabledIs(false, PortName)) {
      output->fatal(CALL_INFO, -1,
                    "inout port (%s) cannot be written, it is being driven by "
                    "the top module\n",
                    PortName.c_str());
    }
  }
#endif

  // update statistics
  unsigned idx = PortMap.at(PortName);
  std::get<V_WRITE_STAT>(Ports[idx])->incrementCollectionCount(1);

  // determine which write to use
  if (UseVPI) {
    writePortVPI(PortName, Packet);
    this->Top->eval();
  } else {
    unsigned Idx = PortMap.at(PortName);
    DirectWriteFunc Func = std::get<V_WRITEFUNC>(Ports[Idx]);
    (*Func)(Top, Packet);
  }
}

void VerilatorSSTpicorv32::writePortAtTick(std::string PortName,
                                           const std::vector<uint8_t> &Packet,
                                           uint64_t Tick) {
  // sanity check
  if (!isNamedPort(PortName)) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
  }

  // Tick is used as a delay/offset, not a definite tick value
  // VPI/Direct is decided when polling the WriteQueue
  WriteQueue.emplace_back(PortName, Tick + getCurrentTick(), Packet);
}

std::vector<uint8_t> VerilatorSSTpicorv32::readPort(std::string PortName) {

  // sanity check
  if (!isNamedPort(PortName)) {
    output->fatal(CALL_INFO, -1, "Could not find port with name=%s\n",
                  PortName.c_str());
  }

// inout ports must be enabled before reading
#if ENABLE_INOUT_HANDLING
  VPortType portType;
  getPortType(PortName, portType);
  if (portType == VPortType::V_INOUT) {
    if (!ENABLE_INOUT_HANDLING) {
      output->fatal(
          CALL_INFO, -1,
          "inout port (%s) cannot be read, inout handling is disabled\n",
          PortName.c_str());
    }

    if (!verifyInoutEnabledIs(true, PortName)) {
      output->fatal(CALL_INFO, -1,
                    "inout port (%s) cannot be read, it is not being driven by "
                    "the top module\n",
                    PortName.c_str());
    }

    // redirect to __out port
    const std::string outPortName = PortName + "__out";
    return readPort(outPortName);
  }
#endif

// update statistics
#if ENABLE_INOUT_HANDLING
  if (PortName.find("__en") == std::string::npos) {
    const size_t outSubstrIdx = PortName.find("__out");
    const std::string visiblePortName = outSubstrIdx != std::string::npos
                                            ? PortName.substr(0, outSubstrIdx)
                                            : PortName;
    const unsigned idx = PortMap.at(visiblePortName);
    std::get<V_READ_STAT>(Ports[idx])->incrementCollectionCount(1);
  }
#else
  const unsigned idx = PortMap.at(PortName);
  std::get<V_READ_STAT>(Ports[idx])->incrementCollectionCount(1);
#endif

  // determine which read to use
  if (UseVPI) {
    const std::vector<uint8_t> &data = readPortVPI(PortName);
    return data;
  } else {
    unsigned Idx = PortMap.at(PortName);
    DirectReadFunc Func = std::get<V_READFUNC>(Ports[Idx]);
    const std::vector<uint8_t> &data = (*Func)(Top);
    return data;
  }
}

bool VerilatorSSTpicorv32::verifyInoutEnabledIs(const bool isEnabled,
                                                const std::string portName) {
  const std::string enPortName = portName + "__en";
  const std::vector<uint8_t> enPortData = readPort(enPortName);
  uint32_t bitCnt;
  getPortWidth(enPortName, bitCnt);
  const uint32_t byteWidth = (bitCnt + 7) / 8;

  for (size_t i = 0; i < byteWidth; i++) {
    // all bits of inout port must be driven before valid read
    const uint8_t mask = bitCnt > 7 ? 255 : (1 << bitCnt) - 1;
    const bool allBitsAtLevel = isEnabled ? (enPortData[i] & mask) == mask
                                          : (enPortData[i] & mask) == 0;
    bitCnt -= 8;

    if (!allBitsAtLevel)
      return false;
  }

  return true;
}

void VerilatorSSTpicorv32::DirectWriteclk(VTop *T,
                                          const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->clk = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadclk(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->clk);
  return d;
}
void VerilatorSSTpicorv32::DirectWriteresetn(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->resetn = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadresetn(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->resetn);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_ready(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->mem_ready = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_ready(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_ready);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_wr(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->pcpi_wr = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_wr(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->pcpi_wr);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_wait(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->pcpi_wait = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_wait(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->pcpi_wait);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_ready(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 8 bit
  SignalHelper S(1);
  T->pcpi_ready = (Packet[0] & S.getMask<uint8_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_ready(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->pcpi_ready);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_rdata(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 32 bits
  SignalHelper S(32);
  uint32_t tmp = 0;
  tmp = (tmp << 8) + (((uint32_t)Packet[3]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[2]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[1]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[0]) & 255);
  T->mem_rdata = (tmp & S.getMask<uint32_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_rdata(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->mem_rdata >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->mem_rdata >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->mem_rdata >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->mem_rdata >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_rd(
    VTop *T, const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 32 bits
  SignalHelper S(32);
  uint32_t tmp = 0;
  tmp = (tmp << 8) + (((uint32_t)Packet[3]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[2]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[1]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[0]) & 255);
  T->pcpi_rd = (tmp & S.getMask<uint32_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_rd(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->pcpi_rd >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rd >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rd >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rd >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWriteirq(VTop *T,
                                          const std::vector<uint8_t> &Packet) {
  assert(Packet.size() > 0 && "received empty packet");
  // less than 32 bits
  SignalHelper S(32);
  uint32_t tmp = 0;
  tmp = (tmp << 8) + (((uint32_t)Packet[3]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[2]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[1]) & 255);
  tmp = (tmp << 8) + (((uint32_t)Packet[0]) & 255);
  T->irq = (tmp & S.getMask<uint32_t>());
  T->eval();
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadirq(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->irq >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->irq >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->irq >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->irq >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritetrap(VTop *T,
                                           const std::vector<uint8_t> &Packet) {
}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadtrap(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->trap);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_valid(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_valid(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_valid);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_instr(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_instr(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_instr);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_wstrb(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_wstrb(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_wstrb);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_la_read(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_la_read(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_la_read);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_la_write(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_la_write(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_la_write);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_la_wstrb(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_la_wstrb(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->mem_la_wstrb);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_valid(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_valid(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->pcpi_valid);
  return d;
}
void VerilatorSSTpicorv32::DirectWritetrace_valid(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadtrace_valid(VTop *T) {
  std::vector<uint8_t> d;
  // less than 8 bit
  d.push_back(T->trace_valid);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_addr(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_addr(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->mem_addr >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->mem_addr >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->mem_addr >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->mem_addr >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_wdata(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_wdata(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->mem_wdata >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->mem_wdata >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->mem_wdata >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->mem_wdata >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_la_addr(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_la_addr(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->mem_la_addr >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_addr >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_addr >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_addr >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritemem_la_wdata(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadmem_la_wdata(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->mem_la_wdata >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_wdata >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_wdata >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->mem_la_wdata >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_insn(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_insn(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->pcpi_insn >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_insn >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_insn >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_insn >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_rs1(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_rs1(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->pcpi_rs1 >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs1 >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs1 >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs1 >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritepcpi_rs2(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadpcpi_rs2(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->pcpi_rs2 >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs2 >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs2 >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->pcpi_rs2 >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWriteeoi(VTop *T,
                                          const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadeoi(VTop *T) {
  std::vector<uint8_t> d;
  // less than 32 bit
  uint8_t tmp = 0;
  tmp = (T->eoi >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->eoi >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->eoi >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->eoi >> 24) & 255;
  d.push_back(tmp);
  return d;
}
void VerilatorSSTpicorv32::DirectWritetrace_data(
    VTop *T, const std::vector<uint8_t> &Packet) {}
std::vector<uint8_t> VerilatorSSTpicorv32::DirectReadtrace_data(VTop *T) {
  std::vector<uint8_t> d;
  // less than 64 bit
  uint8_t tmp = 0;
  tmp = (T->trace_data >> 0) & 255;
  d.push_back(tmp);
  tmp = (T->trace_data >> 8) & 255;
  d.push_back(tmp);
  tmp = (T->trace_data >> 16) & 255;
  d.push_back(tmp);
  tmp = (T->trace_data >> 24) & 255;
  d.push_back(tmp);
  tmp = (T->trace_data >> 32) & 255;
  d.push_back(tmp);
  return d;
}

void VerilatorSSTpicorv32::handle_clk(SST::Event *ev) {
  // clock handler
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  pollWriteQueue();
  writePort("clk", portEvent->getPacket());
  ContextP->timeInc(1);
  delete portEvent;
}

void VerilatorSSTpicorv32::handle_resetn(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("resetn", portEvent->getPacket(), portEvent->getAtTick());
    } else {
      writePort("resetn", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("resetn");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_resetn->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. portName=resetn "
                "action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_ready(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("mem_ready", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("mem_ready", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_ready");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_ready->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_ready action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_wr(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("pcpi_wr", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("pcpi_wr", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_wr");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_wr->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_wr action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_wait(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("pcpi_wait", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("pcpi_wait", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_wait");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_wait->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_wait action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_ready(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("pcpi_ready", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("pcpi_ready", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_ready");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_ready->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_ready action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_rdata(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("mem_rdata", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("mem_rdata", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_rdata");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_rdata->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_rdata action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_rd(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("pcpi_rd", portEvent->getPacket(),
                      portEvent->getAtTick());
    } else {
      writePort("pcpi_rd", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_rd");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_rd->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_rd action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_irq(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);
  if (portEvent->getAction() == PortEventAction::WRITE) {
    if (portEvent->getAtTick() > 0) {
      writePortAtTick("irq", portEvent->getPacket(), portEvent->getAtTick());
    } else {
      writePort("irq", portEvent->getPacket());
    }
    delete portEvent;
    return;
  }

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("irq");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_irq->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(
      CALL_INFO, -1,
      "received port event with unrecognized action. portName=irq action=%u\n",
      static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_trap(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("trap");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_trap->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(
      CALL_INFO, -1,
      "received port event with unrecognized action. portName=trap action=%u\n",
      static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_valid(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_valid");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_valid->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_valid action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_instr(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_instr");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_instr->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_instr action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_wstrb(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_wstrb");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_wstrb->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_wstrb action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_la_read(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_la_read");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_la_read->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_la_read action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_la_write(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_la_write");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_la_write->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_la_write action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_la_wstrb(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_la_wstrb");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_la_wstrb->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_la_wstrb action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_valid(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_valid");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_valid->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_valid action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_trace_valid(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("trace_valid");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_trace_valid->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=trace_valid action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_addr(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_addr");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_addr->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_addr action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_wdata(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_wdata");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_wdata->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_wdata action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_la_addr(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_la_addr");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_la_addr->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_la_addr action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_mem_la_wdata(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("mem_la_wdata");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_mem_la_wdata->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=mem_la_wdata action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_insn(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_insn");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_insn->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_insn action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_rs1(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_rs1");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_rs1->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_rs1 action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_pcpi_rs2(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("pcpi_rs2");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_pcpi_rs2->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=pcpi_rs2 action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_eoi(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("eoi");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_eoi->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(
      CALL_INFO, -1,
      "received port event with unrecognized action. portName=eoi action=%u\n",
      static_cast<uint8_t>(portEvent->getAction()));
}

void VerilatorSSTpicorv32::handle_trace_data(SST::Event *ev) {
  const PortEvent *portEvent = static_cast<const PortEvent *>(ev);

  if (portEvent->getAction() == PortEventAction::READ) {
    const std::vector<uint8_t> packet = readPort("trace_data");
    PortEvent *respPortEvent = new PortEvent(packet);
    link_trace_data->send(respPortEvent);
    delete portEvent;
    return;
  }

  output->fatal(CALL_INFO, -1,
                "received port event with unrecognized action. "
                "portName=trace_data action=%u\n",
                static_cast<uint8_t>(portEvent->getAction()));
}

// EOF
