//===- Bf3DrmtAttrs.cpp - BF3 dRMT dialect attributes ---------------*- C++ -*-===//
//===----------------------------------------------------------------------===//
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h"

#include "llvm/ADT/TypeSwitch.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

#define GET_ATTRDEF_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.cpp.inc"

using namespace mlir;
using namespace edamlir::bf3drmt;

mlir::Type IntAttr::getType() const { return getImpl()->type; }

llvm::APInt IntAttr::getValue() const { return getImpl()->value; }

Attribute mlir::edamlir::bf3drmt::IntAttr::parse(AsmParser &parser, Type odsType) {
    mlir::APInt APValue;
    mlir::Type valType = odsType;

    // Consume the '<' symbol.
    if (parser.parseLess()) return {};

    if (auto type = mlir::dyn_cast<IntegerType>(valType)) {
        // Fetch arbitrary precision integer value.
        if (type.isSigned()) {
            mlir::APInt value;
            if (parser.parseInteger(value)) {
                parser.emitError(parser.getCurrentLocation(), "expected integer value");
                return {};
            }
            if (!value.isSignedIntN(type.getWidth())) {
                parser.emitError(parser.getCurrentLocation(),
                                 "integer value too large for the given type");
                return {};
            }
            APValue = value.sextOrTrunc(type.getWidth());
        } else {
            mlir::APInt value;
            if (parser.parseInteger(value)) {
                parser.emitError(parser.getCurrentLocation(), "expected integer value");
                return {};
            }
            if (!value.isIntN(type.getWidth())) {
                parser.emitError(parser.getCurrentLocation(),
                                 "integer value too large for the given type");
                return {};
            }
            APValue = value.zextOrTrunc(type.getWidth());
        }
    } else if (parser.parseInteger(APValue)) {
        parser.emitError(parser.getCurrentLocation(), "expected integer value");
        return {};
    }

    // Consume the '>' symbol.
    if (parser.parseGreater()) return {};

    return IntAttr::get(odsType, APValue);
}

void mlir::edamlir::bf3drmt::IntAttr::print(AsmPrinter &printer) const {
    printer << '<';

    // Extract the underlying integer value
    llvm::APInt val = getValue();

    // Check if type is bf3drmt.integer and print signed or unsigned
    if (auto intTy = llvm::dyn_cast<bf3drmt::IntegerType>(getType())) {
        val.print(printer.getStream(), intTy.isSigned());
        printer << " : " << getType(); // Print the type for disambiguation
    } else {
        // Fallback if the type is not recognized
        printer << val << " : " << getType();
    }

    printer << '>';
}

LogicalResult IntAttr::verify(function_ref<InFlightDiagnostic()> emitError, Type type,
                              APInt value) {
    // while (auto aliasType = mlir::dyn_cast<AliasType>(type)) type = aliasType.getAliasedType();

    // if (!mlir::isa<BitsType, InfIntType>(type)) {
    //     emitError() << "expected integer type";
    //     return failure();
    // }

    // if (auto intType = mlir::dyn_cast<BitsType>(type)) {
    //     if (value.getBitWidth() != intType.getWidth()) {
    //         emitError() << "type and value bitwidth mismatch: " << intType.getWidth()
    //                     << " != " << value.getBitWidth();
    //         return failure();
    //     }
    // }

    return success();
}

void Bf3DrmtDialect::registerAttributes() {
    addAttributes<
#define GET_ATTRDEF_LIST
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.cpp.inc"  // NOLINT
        >();
}
