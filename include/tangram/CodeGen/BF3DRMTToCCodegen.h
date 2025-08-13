#ifndef TANGRAM_CODEGEN_BF3DRMTTOCCODEGEN_H
#define TANGRAM_CODEGEN_BF3DRMTTOCCODEGEN_H

#include "mlir/Pass/Pass.h"

namespace mlir {
namespace bf3drmt {

std::unique_ptr<Pass> createBF3DRMTToCCodegenPass();

// Include both declarations and registration
#define GEN_PASS_DECL
#define GEN_PASS_REGISTRATION 
#include "tangram/CodeGen/Passes.h.inc"

} // namespace bf3drmt
} // namespace mlir

#endif // TANGRAM_CODEGEN_BF3DRMTTOCCODEGEN_H