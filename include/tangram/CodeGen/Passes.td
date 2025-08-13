#ifndef TANGRAM_CODEGEN_PASSES_TD
#define TANGRAM_CODEGEN_PASSES_TD

include "mlir/Pass/PassBase.td"

def BF3DRMTToCCodegenPass : Pass<"bf3drmt-to-c", "mlir::ModuleOp"> {
  let summary = "Convert BF3DRMT dialect to C source code";
  let description = [{
    This pass translates the optimized BF3DRMT IR (after deployment reasoning) 
    to idiomatic C source code.
  }];
  
  let constructor = "mlir::bf3drmt::createBF3DRMTToCCodegenPass()";
  
  let options = [
    Option<"outputPath", "output-path", "std::string", /*default=*/"\"output.c\"",
           "Path to write the generated C source code">,
  ];
}

#endif // TANGRAM_CODEGEN_PASSES_TD