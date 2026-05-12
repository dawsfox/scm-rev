#!/bin/bash

#Build the test
make

# Check that the exec was built...
if [[ -x verilator_sst.exe ]]; then
  sst-info revcpu
  # Include verilator sst paths: hardcoded for now
  sst --add-lib-path=../../build/src/ --add-lib-path=~/verilator-sst/install/ --output-dot=SCM_REV.dot ./rev-test-verilator_sst.py
else
  echo "Test VERILATOR_SST: File verilator_sst.exe not found - likely build failed"
  exit 1
fi
