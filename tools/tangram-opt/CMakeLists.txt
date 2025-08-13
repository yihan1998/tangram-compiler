get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)

add_llvm_executable(tangram-opt tangram-opt.cpp)

llvm_update_compile_flags(tangram-opt)
target_link_libraries(tangram-opt PRIVATE
  ${dialect_libs}
  ${conversion_libs}
  MLIRBF3DRMTDialect
  MLIRBf3DrmtCodeGen
  MLIROptLib
)