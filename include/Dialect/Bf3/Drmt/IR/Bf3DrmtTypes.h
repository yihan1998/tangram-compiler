//===- Bf3DrmtTypes.h - BF3 dRMT dialect types -------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMTTYPES_H
#define BF3_BF3DRMTTYPES_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Interfaces/MemorySlotInterfaces.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOpsEnums.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypeInterfaces.h"

#define GET_TYPEDEF_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h.inc"

#endif // BF3_BF3DRMTTYPES_H
