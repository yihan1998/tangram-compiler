#ifndef EGGLOG_CUSTOM_DEFS_H
#define EGGLOG_CUSTOM_DEFS_H

#include "mlir/IR/Attributes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/AsmParser/AsmParser.h"
#include "Dialect/P4HIR/P4HIR_Attrs.h"
#include "Dialect/P4HIR/P4HIR_Types.h"

#include "Egglog.h"

/** 
 * Parse the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
mlir::Attribute parseFastMathFlagsAttr(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "arith_fastmath");

    std::string flag = split[1];
    if (flag.front() == '(' && flag.back() == ')') {
        flag = flag.substr(1, flag.size() - 2);
    }

    std::string strAttr = "#arith.fastmath<" + flag + ">";
    mlir::Attribute parsedAttr = mlir::parseAttribute(strAttr, &egglog.context);

    // dump
    // llvm::outs() << "Parsing mlir::arith::FastMathFlagsAttr: " << strAttr << "\n";
    // llvm::outs() << "Parsed mlir::arith::FastMathFlagsAttr: " << parsedAttr << "\n";

    return parsedAttr;
}

/**
 * Serialize the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
std::vector<std::string> stringifyFastMathFlagsAttr(mlir::Attribute attr, Egglog& egglog) {
    std::vector<std::string> split;

    split.push_back("arith_fastmath");

    mlir::arith::FastMathFlagsAttr fastMathAttr = attr.cast<mlir::arith::FastMathFlagsAttr>();
    mlir::arith::FastMathFlags flags = fastMathAttr.getValue();

    split.push_back("(" + mlir::arith::stringifyFastMathFlags(flags) + ")");

    // dump
    llvm::outs() << "Stringified mlir::arith::FastMathFlagsAttr: ";
    for (const std::string& s: split) {
        llvm::outs() << s << " , ";
    }
    llvm::outs() << "\n";

    return split;
}

/** Parse the type (function RankedTensor (IntVec Type) Type) */
mlir::Type parseRankedTensorType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "RankedTensor");

    std::string dimVec = split[1];
    std::string type = split[2];

    // parse dimvec, form (vec-of <N1> <N2> ... <Nn>)
    std::vector<std::string> dimVecSplit = Egglog::splitExpression(dimVec);
    std::vector<int64_t> dims;
    for (unsigned int i = 1; i < dimVecSplit.size(); i++) {
        dims.push_back(std::stoll(dimVecSplit[i]));
    }

    mlir::Type parsedType = egglog.parseType(type);  // parse type
    return mlir::RankedTensorType::get(dims, parsedType);
}

/** Serialize the type (function RankedTensor (IntVec Type) Type) */
std::vector<std::string> stringifyRankedTensorType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    split.push_back("RankedTensor");

    mlir::RankedTensorType tensorType = type.cast<mlir::RankedTensorType>();
    llvm::ArrayRef<int64_t> shape = tensorType.getShape();
    mlir::Type elementType = tensorType.getElementType();

    // serialize dimvec
    std::string dimVec = "(vec-of";
    for (int64_t dim: shape) {
        dimVec += " " + std::to_string(dim);
    }
    dimVec += ")";

    split.push_back(dimVec);
    split.push_back(egglog.eggifyType(elementType));

    return split;
}

/** 
 * Parse the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
mlir::Attribute parseP4HIRIntAttr(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "p4hir_int");

    for (size_t i = 1; i < split.size(); i++) {
        llvm::outs() << "Parsing P4HIR IntAttr: " << split[i] << "\n";
    }

    // Parse the value (first argument)
    std::string valueStr = split[1];
    if (valueStr.front() == '(' && valueStr.back() == ')') {
        valueStr = valueStr.substr(1, valueStr.size() - 2);
    }
    int64_t value = std::stoll(valueStr);

    llvm::outs() << "Parsed value: " << value << "\n";

    // Parse the type (second argument should be a bit type)
    std::string typeStr = split[2];
    mlir::Type type = egglog.parseType(typeStr);

    llvm::outs() << "Parsed type: " << type << "\n";
    
    // Use mlir::dyn_cast instead of type.dyn_cast
    auto bitType = mlir::dyn_cast<P4::P4MLIR::P4HIR::BitsType>(type);
    assert(bitType && "P4HIR int attribute must have bit type");

    llvm::outs() << "Bit width: " << bitType.getWidth() << "\n";

    // Create APInt with the value and bit width
    llvm::APInt apValue(bitType.getWidth(), value, /*isSigned=*/true);

    llvm::outs() << "Parsing P4HIR IntAttr: value=" << value 
                 << ", width=" << bitType.getWidth() << "\n";

    // Create P4HIR integer attribute with correct parameter order: context, type, value
    auto attr = P4::P4MLIR::P4HIR::IntAttr::get(&egglog.context, type, apValue);
    
    llvm::outs() << "Parsed P4HIR IntAttr: " << attr << "\n";
    return attr;
}

std::vector<std::string> stringifyP4HIRIntAttr(mlir::Attribute attr, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Cast to P4HIR IntAttr
    auto intAttr = mlir::dyn_cast<P4::P4MLIR::P4HIR::IntAttr>(attr);
    assert(intAttr && "Expected P4HIR IntAttr");

    // Get the value as APInt
    llvm::APInt value = intAttr.getValue();
    
    // Get the type
    mlir::Type type = intAttr.getType();

    split.push_back("p4hir_int");
    // Convert APInt to string - use getSExtValue() for signed interpretation
    split.push_back(std::to_string(value.getSExtValue()));
    // Add the type
    split.push_back(egglog.eggifyType(type));

    return split;
}

/** Parse P4HIR bit type (function p4hir.bits (Int) Type) */
mlir::Type parseP4HIRBitsType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_bits");

    // Parse width parameter
    std::string width = split[1];
    if (width.front() == '(' && width.back() == ')') {
        width = width.substr(1, width.size() - 2);
    }

    // Create the type string and parse it
    std::string strType = "!p4hir.bit<" + width + ">";
    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);

    return parsedType;
}

/** Stringify P4HIR bit type */
std::vector<std::string> stringifyP4HIRBitsType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the bit width from the type
    auto bitsType = type.cast<P4::P4MLIR::P4HIR::BitsType>();
    unsigned width = bitsType.getWidth();

    split.push_back("p4hir_bits");
    split.push_back(std::to_string(width));

    return split;
}

/** Parse P4HIR reference type (function p4hir_ref (Type) Type) */
mlir::Type parseP4HIRReferenceType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_ref");

    // Parse inner type parameter
    std::string innerTypeStr = split[1];
    
    // Use egglog's parseType method like in parseRankedTensorType
    mlir::Type innerType = egglog.parseType(innerTypeStr);
    if (!innerType) {
        llvm::errs() << "Failed to parse inner type: " << innerTypeStr << "\n";
        return nullptr;
    }

    // Create reference type using P4HIR's RefType::get
    return P4::P4MLIR::P4HIR::ReferenceType::get(innerType);
}

/** Stringify P4HIR reference type */
std::vector<std::string> stringifyP4HIRReferenceType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the inner type from the reference type
    auto refType = type.cast<P4::P4MLIR::P4HIR::ReferenceType>();
    mlir::Type innerType = refType.getObjectType();

    // Get the string representation of the inner type using egglog's eggifyType
    std::string innerTypeStr = egglog.eggifyType(innerType);
    
    split.push_back("p4hir_ref");
    // Add the inner type WITHOUT extra parentheses
    split.push_back(innerTypeStr);  // Remove the extra parentheses

    return split;
}

#endif  // EGGLOG_CUSTOM_DEFS_H