#ifndef BF3DRMT_OPS
#define BF3DRMT_OPS

include "tangram/Dialect/BF3DRMT/BF3DRMTDialect.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

//===----------------------------------------------------------------------===//
// BF3DRMT Operations
//===----------------------------------------------------------------------===//

def BF3DRMT_ConstantOp : BF3DRMT_Op<"constant", [Pure, ConstantLike]> {
  let summary = "constant";
  let description = [{
    Constant operation represents a constant value.
  }];

  let arguments = (ins AnyAttr:$value);
  let results = (outs AnyType:$result);

  let assemblyFormat = "`(` $value `)` attr-dict `:` type($result)";
  
  let hasFolder = 1;
}

def BF3DRMT_AddIOp : BF3DRMT_Op<"addi", [Pure, Commutative]> {
  let summary = "integer addition operation";
  let description = [{
    The "addi" operation performs element-wise addition of two values.
  }];

  let arguments = (ins AnySignlessInteger:$lhs, AnySignlessInteger:$rhs);
  let results = (outs AnySignlessInteger:$result);
  
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($result)";
  
  let hasFolder = 1;
}

def BF3DRMT_SubIOp : BF3DRMT_Op<"subi", [Pure]> {
  let summary = "integer subtraction operation";
  let description = [{
    The "subi" operation performs element-wise subtraction of two values.
  }];

  let arguments = (ins AnySignlessInteger:$lhs, AnySignlessInteger:$rhs);
  let results = (outs AnySignlessInteger:$result);
  
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($result)";
  
  let hasFolder = 1;
}

def BF3DRMT_ShLIOp : BF3DRMT_Op<"shli", [Pure]> {
  let summary = "shift left operation";
  let description = [{
    The "shli" operation shifts the left operand left by the amount
    specified in the right operand.
  }];

  let arguments = (ins AnySignlessInteger:$lhs, AnySignlessInteger:$rhs);
  let results = (outs AnySignlessInteger:$result);
  
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($result)";
  
  let hasFolder = 1;
}

def BF3DRMT_ShrUIOp : BF3DRMT_Op<"shrui", [Pure]> {
  let summary = "unsigned shift right operation";
  let description = [{
    The "shrui" operation shifts the left operand right by the amount
    specified in the right operand. Zero fill.
  }];

  let arguments = (ins AnySignlessInteger:$lhs, AnySignlessInteger:$rhs);
  let results = (outs AnySignlessInteger:$result);
  
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($result)";
  
  let hasFolder = 1;
}

#endif // BF3DRMT_OPS