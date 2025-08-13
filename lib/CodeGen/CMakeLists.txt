add_mlir_library(MLIRBf3DrmtCodeGen
  BF3DRMTToCCodegen.cpp
  CSourceEmitter.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/tangram/CodeGen

  DEPENDS
  MLIRBf3DrmtPassesIncGen

  LINK_LIBS PUBLIC
  MLIRBF3DRMTDialect
  MLIRFuncDialect
  MLIRIR
  MLIRPass
  MLIRSupport
)