//===- Bf3DrmtTypes.td - BF3 dRMT dialect types -------*- tablegen -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMT_TYPES
#define BF3_BF3DRMT_TYPES

include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/EnumAttr.td"
include "mlir/Interfaces/MemorySlotInterfaces.td"

include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.td"
include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypeInterfaces.td"

//===----------------------------------------------------------------------===//
// Bf3 dRMT type definitions
//===----------------------------------------------------------------------===//

class Bf3Drmt_Type<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<Bf3Drmt_Dialect, name, traits> {
  let mnemonic = typeMnemonic;
}

class Bf3Drmt_TypeNoMnemonic<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<Bf3Drmt_Dialect, name, traits> {
    string typeName = dialect.name # "." # typeMnemonic;
}

//===----------------------------------------------------------------------===//
// IntegerType
//===----------------------------------------------------------------------===//

def Bf3Drmt_IntegerType : Bf3Drmt_Type<"Integer", "int"> {
    let summary = "BF3 dRMT Integer type";
    let description = "Custom type in BF3 dRMT dialect";
    let parameters = (ins "unsigned":$width, "bool":$isSigned);
    let hasCustomAssemblyFormat = 1;

    let extraClassDeclaration = [{
        /// Return true if this is a signed integer type.
        bool isSigned() const { return getIsSigned(); }
        /// Return true if this is an unsigned integer type.
        bool isUnsigned() const { return !getIsSigned(); }

        std::string getAlias() const {
            return (isSigned() ? "i" : "u") + std::to_string(getWidth());
        }
    }];
}

//===----------------------------------------------------------------------===//
// VoidType
//===----------------------------------------------------------------------===//

def Bf3Drmt_VoidType : Bf3Drmt_Type<"Void", "void"> {
    let summary = "void type";
    let description = [{
        Represents absense of result of actions and methods, or `void` type for functions.
    }];
    let extraClassDeclaration = [{
        llvm::StringRef getAlias() const { return "void"; };
    }];
}

//===----------------------------------------------------------------------===//
// ValidType
//===----------------------------------------------------------------------===//

def Bf3Drmt_ValidBitType : Bf3Drmt_Type<"ValidBit", "validity.bit"> {
    let summary = "Valid bit type";
    let description = [{
        `bf3drmt.valid.bit` represents valid bit in headers.
    }];

    let hasCustomAssemblyFormat = 1;
    let extraClassDeclaration = [{
        llvm::StringRef getAlias() const { return "validity_bit"; };
    }];
}

//===----------------------------------------------------------------------===//
// PipeType
//===----------------------------------------------------------------------===//
/*
def Bf3Drmt_PipeType : Bf3Drmt_Type<"Pipe", "pipe"> {
    let summary = "Pipe-like type (actions, methods, functions)";
    let description = [{
        The `!bf3drmt.pipe` is a function type.
    }];

    let parameters = (ins ArrayRefParameter<"mlir::Type">:$inputs,
                        "mlir::Type":$optionalReturnType,
                        OptionalArrayRefParameter<"mlir::Type">:$type_arguments);

    let assemblyFormat = [{
        `<` (`<` $type_arguments^ `>`)? custom<PipeType>($inputs, $optionalReturnType) `>`
    }];

    let extraClassDeclaration = [{
        /// Returns the `i`th input operand type. Asserts if out of bounds.
        mlir::Type getInput(unsigned i) const { return getInputs()[i]; }

        /// Returns the number of arguments to the function.
        unsigned getNumInputs() const { return getInputs().size(); }

        /// Returns the result type of the function as an actual return type or
        /// explicit !p4hir.void
        mlir::Type getReturnType() const;

        /// Returns the result type of the function as an ArrayRef, enabling better
        /// integration with generic MLIR utilities.
        llvm::ArrayRef<mlir::Type> getReturnTypes() const;

        /// Returns a clone of this action type with the given argument
        /// and result types. Required for FunctionOp interface
        PipeType clone(mlir::TypeRange inputs, mlir::TypeRange outputs) const;

        /// Returns whether the function returns void.
        bool isVoid() const;
    }];
}
*/

//===----------------------------------------------------------------------===//
// ReferenceType
//===----------------------------------------------------------------------===//

def Bf3Drmt_ReferenceType : Bf3Drmt_Type<"Reference", "ref"> {
    let summary = "reference type";
    let description = [{
        `p4hir.ref` is a type returned by any op referring to an object in P4.
        This is an implementation detail of P4HIR dialect and should be used in
        very limited cases (e.g. `p4hir.alloca` result type is a reference type).
    }];

    let parameters = (ins "mlir::Type":$objectType);

    let builders = [
        TypeBuilderWithInferredContext<(ins "mlir::Type":$objectType), [{
            return $_get(objectType.getContext(), objectType);
        }]>,
        TypeBuilder<(ins "mlir::Type":$objectType), [{
            return $_get($_ctxt, objectType);
        }]>
    ];

    let assemblyFormat = [{
        `<` $objectType `>`
    }];

    let skipDefaultBuilders = 1;
}

//===----------------------------------------------------------------------===//
// ArrayType
//===----------------------------------------------------------------------===//

def Bf3Drmt_ArrayType : Bf3Drmt_Type<"Array", "array", [
    DeclareTypeInterfaceMethods<DestructurableTypeInterface>,
    DeclareTypeInterfaceMethods<Bf3Drmt_FieldIDTypeInterface>
]> {
    let summary = "fixed-sized array";
    let description = [{
        Fixed sized arrays are roughly similar to C arrays. Arrays are always considered
        packed.
    }];

    let mnemonic = "array";
    let parameters = (ins "size_t":$size, "::mlir::Type":$elementType);

    let builders = [
        AttrBuilderWithInferredContext<
        (ins "size_t":$size, "mlir::Type":$elementType), [{
            return $_get(elementType.getContext(), size, elementType);
        }]>
    ];

    let assemblyFormat = [{
        `<` $size `x` $elementType `>`
    }];
}

//===----------------------------------------------------------------------===//
// StructType
//===----------------------------------------------------------------------===//

class Bf3Drmt_StructLikeType<string name, string typeMnemonic>
    : Bf3Drmt_Type<name, typeMnemonic,  [
    Bf3Drmt_StructLikeTypeInterface,
    DestructurableTypeInterface
]> {
    let hasCustomAssemblyFormat = 1;

    let parameters = (
    ins StringRefParameter<"struct name">:$name,
        ArrayRefParameter<"edamlir::bf3drmt::FieldInfo", "struct fields">:$elements,
        OptionalParameter<"mlir::DictionaryAttr", "annotations">:$annotations
    );

    string extra2ClassDeclaration = "";
    let extraClassDeclaration = !strconcat([{
        mlir::Type getFieldType(mlir::StringRef fieldName) {
            for (const auto &field : getElements())
                if (field.name == fieldName) return field.type;
            return {};
        }

        std::optional<bf3drmt::FieldInfo> getField(mlir::StringRef fieldName) {
            for (const auto &field : getElements())
                if (field.name == fieldName) return field;
            return {};
        }

        llvm::ArrayRef<bf3drmt::FieldInfo> getFields() {
            return getElements();
        }

        void getInnerTypes(mlir::SmallVectorImpl<mlir::Type> &types) {
            for (const auto &field : getElements()) types.push_back(field.type);
        }

        std::optional<unsigned> getFieldIndex(mlir::StringRef fieldName) {
            llvm::ArrayRef<bf3drmt::FieldInfo> elems = getElements();
            for (size_t idx = 0, numElems = elems.size(); idx < numElems; ++idx)
                if (elems[idx].name == fieldName) return idx;
            return {};
        }

        std::optional<unsigned> getFieldIndex(mlir::StringAttr fieldName) {
            llvm::ArrayRef<bf3drmt::FieldInfo> elems = getElements();
            for (size_t idx = 0, numElems = elems.size(); idx < numElems; ++idx)
                if (elems[idx].name == fieldName) return idx;
            return {};
        }

        // FieldID type interface implementation
        std::pair<unsigned, llvm::SmallVector<unsigned>> getFieldIDsStruct() const {
            unsigned fieldID = 0;
            auto elements = getElements();
            llvm::SmallVector<unsigned> fieldIDs;
            fieldIDs.reserve(elements.size());
            for (auto &element : elements) {
                auto type = element.type;
                fieldID += 1;
                fieldIDs.push_back(fieldID);
                // Increment the field ID for the next field by the number of subfields.
                fieldID += FieldIdImpl::getMaxFieldID(type);
            }
            return {fieldID, fieldIDs};
        }

        std::pair<mlir::Type, unsigned> getSubTypeByFieldID(unsigned fieldID) const {
            if (fieldID == 0) return {*this, 0};
            auto [maxId, fieldIDs] = getFieldIDsStruct();
            auto *it = std::prev(llvm::upper_bound(fieldIDs, fieldID));
            auto subfieldIndex = std::distance(fieldIDs.begin(), it);
            auto subfieldType = getElements()[subfieldIndex].type;
            auto subfieldID = fieldID - fieldIDs[subfieldIndex];
            return {subfieldType, subfieldID};
        }

        mlir::Type getTypeAtIndex(mlir::Attribute index) const {
            auto indexAttr = mlir::dyn_cast<mlir::IntegerAttr>(index);
            if (!indexAttr) return {};

            return getSubTypeByFieldID(indexAttr.getInt()).first;
        }

        unsigned getFieldID(unsigned index) const {
            auto [maxId, fieldIDs] = getFieldIDsStruct();
            return fieldIDs[index];
        }

        unsigned getMaxFieldID() const {
            unsigned fieldID = 0;
            for (const auto &field : getElements()) fieldID += 1 + FieldIdImpl::getMaxFieldID(field.type);
            return fieldID;
        }

        unsigned getIndexForFieldID(unsigned fieldID) const {
            assert(!getElements().empty() && "struct must have >0 fields");
            auto [maxId, fieldIDs] = getFieldIDsStruct();
            auto *it = std::prev(llvm::upper_bound(fieldIDs, fieldID));
            return std::distance(fieldIDs.begin(), it);
        }

        std::pair<unsigned, unsigned> getIndexAndSubfieldID(unsigned fieldID) const {
            auto index = getIndexForFieldID(fieldID);
            auto elementFieldID = getFieldID(index);
            return {index, fieldID - elementFieldID};
        }

        std::pair<unsigned, bool> projectToChildFieldID(unsigned fieldID,
                                                        unsigned index) const {
            auto [maxId, fieldIDs] = getFieldIDsStruct();
            auto childRoot = fieldIDs[index];
            auto rangeEnd = index + 1 >= getElements().size() ? maxId : (fieldIDs[index + 1] - 1);
            return std::make_pair(fieldID - childRoot, fieldID >= childRoot && fieldID <= rangeEnd);
        }

        std::optional<llvm::DenseMap<mlir::Attribute, mlir::Type>> getSubelementIndexMap() const {
            llvm::DenseMap<mlir::Attribute, mlir::Type> destructured;
            for (auto [i, field] : llvm::enumerate(getElements()))
                destructured.try_emplace(mlir::IntegerAttr::get(mlir::IndexType::get(getContext()), i), field.type);
            return destructured;
        }
    }], "\n", extra2ClassDeclaration);
}

def Bf3Drmt_StructType : Bf3Drmt_StructLikeType<"Struct", "struct"> {
    let summary = "struct type";
    let description = [{
        Represents a structure of name, value pairs.
        !p4hir.struct<"name", fieldName1: Type1, fieldName2: Type2>
    }];
}


//===----------------------------------------------------------------------===//
// HeaderType
//===----------------------------------------------------------------------===//

def Bf3Drmt_HeaderType : Bf3Drmt_StructLikeType<"Header", "header"> {
    let summary = "header type";
    let description = [{
        Represents a structure of name, value pairs.
        !p4hir.header<"name", fieldName1: Type1, fieldName2: Type2>

        Special field named "__valid" of type !p4hir.validity.bit is used to
        represent validity bit
    }];

    // We skip default builders entirely to consistently add validity bit field on fly
    let skipDefaultBuilders = 1;

    let builders = [
        TypeBuilder<(ins "llvm::StringRef":$name,
                        "llvm::ArrayRef<bf3drmt::FieldInfo>":$fields,
                        CArg<"mlir::DictionaryAttr", "{}">:$annotations)>
    ];

    // This adds more C++ stuff into parent extraClassDeclaration
    let extra2ClassDeclaration = [{
        static constexpr llvm::StringRef validityBit = "__valid";
    }];
}

def Bf3Drmt_StructLikeType : AnyTypeOf<[Bf3Drmt_StructType]>;

/// A ref type with the specified constraints on the nested type.
class Bf3Drmt_SpecificRefType<Type type> : ConfinedType<Bf3Drmt_ReferenceType,
    [SubstLeaves<"$_self", "mlir::cast<bf3drmt::ReferenceType>($_self).getObjectType()", type.predicate>],
    "ref of " # type.summary, "bf3drmt::ReferenceType"
> {
    Type objectType = type;
}

def Bf3Drmt_StructLikeRefType : Bf3Drmt_SpecificRefType<Bf3Drmt_StructLikeType>;

//===----------------------------------------------------------------------===//
// BF3 dRMT type constraints.
//===----------------------------------------------------------------------===//

def AnyBf3DrmtType : AnyTypeOf<[Bf3Drmt_IntegerType, Bf3Drmt_StructType, Bf3Drmt_HeaderType, Bf3Drmt_ArrayType]> {}
def LoadableBf3DrmtType : AnyTypeOf<[Bf3Drmt_IntegerType, Bf3Drmt_StructType, Bf3Drmt_HeaderType, Bf3Drmt_ValidBitType]> {}

#endif // BF3_BF3DRMT_TYPES
