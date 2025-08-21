add_mlir_library(MLIREqSatPass
  Egglog.cpp
  EgglogCustomDefs.cpp
  EqualitySaturationPass.cpp

  PARTIAL_SOURCES_INTENDED

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Pass

  DEPENDS
  mlir-generic-headers

  LINK_LIBS PUBLIC
  MLIRAnalysis
  MLIRIR
  MLIRPass
)

add_mlir_library(MLIRP4HIRAdaptivePartitioningPass
  P4HIRAdaptivePartitioningPass.cpp

  PARTIAL_SOURCES_INTENDED

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Pass

  DEPENDS
  mlir-generic-headers

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRPass
  MLIRTransforms
  MLIRBF3DRMT
)