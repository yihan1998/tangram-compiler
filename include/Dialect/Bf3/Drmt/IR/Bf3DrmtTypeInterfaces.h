#ifndef BF3_BF3DRMTTYPEINTERFACES_H
#define BF3_BF3DRMTTYPEINTERFACES_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Types.h"

namespace mlir::edamlir::bf3drmt {

/// Struct defining a field. Used in structs and header
struct FieldInfo {
    mlir::StringAttr name;
    mlir::Type type;
    mlir::DictionaryAttr annotations;

    FieldInfo(mlir::StringAttr name,
                mlir::Type type,
                mlir::DictionaryAttr annotations = {})
        : name(name), type(type),
        annotations(annotations && !annotations.empty() ? annotations : mlir::DictionaryAttr())
    { }
};

namespace FieldIdImpl {
unsigned getMaxFieldID(::mlir::Type);

std::pair<::mlir::Type, unsigned> getSubTypeByFieldID(::mlir::Type, unsigned fieldID);

::mlir::Type getFinalTypeByFieldID(::mlir::Type type, unsigned fieldID);

std::pair<unsigned, bool> projectToChildFieldID(::mlir::Type, unsigned fieldID, unsigned index);

std::pair<unsigned, unsigned> getIndexAndSubfieldID(::mlir::Type type, unsigned fieldID);

unsigned getFieldID(::mlir::Type type, unsigned index);

unsigned getIndexForFieldID(::mlir::Type type, unsigned fieldID);

}  // namespace FieldIdImpl

}  // mlir::edamlir::bf3drmt

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypeInterfaces.h.inc"

#endif  // BF3_BF3DRMTTYPEINTERFACES_H
