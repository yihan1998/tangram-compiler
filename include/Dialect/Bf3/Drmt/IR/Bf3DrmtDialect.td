//===- Bf3DrmtDialect.td - BF3 dRMT dialect -----------*- tablegen -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMT_DIALECT
#define BF3_BF3DRMT_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// BF3 dRMT dialect definition.
//===----------------------------------------------------------------------===//

def Bf3Drmt_Dialect : Dialect {
    let name = "bf3drmt";
    let summary = "dRMT dialect on BlueField-3";
    let description = [{
        This dialect represents dRMT on BlueField-3.
    }];
    let cppNamespace = "::mlir::edamlir::bf3drmt";

    let useDefaultTypePrinterParser = 0;
    let useDefaultAttributePrinterParser = 1;
    let hasConstantMaterializer = 1;

    let extraClassDeclaration = [{
        void registerTypes();
        void registerAttributes();
        mlir::Type parseType(mlir::DialectAsmParser &parser) const override;
        void printType(mlir::Type type, mlir::DialectAsmPrinter &printer) const override;
    }];
}

#endif // BF3_BF3DRMT_DIALECT
