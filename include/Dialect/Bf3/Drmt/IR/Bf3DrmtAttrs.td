#ifndef BF3_BF3DRMT_ATTRS_TD
#define BF3_BF3DRMT_ATTRS_TD

include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/IR/EnumAttr.td"

include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.td"

class Bf3Drmt_Attr<string name, string attrMnemonic, list<Trait> traits = []>
    : AttrDef<Bf3Drmt_Dialect, name, traits> {
  let mnemonic = attrMnemonic;
}

//===----------------------------------------------------------------------===//
// IntAttr
//===----------------------------------------------------------------------===//

def Bf3Drmt_IntAttr : Bf3Drmt_Attr<"Int", "int", [TypedAttrInterface]> {
    let summary = "An Attribute containing a integer value";
    let description = [{
        An integer attribute is a literal attribute that represents an integral
        value of the specified integer type.
    }];
    let parameters = (ins AttributeSelfTypeParameter<"">:$type, "llvm::APInt":$value);
    let builders = [
        AttrBuilderWithInferredContext<(ins "mlir::Type":$type, "const llvm::APInt &":$value), [{
        return $_get(type.getContext(), type, value);
        }]>,
    ];
    let extraClassDeclaration = [{
        int64_t getSInt() const { return getValue().getSExtValue(); }
        uint64_t getUInt() const { return getValue().getZExtValue(); }
        bool isNullValue() const { return getValue() == 0; }
    }];

    let genVerifyDecl = 1;
    let hasCustomAssemblyFormat = 1;
    let genStorageClass = 0;
}


//===----------------------------------------------------------------------===//
// ValidAttr
//===----------------------------------------------------------------------===//

def ValidityBit_Invalid: I32BitEnumAttrCaseNone<"Invalid", "invalid">;
def ValidityBit_Valid  : I32BitEnumAttrCaseBit<"Valid", 0, "valid">;

def Bf3Drmt_ValidityBit : I32BitEnumAttr<
        "ValidityBit",
        "validity of a header",
        [ValidityBit_Invalid, ValidityBit_Valid]> {
    let cppNamespace = "::mlir::edamlir::bf3drmt";
    let genSpecializedAttr = 0;
}

def Bf3Drmt_ValidityBitAttr : EnumAttr<Bf3Drmt_Dialect, Bf3Drmt_ValidityBit, "validity.bit",
                               [TypedAttrInterface]> {
    let extraClassDeclaration = [{
        mlir::Type getType() { return bf3drmt::ValidBitType::get(getContext()); }
    }];
}

#endif  // BF3_BF3DRMT_ATTRS_TD