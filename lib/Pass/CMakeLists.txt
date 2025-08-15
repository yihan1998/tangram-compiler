add_mlir_library(MLIREqSatPass
  Egglog.cpp
  EqualitySaturationPass.cpp

  ADDITIONAL_HEADER_DIRS
  ${MLIR_MAIN_INCLUDE_DIR}/mlir/Pass

  DEPENDS
  mlir-generic-headers

  LINK_LIBS PUBLIC
  MLIRAnalysis
  MLIRIR
  )
