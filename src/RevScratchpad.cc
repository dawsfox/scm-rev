#include "RevScratchpad.h"

namespace SST::RevCPU {
RevScratchpad::RevScratchpad(uint64_t baseAddr, uint64_t size) 
  : baseAddr(baseAddr), size(size) {
  if (size % 4 != 0) {
    size += 4 - (size % 4);
  }
  data = new uint32_t[size / 4];
  // NOTE: initializing the instruction memory for pico here 
  // as hardcode, just a shortcut until loader is set up
  uint32_t firmware[6] = {0x3fc00093, 0x0000a023, 0x0000a103, 0x00110113, 0x0020a023, 0xff5ff06f};
  for (int i=0; i<6; i++) {
    data[i] = firmware[i];
  }
}


bool RevScratchpad::contains(uint64_t addr) {
  return (addr > baseAddr && addr < (baseAddr + size));
}

// accessSize is in number of bytes. Assume addr is a byte address
bool RevScratchpad::isValidAccess(uint64_t addr, int accessSize) {
  // only 1, 2, 4, 8 byte accesses are valid
  if (accessSize != 1 && accessSize != 2 && accessSize != 4 && accessSize != 8) {
    return false;
  }
  // accesses must be 4-byte aligned
  if (addr % 4 != 0) {
    return false;
  }
  uint64_t range_high = addr + static_cast<uint64_t>(accessSize); 
  if (addr < baseAddr || range_high > (baseAddr + size)) {
    return false;
  }
  return true;
}

bool RevScratchpad::read64(uint64_t addr, uint64_t * rdata) {
  if ( !isValidAccess(addr, 8) ) {
    return false;
  }
  // riscv is little endian, LSB is at the lower address
  uint64_t low_index = (addr - baseAddr) / 4;
  uint64_t high_index = (addr + 4 - baseAddr) / 4;
  *rdata = (static_cast<uint64_t>(data[high_index]) << 32) + data[low_index];
  return true;
}

bool RevScratchpad::read32(uint64_t addr, uint32_t * rdata) {
  if ( !isValidAccess(addr, 4) ) {
    return false;
  }
  uint64_t index = (addr - baseAddr) / 4;
  *rdata = data[index];
  return true;
}

/*
bool RevScratchpad::read16(uint64_t addr, uint16_t * rdata) {
  if ( !isValidAccess(addr, 2) ) {
    return false;
  }
  // riscv is little endian, LSB is at the lower address
  return true;
}
*/

bool RevScratchpad::write64(uint64_t addr, uint64_t wdata) {
  if ( !isValidAccess(addr, 8) ) {
    return false;
  }
  // riscv is little endian, LSB is at the lower address
  uint64_t low_index = (addr - baseAddr) / 4;
  uint64_t high_index = (addr + 4 - baseAddr) / 4;
  data[low_index] = static_cast<uint32_t>( wdata & 0x00000000FFFFFFFF );
  data[high_index] = static_cast<uint32_t>( wdata >> 32 );
  return true;
}

bool RevScratchpad::write32(uint64_t addr, uint32_t wdata) {
  if ( !isValidAccess(addr, 4) ) {
    return false;
  }
  uint64_t index = (addr - baseAddr) / 4;
  data[index] = wdata;
  return true;
}

} // namespace SST::RevCPU
