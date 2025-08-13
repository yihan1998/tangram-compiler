#ifndef TANGRAM_DIALECT_BF3DRMT_BF3DRMTOPS_H
#define TANGRAM_DIALECT_BF3DRMT_BF3DRMTOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"

#define GET_OP_CLASSES
#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.h.inc"

#endif // TANGRAM_DIALECT_BF3DRMT_BF3DRMTOPS_H