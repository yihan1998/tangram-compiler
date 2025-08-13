#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.h"
#include "tangram/Dialect/BF3DRMT/BF3DRMTDialect.h"
#include "mlir/IR/OpImplementation.h"

using namespace mlir;
using namespace mlir::bf3drmt;

#define GET_OP_CLASSES
#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.cpp.inc"

//===----------------------------------------------------------------------===//
// BF3DRMT Operations
//===----------------------------------------------------------------------===//

OpFoldResult ConstantOp::fold(FoldAdaptor) {
  return getValueAttr();
}

OpFoldResult AddIOp::fold(FoldAdaptor adaptor) {
  // Constant folding for integer addition
  if (auto lhsAttr = adaptor.getLhs().dyn_cast_or_null<IntegerAttr>()) {
    if (auto rhsAttr = adaptor.getRhs().dyn_cast_or_null<IntegerAttr>()) {
      auto lhsValue = lhsAttr.getValue();
      auto rhsValue = rhsAttr.getValue();
      return IntegerAttr::get(getResult().getType(), lhsValue + rhsValue);
    }
  }
  return {};
}

OpFoldResult SubIOp::fold(FoldAdaptor adaptor) {
  // Constant folding for integer subtraction
  if (auto lhsAttr = adaptor.getLhs().dyn_cast_or_null<IntegerAttr>()) {
    if (auto rhsAttr = adaptor.getRhs().dyn_cast_or_null<IntegerAttr>()) {
      auto lhsValue = lhsAttr.getValue();
      auto rhsValue = rhsAttr.getValue();
      return IntegerAttr::get(getResult().getType(), lhsValue - rhsValue);
    }
  }
  return {};
}

OpFoldResult ShLIOp::fold(FoldAdaptor adaptor) {
  // Constant folding for shift left
  if (auto lhsAttr = adaptor.getLhs().dyn_cast_or_null<IntegerAttr>()) {
    if (auto rhsAttr = adaptor.getRhs().dyn_cast_or_null<IntegerAttr>()) {
      auto lhsValue = lhsAttr.getValue();
      auto rhsValue = rhsAttr.getValue();
      return IntegerAttr::get(getResult().getType(), lhsValue << rhsValue.getZExtValue());
    }
  }
  return {};
}

OpFoldResult ShrUIOp::fold(FoldAdaptor adaptor) {
  // Constant folding for unsigned shift right
  if (auto lhsAttr = adaptor.getLhs().dyn_cast_or_null<IntegerAttr>()) {
    if (auto rhsAttr = adaptor.getRhs().dyn_cast_or_null<IntegerAttr>()) {
      auto lhsValue = lhsAttr.getValue();
      auto rhsValue = rhsAttr.getValue();
      return IntegerAttr::get(getResult().getType(), lhsValue.lshr(rhsValue.getZExtValue()));
    }
  }
  return {};
}