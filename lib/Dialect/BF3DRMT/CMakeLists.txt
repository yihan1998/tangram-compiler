add_mlir_dialect_library(MLIRBF3DRMTDialect
  BF3DRMTDialect.cpp
  BF3DRMTOps.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/tangram/Dialect/BF3DRMT

  DEPENDS
  MLIRBf3DrmtOpsIncGen

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRInferTypeOpInterface
  MLIRSideEffectInterfaces
)