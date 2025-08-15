//===- Bf3DrmtDialect.cpp - BF3 dRMT dialect ---------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h"

using namespace mlir;

//===----------------------------------------------------------------------===//
// BF3 dRMT dialect.
//===----------------------------------------------------------------------===//

void mlir::edamlir::bf3drmt::Bf3DrmtDialect::initialize() {
    std::cout << "Initializing BF3 dRMT Dialect..." << std::endl;
//     addOperations<
// #define GET_OP_LIST
// #include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.cpp.inc"
//         >();
    registerTypes();
    registerAttributes();
}
