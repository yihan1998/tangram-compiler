/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Definitions                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: BF3DRMTOps.td                                                        *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

MLIR_DEFINE_EXPLICIT_TYPE_ID(::mlir::bf3drmt::BF3DRMTDialect)
namespace mlir {
namespace bf3drmt {

BF3DRMTDialect::BF3DRMTDialect(::mlir::MLIRContext *context)
    : ::mlir::Dialect(getDialectNamespace(), context, ::mlir::TypeID::get<BF3DRMTDialect>()) {
  
  initialize();
}

BF3DRMTDialect::~BF3DRMTDialect() = default;

} // namespace bf3drmt
} // namespace mlir
