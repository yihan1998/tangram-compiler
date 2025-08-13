#ifndef BF3DRMT_DIALECT
#define BF3DRMT_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// BF3DRMT dialect definition.
//===----------------------------------------------------------------------===//

def BF3DRMT_Dialect : Dialect {
  let name = "bf3drmt";
  let summary = "A high-level dialect for BF3DRMT operations.";
  let description = [{
    The BF3DRMT dialect provides operations for representing hash functions,
    bit manipulation, and deployment-aware optimizations.
  }];
  let cppNamespace = "::mlir::bf3drmt";
  
  let useDefaultTypePrinterParser = 1;
  let useDefaultAttributePrinterParser = 1;
}

//===----------------------------------------------------------------------===//
// Base BF3DRMT operation definition.
//===----------------------------------------------------------------------===//

class BF3DRMT_Op<string mnemonic, list<Trait> traits = []> :
        Op<BF3DRMT_Dialect, mnemonic, traits>;

#endif // BF3DRMT_DIALECT