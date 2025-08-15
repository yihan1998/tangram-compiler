//===- Bf3DrmtAttrs.h - BF3 dRMT dialect attributes -----------------*- C++ -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMTATTRS_H
#define BF3_BF3DRMTATTRS_H

// We explicitly do not use push / pop for diagnostic in
// order to propagate pragma further on
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "mlir/IR/BuiltinAttributes.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

namespace mlir::edamlir::bf3drmt::detail {
struct IntAttrStorage : public ::mlir::AttributeStorage {
    using KeyTy = std::tuple<::mlir::Type, llvm::APInt>;
    IntAttrStorage(::mlir::Type type, llvm::APInt value)
        : type(std::move(type)), value(std::move(value)) {}

    KeyTy getAsKey() const { return KeyTy(type, value); }

    bool operator==(const KeyTy &rhs) const {
        auto rhsType = std::get<0>(rhs);
        const auto &rhsValue = std::get<1>(rhs);
        return (type == rhsType) &&
               (value.getBitWidth() == rhsValue.getBitWidth() && value == rhsValue);
    }

    static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
        return ::llvm::hash_combine(std::get<0>(tblgenKey), std::get<1>(tblgenKey));
    }

    static IntAttrStorage *construct(::mlir::AttributeStorageAllocator &allocator,
                                     KeyTy &&tblgenKey) {
        auto type = std::move(std::get<0>(tblgenKey));
        auto value = std::move(std::get<1>(tblgenKey));
        return new (allocator.allocate<IntAttrStorage>())
            IntAttrStorage(std::move(type), std::move(value));
    }

    mlir::Type type;
    llvm::APInt value;
};

}  // namespace P4::P4MLIR::P4HIR::detail

#define GET_ATTRDEF_CLASSES
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h.inc"

#endif  // BF3_BF3DRMTATTRS_H