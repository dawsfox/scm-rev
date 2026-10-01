#ifndef _SST_REV_SCRATCHPAD_H_
#define _SST_REV_SCRATCHPAD_H_

// -- C++ Headers
#include <vector>

// -- SST Headers
#include "SST.h"

// -- RevCPU Headers
#include "RevCommon.h"
#include "RevFlag.h"
#include "RevInstHelpers.h"
#include "RevCodelet.h"
#include "RevMem.h"

namespace SST::RevCPU {


// ----------------------------------------
// RevScratchpad
// ----------------------------------------
class RevScratchpad  {
public:

  // clang-format on

  /// RevScratchpad: constructor
  RevScratchpad();
  RevScratchpad(uint64_t baseAddr, uint64_t size);

  /// RevScratchpad: destructor
  ~RevScratchpad() {
    delete [] data;
  }

  /// RevScratchpad: disallow copying and assignment
  RevScratchpad( const RevScratchpad& )            = delete;
  RevScratchpad& operator=( const RevScratchpad& ) = delete;

  bool isValidAccess(uint64_t addr, int accessSize);
  bool contains(uint64_t addr);

  // on successful read, returns true and sets rdata to the requested data
  bool read64(uint64_t addr, uint64_t * rdata);
  bool read32(uint64_t addr, uint32_t * rdata);
  //bool read16(uint64_t addr, uint16_t * rdata);
  //bool read8(uint64_t addr, uint8_t * rdata);

  // on successful write, returns true
  bool write64(uint64_t addr, uint64_t wdata);
  bool write32(uint64_t addr, uint32_t wdata);
  //bool write16(uint64_t addr, uint16_t wdata);
  //bool write8(uint64_t addr, uint8_t wdata);

private:
  uint64_t baseAddr;
  uint64_t size; // number of bytes held; data is represented by 32-bit elements
  uint32_t * data;
  

  // -- private data members
  // Instead of a mem interface, should this connect to RevMem?

};  // RevScratchpad

}  // namespace SST::RevCPU

#endif  // _SST_REVCPU_SCRATCHPAD_H_
