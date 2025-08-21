//===- Bf3DrmtOps.cpp - BF3 dRMT dialect ops ---------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#include <string>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/LogicalResult.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Interfaces/FunctionImplementation.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Transforms/InliningUtils.h"

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypeInterfaces.h"

using namespace mlir;
using namespace mlir::edamlir;

/// Use the same printer for both struct_extract and struct_extract_ref since the
/// syntax is identical.
template <typename AggType>
static void printExtractOp(OpAsmPrinter &printer, AggType op) {
    printer << " ";
    printer.printOperand(op.getInput());
    printer << "[\"" << op.getFieldName() << "\"]";
    printer.printOptionalAttrDict(op->getAttrs(), {"fieldIndex"});
    printer << " : ";

    auto type = op.getInput().getType();
    if (auto validType = mlir::dyn_cast<bf3drmt::ReferenceType>(type))
        printer.printStrippedAttrOrType(validType);
    else
        printer << type;
}

static ParseResult parseExtractRefOp(OpAsmParser &parser, OperationState &result) {
    OpAsmParser::UnresolvedOperand operand;
    StringAttr fieldName;
    bf3drmt::ReferenceType declType;

    if (parser.parseOperand(operand) || parser.parseLSquare() || parser.parseAttribute(fieldName) ||
        parser.parseRSquare() || parser.parseOptionalAttrDict(result.attributes) ||
        parser.parseColon() || parser.parseCustomTypeWithFallback<bf3drmt::ReferenceType>(declType))
        return failure();

    auto aggType = mlir::dyn_cast<bf3drmt::Bf3Drmt_StructLikeTypeInterface>(declType.getObjectType());
    if (!aggType) {
        parser.emitError(parser.getNameLoc(), "expected reference to aggregate type");
        return failure();
    }
    auto fieldIndex = aggType.getFieldIndex(fieldName);
    if (!fieldIndex) {
        parser.emitError(parser.getNameLoc(),
                         "field name '" + fieldName.getValue() + "' not found in aggregate type");
        return failure();
    }

    auto indexAttr = IntegerAttr::get(bf3drmt::IntegerType::get(parser.getContext(), 32, false), *fieldIndex);
    result.addAttribute("fieldIndex", indexAttr);
    Type resultType = bf3drmt::ReferenceType::get(aggType.getFields()[*fieldIndex].type);
    result.addTypes(resultType);

    if (parser.resolveOperand(operand, declType, result.operands)) return failure();
    return success();
}

static ParseResult parseExtractOp(OpAsmParser &parser, OperationState &result) {
    OpAsmParser::UnresolvedOperand operand;
    SmallVector<StringAttr> fieldNames;
    Type declType;

    // Parse the input operand
    if (parser.parseOperand(operand))
        return failure();

    // Parse the field path in square brackets
    if (parser.parseLSquare())
        return failure();

    // Parse comma-separated field names
    while (true) {
        StringAttr fieldName;
        if (parser.parseAttribute(fieldName))
            return failure();
        fieldNames.push_back(fieldName);

        // Check for more fields
        if (succeeded(parser.parseOptionalComma()))
            continue;
        break;
    }

    if (parser.parseRSquare() || 
        parser.parseOptionalAttrDict(result.attributes) ||
        parser.parseColon() || 
        parser.parseType(declType))
        return failure();

    // // Verify the input is a reference type
    // auto refType = mlir::dyn_cast<mlir::edamlir::bf3drmt::Bf3Drmt_ReferenceType>(declType);
    // if (!refType) {
    //     parser.emitError(parser.getNameLoc(), "expected reference type");
    //     return failure();
    // }

    // // Verify each field exists in the struct hierarchy
    // Type currentType = refType.getObjectType();
    // for (auto fieldName : fieldNames) {
    //     auto structType = mlir::dyn_cast<mlir::edamlir::bf3drmt::StructLikeTypeInterface>(currentType);
    //     if (!structType) {
    //         parser.emitError(parser.getNameLoc(),
    //             "cannot access field '" + fieldName.getValue() + "' of non-struct type");
    //         return failure();
    //     }

    //     auto fieldIndex = structType.getFieldIndex(fieldName);
    //     if (!fieldIndex) {
    //         parser.emitError(parser.getNameLoc(),
    //             "field name '" + fieldName.getValue() + "' not found in aggregate type");
    //         return failure();
    //     }

    //     currentType = structType.getFields()[*fieldIndex].type;
    // }

    // // Add the field path attribute
    // result.addAttribute("fieldPath", parser.getBuilder().getArrayAttr(fieldNames));

    // // Set the result type (reference to the final field type)
    // result.addTypes(mlir::edamlir::bf3drmt::Bf3Drmt_ReferenceType::get(currentType));

    // // Resolve the operand
    // if (parser.resolveOperand(operand, refType, result.operands))
    //     return failure();

    return success();
}

template <typename AggregateOp>
static LogicalResult verifyAggregateFieldIndexAndType(AggregateOp &op,
                                                      bf3drmt::Bf3Drmt_StructLikeTypeInterface aggType,
                                                      Type elementType) {
    auto index = op.getFieldIndex();
    auto fields = aggType.getFields();
    if (index >= fields.size())
        return op.emitOpError() << "field index " << index
                                << " exceeds element count of aggregate type";

    if (elementType != fields[index].type)
        return op.emitOpError() << "type " << fields[index].type
                                << " of accessed field in aggregate at index " << index
                                << " does not match expected type " << elementType;

    return success();
}

//===----------------------------------------------------------------------===//
// ConstOp
//===----------------------------------------------------------------------===//

void bf3drmt::ConstOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
    if (getName() && !getName()->empty()) {
        setNameFn(getResult(), *getName());
        return;
    }

    setNameFn(getResult(), "cst");
}

LogicalResult bf3drmt::ConstOp::verify() {
    return success();
}

OpFoldResult bf3drmt::ConstOp::fold(FoldAdaptor adaptor) { return getValue(); }

//===----------------------------------------------------------------------===//
// VariableOp
//===----------------------------------------------------------------------===//

void bf3drmt::VariableOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
    if (getName() && !getName()->empty()) setNameFn(getResult(), *getName());
}

LogicalResult bf3drmt::VariableOp::canonicalize(bf3drmt::VariableOp op, PatternRewriter &rewriter) {
    // Check if the variable has one unique assignment to it, all other
    // uses are reads, and that all uses are in the same block as the variable
    // itself.
    auto *block = op->getBlock();
    bf3drmt::AssignOp uniqueAssignOp;
    for (auto *user : op->getUsers()) {
        // Ensure that all users of the variable are in the same block.
        // TODO: Relax this condition, only require assignment to be in the same block
        if (user->getBlock() != block) return failure();

        // Ensure there is at most one unique assignment to the variable.
        if (auto assignOp = mlir::dyn_cast<bf3drmt::AssignOp>(user)) {
            if (uniqueAssignOp) return failure();
            uniqueAssignOp = assignOp;
            continue;
        }

        // Ensure all other users are reads.
        if (!mlir::isa<ReadOp>(user)) return failure();
    }
    if (!uniqueAssignOp) return failure();

    // Remove the assign op and replace all reads with the new assigned var op.
    mlir::Value assignedValue = uniqueAssignOp.getValue();
    rewriter.eraseOp(uniqueAssignOp);
    for (auto *user : llvm::make_early_inc_range(op->getUsers())) {
        auto readOp = mlir::cast<bf3drmt::ReadOp>(user);
        rewriter.replaceOp(readOp, assignedValue);
    }

    // Remove the original variable.
    rewriter.eraseOp(op);
    return success();
}

//===----------------------------------------------------------------------===//
// ControlOp
//===----------------------------------------------------------------------===//

void bf3drmt::ControlOp::build(mlir::OpBuilder &builder, mlir::OperationState &result,
                               llvm::StringRef sym_name, ArrayRef<DictionaryAttr> argAttrs,
                               mlir::DictionaryAttr annotations) {
    result.addRegion();

    result.addAttribute(::mlir::SymbolTable::getSymbolAttrName(), builder.getStringAttr(sym_name));

    // Controls are top-level objects with public visibility
    result.addAttribute(::mlir::SymbolTable::getVisibilityAttrName(), builder.getStringAttr("public"));

    if (annotations && !annotations.empty())
        result.addAttribute(getAnnotationsAttrName(result.name), annotations);
    
    // Add arg_attrs if provided
    if (!argAttrs.empty()) {
        SmallVector<mlir::Attribute> attrs(argAttrs.begin(), argAttrs.end());
        result.addAttribute(getArgAttrsAttrName(result.name), builder.getArrayAttr(attrs));
    }
}

void bf3drmt::ControlOp::createEntryBlock() {
    assert(getBody().empty() && "can only create entry block for empty control");
    Block &first = getBody().emplaceBlock();
    auto loc = getBody().getLoc();
    // Note: No arguments to add since bf3drmt ControlOp doesn't have function type
}

void bf3drmt::ControlOp::print(mlir::OpAsmPrinter &printer) {
    auto funcName = getSymNameAttr().getValue();

    printer << ' ';
    printer.printSymbolName(funcName);

    // Print attributes (excluding the ones we custom print)
    printer.printOptionalAttrDictWithKeyword((*this)->getAttrs(),
        {::mlir::SymbolTable::getSymbolAttrName(), ::mlir::SymbolTable::getVisibilityAttrName(),
         getAnnotationsAttrName(), getArgAttrsAttrName()});

    if (auto ann = getAnnotations(); ann && !ann->empty()) {
        printer << " annotations ";
        printer.printAttributeWithoutType(*ann);
    }

    printer << ' ';
    printer.printRegion(getBody(), /*printEntryBlockArgs=*/false, /*printBlockTerminators=*/true);
}

mlir::ParseResult bf3drmt::ControlOp::parse(mlir::OpAsmParser &parser, mlir::OperationState &result) {
    llvm::SMLoc loc = parser.getCurrentLocation();
    auto &builder = parser.getBuilder();

    // Parse the name as a symbol.
    StringAttr nameAttr;
    if (parser.parseSymbolName(nameAttr, ::mlir::SymbolTable::getSymbolAttrName(), result.attributes))
        return mlir::failure();

    // Controls are visible from top-level
    result.addAttribute(::mlir::SymbolTable::getVisibilityAttrName(), builder.getStringAttr("public"));

    // If additional attributes are present, parse them.
    if (parser.parseOptionalAttrDictWithKeyword(result.attributes)) return failure();

    // Parse annotations
    mlir::DictionaryAttr annotations;
    if (::mlir::succeeded(parser.parseOptionalKeyword("annotations"))) {
        if (parser.parseAttribute<mlir::DictionaryAttr>(annotations)) return failure();
        result.addAttribute(getAnnotationsAttrName(result.name), annotations);
    }

    // Parse the control body.
    auto *body = result.addRegion();
    if (parser.parseRegion(*body, /*arguments=*/{}, /*enableNameShadowing=*/false)) return mlir::failure();

    // Make sure its not empty.
    if (body->empty()) return parser.emitError(loc, "expected non-empty control body");

    return mlir::success();
}

#if 0
//===----------------------------------------------------------------------===//
// BinaryOp
//===----------------------------------------------------------------------===//

void bf3drmt::BinOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
    setNameFn(getResult(), stringifyEnum(getKind()));
}

//===----------------------------------------------------------------------===//
// StructOp
//===----------------------------------------------------------------------===//

ParseResult bf3drmt::StructOp::parse(OpAsmParser &parser, OperationState &result) {
    llvm::SMLoc inputOperandsLoc = parser.getCurrentLocation();
    llvm::SmallVector<OpAsmParser::UnresolvedOperand, 4> operands;
    Type declType;

    if (parser.parseLParen() || parser.parseOperandList(operands) || parser.parseRParen() ||
        parser.parseOptionalAttrDict(result.attributes) || parser.parseColonType(declType))
        return failure();

    auto structType = mlir::dyn_cast<Bf3Drmt_StructLikeTypeInterface>(declType);
    if (!structType) return parser.emitError(parser.getNameLoc(), "expected !p4hir.struct type");

    llvm::SmallVector<Type, 4> structInnerTypes;
    structType.getInnerTypes(structInnerTypes);
    result.addTypes(structType);

    if (parser.resolveOperands(operands, structInnerTypes, inputOperandsLoc, result.operands))
        return failure();
    return success();
}

void bf3drmt::StructOp::print(OpAsmPrinter &printer) {
    printer << " (";
    printer.printOperands(getInput());
    printer << ")";
    printer.printOptionalAttrDict((*this)->getAttrs());
    printer << " : " << getType();
}

LogicalResult bf3drmt::StructOp::verify() {
    auto elements = mlir::cast<Bf3Drmt_StructLikeTypeInterface>(getType()).getFields();

    if (elements.size() != getInput().size()) return emitOpError("struct field count mismatch");

    for (const auto &[field, value] : llvm::zip(elements, getInput()))
        if (field.type != value.getType())
            return emitOpError("struct field `") << field.name << "` type does not match";

    return success();
}

void bf3drmt::StructOp::getAsmResultNames(function_ref<void(Value, StringRef)> setNameFn) {
    llvm::SmallString<32> name;
    if (auto structType = mlir::dyn_cast<StructType>(getType())) {
        name += "struct_";
        name += structType.getName();
    }

    setNameFn(getResult(), name);
}

LogicalResult bf3drmt::StructExtractOp::verify() {
    return verifyAggregateFieldIndexAndType(*this, mlir::cast<bf3drmt::Bf3Drmt_StructLikeTypeInterface>(getInput().getType()), getType());
}

ParseResult bf3drmt::StructExtractOp::parse(OpAsmParser &parser, OperationState &result) {
    return parseExtractOp(parser, result);
}

void bf3drmt::StructExtractOp::print(OpAsmPrinter &printer) { printExtractOp(printer, *this); }

void bf3drmt::StructExtractOp::build(OpBuilder &builder, OperationState &odsState, Value input,
                                   bf3drmt::FieldInfo field) {
    auto structType = mlir::cast<bf3drmt::Bf3Drmt_StructLikeTypeInterface>(input.getType());
    auto fieldIndex = structType.getFieldIndex(field.name);
    assert(fieldIndex.has_value() && "field name not found in aggregate type");
    build(builder, odsState, field.type, input, *fieldIndex);
}

void bf3drmt::StructExtractOp::build(OpBuilder &builder, OperationState &odsState, Value input,
                                   StringAttr fieldName) {
    auto structType = mlir::cast<bf3drmt::Bf3Drmt_StructLikeTypeInterface>(input.getType());
    auto fieldIndex = structType.getFieldIndex(fieldName);
    auto fieldType = structType.getFieldType(fieldName);
    assert(fieldIndex.has_value() && "field name not found in aggregate type");
    build(builder, odsState, fieldType, input, *fieldIndex);
}

void bf3drmt::StructExtractOp::getAsmResultNames(function_ref<void(Value, StringRef)> setNameFn) {
    setNameFn(getResult(), getFieldName());
}

OpFoldResult bf3drmt::StructExtractOp::fold(FoldAdaptor adaptor) {
    // Fold extract from aggregate constant
    // if (auto aggAttr = adaptor.getInput()) {
    //     return mlir::cast<P4HIR::AggAttr>(aggAttr).getFields()[getFieldIndex()];
    // }
    // // Fold extract from struct
    // if (auto structOp = mlir::dyn_cast_or_null<P4HIR::StructOp>(getInput().getDefiningOp())) {
    //     return structOp.getOperand(getFieldIndex());
    // }

    return {};
}
#endif // 0


//===----------------------------------------------------------------------===//
// StructExtractRefOp
//===----------------------------------------------------------------------===//

void bf3drmt::StructExtractRefOp::getAsmResultNames(function_ref<void(Value, StringRef)> setNameFn) {
    llvm::SmallString<16> name = getFieldName();
    name += "_field_ref";
    setNameFn(getResult(), name);
}

ParseResult bf3drmt::StructExtractRefOp::parse(OpAsmParser &parser, OperationState &result) {
    llvm::outs() << "Parsing StructExtractRefOp\n";
    return parseExtractRefOp(parser, result);
}

void bf3drmt::StructExtractRefOp::print(OpAsmPrinter &printer) { printExtractOp(printer, *this); }

LogicalResult bf3drmt::StructExtractRefOp::verify() {
    auto type = mlir::cast<Bf3Drmt_StructLikeTypeInterface>(
    mlir::cast<ReferenceType>(getInput().getType()).getObjectType());
    return verifyAggregateFieldIndexAndType(*this, type, getType().getObjectType());
}

void bf3drmt::StructExtractRefOp::build(OpBuilder &builder, OperationState &odsState, Value input,
                                      bf3drmt::FieldInfo field) {
    auto structLikeType = mlir::cast<ReferenceType>(input.getType()).getObjectType();
    auto structType = mlir::cast<Bf3Drmt_StructLikeTypeInterface>(structLikeType);
    auto fieldIndex = structType.getFieldIndex(field.name);
    assert(fieldIndex.has_value() && "field name not found in aggregate type");
    build(builder, odsState, ReferenceType::get(field.type), input, *fieldIndex);
}

void bf3drmt::StructExtractRefOp::build(OpBuilder &builder, OperationState &odsState, Value input,
                                      StringAttr fieldName) {
    auto structLikeType = mlir::cast<ReferenceType>(input.getType()).getObjectType();
    auto structType = mlir::cast<Bf3Drmt_StructLikeTypeInterface>(structLikeType);
    auto fieldIndex = structType.getFieldIndex(fieldName);
    auto fieldType = structType.getFieldType(fieldName);
    assert(fieldIndex.has_value() && "field name not found in aggregate type");
    build(builder, odsState, ReferenceType::get(fieldType), input, *fieldIndex);
}

//===----------------------------------------------------------------------===//
// ReadOp
//===----------------------------------------------------------------------===//

void bf3drmt::ReadOp::getAsmResultNames(OpAsmSetValueNameFn setNameFn) {
    setNameFn(getResult(), "val");
}

//===----------------------------------------------------------------------===//
// PipeKeyOp
//===----------------------------------------------------------------------===//

void bf3drmt::PipeKeyOp::build(
    mlir::OpBuilder &builder, mlir::OperationState &result, mlir::DictionaryAttr annotations,
    llvm::function_ref<void(mlir::OpBuilder &, mlir::Location)> keyBuilder) {
    if (annotations && !annotations.empty())
        result.addAttribute(getAnnotationsAttrName(result.name), annotations);

    OpBuilder::InsertionGuard guard(builder);

    Region *entryRegion = result.addRegion();
    builder.createBlock(entryRegion);
    keyBuilder(builder, result.location);
}

Operation *bf3drmt::Bf3DrmtDialect::materializeConstant(OpBuilder &builder, Attribute value, Type type, Location loc) {
    auto typedAttr = mlir::cast<mlir::TypedAttr>(value);
    assert(typedAttr.getType() == type && "type mismatch");
    return builder.create<bf3drmt::ConstOp>(loc, typedAttr);
}

#define GET_OP_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.cpp.inc"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.cpp.inc"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOpsEnums.cpp.inc"
