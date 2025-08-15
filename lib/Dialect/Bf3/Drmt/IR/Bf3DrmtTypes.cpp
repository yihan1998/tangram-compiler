//===- Bf3DrmtTypes.cpp - BF3 dRMT dialect types -----------*- C++ -*-===//
//===----------------------------------------------------------------------===//
#include <iostream>

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/TypeSwitch.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/Support/LLVM.h"

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"

using namespace mlir;
using namespace edamlir;
using namespace edamlir::bf3drmt;

#define GET_TYPEDEF_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.cpp.inc"

static mlir::ParseResult parseFuncType(mlir::AsmParser &p, llvm::SmallVector<mlir::Type> &params,
                                       mlir::Type &optionalResultType);
static mlir::ParseResult parseFuncType(mlir::AsmParser &p, llvm::SmallVector<mlir::Type> &params);

static void printFuncType(mlir::AsmPrinter &p, mlir::ArrayRef<mlir::Type> params, mlir::Type optionalResultType = {});

//===----------------------------------------------------------------------===//
// IntegerType
//===----------------------------------------------------------------------===//

void bf3drmt::IntegerType::print(AsmPrinter &printer) const {
    printer << getWidth() << (isSigned() ? "i" : "u");
}

mlir::Type bf3drmt::IntegerType::parse(mlir::AsmParser &parser) {
    // Parse the '<' 
    if (parser.parseLess()) 
        return nullptr;

    // Parse the width
    unsigned width;
    if (parser.parseInteger(width))
        return nullptr;

    // Default to unsigned if not specified
    bool isSigned = false;

    // Parse optional signedness
    if (succeeded(parser.parseOptionalColon())) {
        // Parse 's' for signed or 'u' for unsigned
        StringRef signSpecifier;
        if (parser.parseKeyword(&signSpecifier))
            return nullptr;

        if (signSpecifier == "s") {
            isSigned = true;
        } else if (signSpecifier == "u") {
            isSigned = false;
        } else {
            parser.emitError(parser.getNameLoc(), 
                            "expected 's' for signed or 'u' for unsigned");
            return nullptr;
        }
    }

    // Parse the '>'
    if (parser.parseGreater())
        return nullptr;

    // Get the context and return the constructed type
    mlir::MLIRContext *ctx = parser.getContext();
    return IntegerType::get(ctx, width, isSigned);
}

//===----------------------------------------------------------------------===//
// ValidType
//===----------------------------------------------------------------------===//

Type ValidBitType::parse(mlir::AsmParser &parser) { return get(parser.getContext()); }

void ValidBitType::print(mlir::AsmPrinter &printer) const {}

//===----------------------------------------------------------------------===//
// StructLikeType
//===----------------------------------------------------------------------===//

static ParseResult parseFields(AsmParser &p, std::string &name,
                               SmallVectorImpl<FieldInfo> &parameters,
                               mlir::DictionaryAttr &annotations) {
    llvm::StringSet<> nameSet;
    mlir::NamedAttrList annList;
    bool hasDuplicateName = false;
    bool parsedName = false;
    auto parseResult =
        p.parseCommaSeparatedList(mlir::AsmParser::Delimiter::LessGreater, [&]() -> ParseResult {
            // First, try to parse name
            if (!parsedName) {
                if (p.parseKeywordOrString(&name) || p.parseOptionalAttrDict(annList))
                    return failure();
                parsedName = true;
                annotations = annList.getDictionary(p.getContext());
                return success();
            }

            // Parse fields
            std::string fieldName;
            Type fieldType;
            mlir::NamedAttrList fieldAnnotations;

            auto fieldLoc = p.getCurrentLocation();
            if (p.parseKeywordOrString(&fieldName) || p.parseColon() || p.parseType(fieldType) ||
                p.parseOptionalAttrDict(fieldAnnotations))
                return failure();

            if (!nameSet.insert(fieldName).second) {
                p.emitError(fieldLoc, "duplicate field name \'" + name + "\'");
                // Continue parsing to print all duplicates, but make sure to error
                // eventually
                hasDuplicateName = true;
            }

            parameters.emplace_back(StringAttr::get(p.getContext(), fieldName), fieldType,
                                    fieldAnnotations.getDictionary(p.getContext()));
            return success();
        });

    if (hasDuplicateName) return failure();
    return parseResult;
}

/// Print out a list of named fields surrounded by <>.
static void printFields(AsmPrinter &p, StringRef name, ArrayRef<FieldInfo> fields,
                        mlir::DictionaryAttr annotations) {
    p << '<';
    p.printString(name);
    if (annotations && !annotations.empty()) {
        p << ' ';
        p.printAttributeWithoutType(annotations);
    }
    if (!fields.empty()) p << ", ";
    llvm::interleaveComma(fields, p, [&](const FieldInfo &field) {
        p.printKeywordOrString(field.name.getValue());
        p << ": " << field.type;
        if (field.annotations && !field.annotations.empty()) {
            p << " ";
            p.printAttributeWithoutType(field.annotations);
        }
    });
    p << ">";
}

//===----------------------------------------------------------------------===//
// Parse
//===----------------------------------------------------------------------===//

Type HeaderType::parse(AsmParser &p) {
    llvm::SmallVector<FieldInfo, 4> parameters;
    std::string name;
    mlir::DictionaryAttr annotations;
    if (parseFields(p, name, parameters, annotations)) return {};
    // Do not use our own get() here as it adds __validity bit. And we do have it already.
    return Base::get(p.getContext(), name, parameters,
                     annotations && !annotations.empty() ? annotations : mlir::DictionaryAttr());
}

//===----------------------------------------------------------------------===//
// Verify
//===----------------------------------------------------------------------===//
#if 0
LogicalResult HeaderType::verify(function_ref<InFlightDiagnostic()> emitError, StringRef,
                                 ArrayRef<FieldInfo> elements, DictionaryAttr) {
    if (elements.empty()) {
        emitError() << "empty p4hir.header type";
        return failure();
    }

    LogicalResult result = success();
    llvm::SmallDenseSet<StringAttr> fieldNameSet;
    fieldNameSet.reserve(elements.size());
    for (const auto &elt : elements) {
        if (!fieldNameSet.insert(elt.name).second) {
            result = failure();
            emitError() << "duplicate field name '" << elt.name.getValue()
                        << "' in p4hir.header type";
        }
    }

    auto lastField = elements.back();
    if (lastField.name != validityBit || !mlir::isa<P4HIR::ValidBitType>(lastField.type)) {
        result = failure();
        emitError() << "the last field of p4hir.header type should be validity bit, but got"
                    << lastField.name << " of type " << lastField.type;
    }

    auto varbitCount = llvm::count_if(
        elements, [](const FieldInfo &field) { return mlir::isa<P4HIR::VarBitsType>(field.type); });

    if (varbitCount > 1) {
        result = failure();
        emitError() << "only one varbit field is allowed in p4hir.header type";
    }

    // If a varbit field is found, ensure it is the last “data” field
    // immediately before the validity field
    if (varbitCount > 0) {
        // Find the index of the varbit field
        auto varbitField = llvm::find_if(elements, [](const FieldInfo &field) {
            return mlir::isa<P4HIR::VarBitsType>(field.type);
        });

        size_t varbitIndex = std::distance(elements.begin(), varbitField);
        size_t expectedIndex = elements.size() - 2;  // The index is right before validity bit

        if (varbitIndex != expectedIndex) {
            result = failure();
            emitError() << "varbit field " << varbitField->name
                        << " must be immediately before the validity field in p4hir.header type";
        }
    }

    // Helper function to verify inner types recursively, inspecting nested structs
    std::function<void(ArrayRef<FieldInfo>, bool)> verifyInnerTypes =
        [&](ArrayRef<FieldInfo> elements, bool isHeader) -> void {
        for (const auto &elt : elements) {
            auto fieldType = elt.type;
            while (auto aliasType = mlir::dyn_cast<P4HIR::AliasType>(fieldType))
                fieldType = aliasType.getAliasedType();

            // varbit and validity bit are allowed at the top (header) level only
            if (isHeader && mlir::isa<P4HIR::VarBitsType, P4HIR::ValidBitType>(fieldType)) {
                continue;
            }
            if (mlir::isa<P4HIR::BitsType, P4HIR::BoolType, P4HIR::SerEnumType>(fieldType)) {
                continue;
            }
            if (const auto structType = mlir::dyn_cast<P4HIR::StructType>(fieldType)) {
                verifyInnerTypes(structType.getElements(), false);
                continue;
            }

            result = failure();
            emitError() << "field name " << elt.name << " is of type '" << fieldType
                        << "' that is not allowed in p4hir.header type";
        }
    };

    verifyInnerTypes(elements, true);

    return result;
}
#endif
//===----------------------------------------------------------------------===//
// Print
//===----------------------------------------------------------------------===//

void StructType::print(AsmPrinter &p) const {
    printFields(p, getName(), getElements(), getAnnotations());
}

void HeaderType::print(AsmPrinter &p) const {
    printFields(p, getName(), getElements(), getAnnotations());
}

HeaderType HeaderType::get(mlir::MLIRContext *context, llvm::StringRef name,
                           llvm::ArrayRef<FieldInfo> fields, mlir::DictionaryAttr annotations) {
    llvm::SmallVector<FieldInfo, 4> realFields(fields);
    realFields.emplace_back(mlir::StringAttr::get(context, validityBit),
                            bf3drmt::ValidBitType::get(context));

    return Base::get(context, name, realFields,
                     annotations && !annotations.empty() ? annotations : mlir::DictionaryAttr());
}
//===----------------------------------------------------------------------===//
// ArrayType
//===----------------------------------------------------------------------===//

unsigned ArrayType::getMaxFieldID() const {
    return getSize() * (FieldIdImpl::getMaxFieldID(getElementType()) + 1);
}

std::pair<Type, unsigned> ArrayType::getSubTypeByFieldID(unsigned fieldID) const {
    if (fieldID == 0) return {*this, 0};
    return {getElementType(), getIndexAndSubfieldID(fieldID).second};
}

std::pair<unsigned, bool> ArrayType::projectToChildFieldID(unsigned fieldID, unsigned index) const {
    auto childRoot = getFieldID(index);
    auto rangeEnd = index >= getSize() ? getMaxFieldID() : (getFieldID(index + 1) - 1);
    return std::make_pair(fieldID - childRoot, fieldID >= childRoot && fieldID <= rangeEnd);
}

unsigned ArrayType::getIndexForFieldID(unsigned fieldID) const {
    assert(fieldID && "fieldID must be at least 1");
    // Divide the field ID by the number of fieldID's per element.
    return (fieldID - 1) / (FieldIdImpl::getMaxFieldID(getElementType()) + 1);
}

std::pair<unsigned, unsigned> ArrayType::getIndexAndSubfieldID(unsigned fieldID) const {
    auto index = getIndexForFieldID(fieldID);
    auto elementFieldID = getFieldID(index);
    return {index, fieldID - elementFieldID};
}

unsigned ArrayType::getFieldID(unsigned index) const {
    return 1 + index * (FieldIdImpl::getMaxFieldID(getElementType()) + 1);
}

std::optional<DenseMap<Attribute, Type>> ArrayType::getSubelementIndexMap() const {
    DenseMap<Attribute, Type> destructured;
    for (unsigned i = 0; i < getSize(); ++i)
        destructured.insert({IntegerAttr::get(IndexType::get(getContext()), i), getElementType()});
    return destructured;
}

Type ArrayType::getTypeAtIndex(Attribute) const { return getElementType(); }

namespace mlir::edamlir::bf3drmt {
bool operator==(const FieldInfo &a, const FieldInfo &b) {
    return a.name == b.name && a.type == b.type;
}
llvm::hash_code hash_value(const FieldInfo &fi) { return llvm::hash_combine(fi.name, fi.type); }
}

void Bf3DrmtDialect::printType(mlir::Type type, mlir::DialectAsmPrinter &os) const {
    // Try to print as a tablegen'd type.
    if (generatedTypePrinter(type, os).succeeded()) return;

    // Add some special handling for certain types
    TypeSwitch<Type>(type).Case<IntegerType>([&](IntegerType type) { type.print(os); }).Default([](Type) {
        llvm::report_fatal_error("printer is missing a handler for this type");
    });
}

void Bf3DrmtDialect::registerTypes() {
    std::cout << "Register types..." << std::endl;
    addTypes<
#define GET_TYPEDEF_LIST
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.cpp.inc"
        >();
}
