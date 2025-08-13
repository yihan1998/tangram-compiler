#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "tangram/Dialect/BF3DRMT/BF3DRMTDialect.h"
#include "tangram/CodeGen/BF3DRMTToCCodegen.h"

int main(int argc, char **argv) {
  mlir::registerAllPasses();
  
  // Register our passes
  mlir::bf3drmt::registerBF3DRMTToCCodegenPass();

  mlir::DialectRegistry registry;
  registry.insert<mlir::bf3drmt::BF3DRMTDialect>();
  mlir::registerAllDialects(registry);

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "Tangram optimizer\n", registry));
}