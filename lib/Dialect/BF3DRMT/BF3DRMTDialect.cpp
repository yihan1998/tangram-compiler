#include "tangram/Dialect/BF3DRMT/BF3DRMTDialect.h"
#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.h"

using namespace mlir;
using namespace mlir::bf3drmt;

#include "tangram/Dialect/BF3DRMT/BF3DRMTOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// BF3DRMT dialect.
//===----------------------------------------------------------------------===//

void BF3DRMTDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.cpp.inc"
  >();
}