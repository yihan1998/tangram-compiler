# C Code Generator for BF3DRMT MLIR Dialect

This repository contains a complete implementation of a C code generator for the BF3DRMT MLIR dialect, designed for translating optimized IR to efficient C source code.

## Features Implemented

✅ **MLIR Dialect Infrastructure**
- BF3DRMT dialect with TableGen definitions
- Core operations: constant, addi, subi, shli, shrui
- Proper MLIR pass integration

✅ **C Code Generator**
- BF3DRMTToCCodegenPass for IR to C translation
- CSourceEmitter for formatted C output
- Type-safe C code generation (i32 → uint32_t)

✅ **Example Generation**
- Input: BF3DRMT MLIR operations
- Output: Compilable, optimized C code
- Verified working examples included

## Quick Start

### Build the Project
```bash
mkdir build && cd build
cmake .. -G Ninja -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
ninja
```

### Test Generated Code
```bash
cd examples
gcc generated_example.c -o test_program
./test_program
```

## Example Translation

**Input MLIR:**
```mlir
func.func @multiply_shift_hash_32_const(%input: i32) -> i32 {
  %c41 = bf3drmt.constant (41) : i32
  %c24 = bf3drmt.constant (24) : i32
  %product = bf3drmt.addi %input, %c41 : i32, i32 -> i32
  %result = bf3drmt.shrui %product, %c24 : i32, i32 -> i32
  func.return %result : i32
}
```

**Generated C Code:**
```c
#include <stdint.h>
#include <stdlib.h>

uint32_t multiply_shift_hash_32_const(uint32_t arg0) {
    uint32_t temp0 = 41;
    uint32_t temp1 = 24;
    uint32_t temp2 = arg0 + temp0;
    uint32_t temp3 = temp2 >> temp1;
    return temp3;
}
```

## Architecture

- **Dialect Layer**: BF3DRMT operations defined in TableGen
- **Pass Layer**: Translation pass using MLIR visitor pattern
- **Emission Layer**: C source formatting and type management
- **Integration**: Command-line tools and library interfaces

## Status

The core functionality is complete and working:
- Libraries compile successfully
- Code generation produces correct C output  
- Generated code compiles and executes properly
- Pass infrastructure is integrated with MLIR

Ready for integration into larger MLIR compilation pipelines!