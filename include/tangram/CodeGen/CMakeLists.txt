set(LLVM_TARGET_DEFINITIONS Passes.td)
mlir_tablegen(Passes.h.inc -gen-pass-decls -name BF3DRMT)
add_public_tablegen_target(MLIRBf3DrmtPassesIncGen)