//===- Bf3DrmtDialect.cpp - BF3 dRMT dialect ---------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.h"

using namespace mlir;

//===----------------------------------------------------------------------===//
// BF3 dRMT dialect.
//===----------------------------------------------------------------------===//

void mlir::edamlir::bf3drmt::Bf3DrmtDialect::initialize() {
    registerTypes();
    registerAttributes();
    addOperations<
#define GET_OP_LIST
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.cpp.inc"
        >();
}
