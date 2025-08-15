//===- Bf3DrmtOps.h - BF3 dRMT dialect ops -----------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMTOPS_H
#define BF3_BF3DRMTOPS_H

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/MemorySlotInterfaces.h"
#include "mlir/Interfaces/LoopLikeInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOpsEnums.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

#define GET_OP_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.h.inc"

#endif // BF3_BF3DRMTOPS_H
