add_mlir_dialect_library(MLIRBF3DRMT
    Bf3DrmtTypes.cpp
    Bf3DrmtTypeInterfaces.cpp
    Bf3DrmtDialect.cpp
    Bf3DrmtOps.cpp
    Bf3DrmtAttrs.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/Dialect/Bf3

    DEPENDS
    EDAMLIR_BF3DRMT_IncGen

    LINK_LIBS PUBLIC
    MLIRIR
    MLIRInferTypeOpInterface
    MLIRFuncDialect
    MLIREqSatPass
    P4MLIR_P4HIR
)
