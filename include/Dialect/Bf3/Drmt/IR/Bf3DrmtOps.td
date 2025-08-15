//===- Bf3DrmtOps.td - BF3 dRMT dialect ops -----------*- tablegen -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMT_OPS
#define BF3_BF3DRMT_OPS

include "mlir/IR/OpBase.td"
include "mlir/IR/OpAsmInterface.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/FunctionInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/LoopLikeInterface.td"
include "mlir/Interfaces/MemorySlotInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.td"
include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.td"
include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypeInterfaces.td"

//===----------------------------------------------------------------------===//
// Base P4HIR operation definition.
//===----------------------------------------------------------------------===//

class Bf3Drmt_Op<string mnemonic, list<Trait> traits = []> :
        Op<Bf3Drmt_Dialect, mnemonic, traits>;

def Bf3Drmt_FooOp : Bf3Drmt_Op<"foo", [Pure,
                                             SameOperandsAndResultType]> {
    let summary = "Illustrates how to define an operation.";
    let description = [{
        The `standalone.foo` operation illustrates how to define a new
        operation in a dialect. It uses an operation trait to declare that it
        has no side effects.

        This operation takes an integer argument and returns an integer.

        Example:

        ```mlir
        %0 = arith.constant 2 : i32
        // Apply the foo operation to %0
        %1 = standalone.foo %0 : i32
        ```
    }];

    let arguments = (ins I32:$input);
    let results = (outs I32:$res);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

#endif // BF3_BF3DRMT_OPS
