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
include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.td"

//===----------------------------------------------------------------------===//
// Base P4HIR operation definition.
//===----------------------------------------------------------------------===//

class Bf3Drmt_Op<string mnemonic, list<Trait> traits = []> :
        Op<Bf3Drmt_Dialect, mnemonic, traits>;

def Bf3Drmt_ConstOp : Bf3Drmt_Op<"const",
    [ConstantLike, Pure, AllTypesMatch<["value", "res"]>,
     DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>]> {
    let summary = "Defines a P4 constant";
    let description = [{
        The `bf3drmt.const` operation turns a literal into an SSA value.
        The data is attached to the operation as an attribute. It
        represents a constant declaration.
    }];

    // The constant operation takes a constant value (via typed attributes) and
    // optional name.

    let arguments = (ins TypedAttrInterface:$value, OptionalAttr<StrAttr>:$name,
                         OptionalAttr<DictionaryAttr>:$annotations);

    let results = (outs Bf3Drmt_IntegerType:$res);

    let assemblyFormat = [{
        (` ` `[` $name^ `]`)? $value (`annotations` $annotations^)? attr-dict
    }];

    // Allow building a ConstantOp with no name, etc.
    let builders = [
        OpBuilder<(ins "mlir::TypedAttr":$value), [{
            build($_builder, $_state, value.getType(), value, {}, {});
        }]>,
        OpBuilder<(ins "mlir::TypedAttr":$value, "mlir::DictionaryAttr":$annotations), [{
            build($_builder, $_state, value.getType(), value, {}, annotations.empty() ? mlir::DictionaryAttr() : annotations);
        }]>
    ];

    let extraClassDeclaration = [{
        template<class T> T getValueAs() { return mlir::cast<T>(getValue()); }
    }];

    let hasVerifier = 1;
    let hasFolder = 1;
}

def VariableOp : Bf3Drmt_Op<"variable", [
                 DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>,
/*,
                 DeclareOpInterfaceMethods<PromotableAllocationOpInterface>,
                 DeclareOpInterfaceMethods<DestructurableAllocationOpInterface>*/]> {
  let summary = "Defines a scope-local variable";
  let description = [{
    The `p4hir.alloca` operation defines a scope-local variable.

    The presence `init` attribute indicates that the local variable represented
    by this alloca was originally initialized in P4 source code. In such
    cases, the first use contains the initialization (e.g. via p4hir.assign).

    The result type is a reference to the input's type.

    Example:

    ```mlir
    // bit<32> count = 3;
    %0 = p4hir.variable ["count", init] : <!p4hir.bit<32>>
    ...
    ```
  }];

  let arguments = (ins OptionalAttr<StrAttr>:$name, UnitAttr:$init,
                       OptionalAttr<DictionaryAttr>:$annotations);

  let results = (outs Res<Bf3Drmt_ReferenceType, ""/*,
                      [MemAlloc<AutomaticAllocationScopeResource>]*/>:$ref);

  let assemblyFormat = [{
    ` ` `[` $name
       (`,` `init` $init^)?
    `]`
    (`annotations` $annotations^)?
     attr-dict `:` type($ref)
  }];

  let builders = [
    // Allow building a VariableOp with no annotations
    OpBuilder<(ins "mlir::Type":$resultType, "const llvm::Twine &":$name), [{
          build($_builder, $_state,
                resultType, $_builder.getStringAttr(name), false, {});
        }]>,

    OpBuilder<(ins "mlir::Type":$resultType, "const llvm::Twine &":$name, "mlir::DictionaryAttr":$annotations), [{
          build($_builder, $_state,
                resultType, $_builder.getStringAttr(name), false, annotations.empty() ? mlir::DictionaryAttr() : annotations);
        }]>,

    OpBuilder<(ins "mlir::Type":$resultType, "const llvm::Twine &":$name, "bool":$init), [{
          build($_builder, $_state,
                resultType, $_builder.getStringAttr(name), init, {});
        }]>,

    OpBuilder<(ins "mlir::Type":$resultType, "const llvm::Twine &":$name, "bool":$init, "mlir::DictionaryAttr":$annotations), [{
          build($_builder, $_state,
                resultType, $_builder.getStringAttr(name), init, annotations.empty() ? mlir::DictionaryAttr() : annotations);
        }]>,

  ];

  let hasVerifier = 0;
  let hasCanonicalizeMethod = 1;
}

include "Dialect/Bf3/Drmt/IR/Bf3DrmtControlOps.td"

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

def Bf3Drmt_StructExtractRefOp : Bf3Drmt_Op<"struct_extract_ref", [Pure,
    DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>]> {

    let summary = "Project from a struct reference to a reference to a nested struct field";
    let description = [{
        Extracts a reference to a leaf field within a nested struct or header.

        Example:
        ```mlir
        %ttl = bf3drmt.struct_extract_ref %hdrs["ipv4", "ttl"]
        ```

        The type of %ttl is a reference to the leaf field type.
    }];

    let arguments = (ins Bf3Drmt_StructLikeRefType:$input, I32Attr:$fieldIndex);

    let results = (outs Bf3Drmt_ReferenceType:$result);

    let hasCustomAssemblyFormat = 1;
    let hasVerifier = 1;

    let builders = [
        OpBuilder<(ins "mlir::Value":$input, "bf3drmt::FieldInfo":$field)>,
        OpBuilder<(ins "mlir::Value":$input, "mlir::StringAttr":$fieldName)>,
        OpBuilder<(ins "mlir::Value":$input, "llvm::StringRef":$fieldName), [{
        build($_builder, $_state, input, $_builder.getStringAttr(fieldName));
        }]>
    ];

    let extraClassDeclaration = [{
        /// Return the name attribute of the accessed field.
        mlir::StringAttr getFieldNameAttr() {
            auto type = mlir::cast<Bf3Drmt_StructLikeTypeInterface>(mlir::cast<ReferenceType>(getInput().getType()).getObjectType());
            return type.getFields()[getFieldIndex()].name;
        }

        /// Return the name of the accessed field.
        llvm::StringRef getFieldName() {
            return getFieldNameAttr().getValue();
        }
    }];
}

def Bf3Drmt_ReadOp : Bf3Drmt_Op<"read", [
    TypesMatchWith<"type of 'result' matches object type of 'ref'",
                    "ref", "result",
                    "mlir::cast<bf3drmt::ReferenceType>($_self).getObjectType()">,
    DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>]> {

    let summary = "Read value from variable";
    let description = [{
        `p4hir.read` reads a value given a variable reference
        backed up by a `p4hir.ref` type.

        Example:

        ```mlir

        // Read from local variable, reference in %0.
        %1 = p4hir.read %0 : <!p4hir.bit<32>>

        ```
    }];

    let arguments = (ins Arg<Bf3Drmt_ReferenceType, "the reference to load from">:$ref);

    let results = (outs LoadableBf3DrmtType:$result);

    let assemblyFormat = [{
        $ref `:` type($ref) attr-dict
    }];
}

def Bf3Drmt_AssignOp : Bf3Drmt_Op<"assign", [
    TypesMatchWith<"type of 'value' matches object type of 'addr'",
                    "ref", "value",
                    "mlir::cast<bf3drmt::ReferenceType>($_self).getObjectType()">,/*,
                    DeclareOpInterfaceMethods<PromotableMemOpInterface>*/]> {

    let summary = "Assign value to variable";
    let description = [{
        `p4hir.assign` stores a value (first operand) to the object referenced
        in the second operand.

        Example:

        ```mlir
        // Store a function argument to local variable, address in %0.
        p4hir.assign %arg0, %0 : <!p4hir.bit<32>>
        ```
    }];

    let arguments = (ins LoadableBf3DrmtType:$value,
                        Arg<Bf3Drmt_ReferenceType, "the object to store the value">:$ref);

    let assemblyFormat = [{
        $value `,` $ref attr-dict `:` type($ref)
    }];
}

#endif // BF3_BF3DRMT_OPS
