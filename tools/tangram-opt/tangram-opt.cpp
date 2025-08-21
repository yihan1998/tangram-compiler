#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

// #include "Conversion/Passes.h"

#include "Pass/Egglog.h"
#include "Pass/EqualitySaturationPass.h"
#include "Pass/EggifyPass.h"
#include "Pass/EgglogCustomDefs.h"

#include "Pass/P4HIRAdaptivePartitioningPass.h"

// Generate the code for registering conversion passes.
// #define GEN_PASS_REGISTRATION
// #include "Conversion/Passes.h.inc"

#include "Dialect/P4HIR/P4HIR_Dialect.h"
#include "Dialect/P4HIR/P4HIR_Attrs.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtDialect.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h"

std::string getMlirFile(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            return argv[i];
        }
    }

    return "";
}

std::string getEggFile(int argc, char** argv, std::string mlirFile) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (i < argc && (arg == "-egg" || arg == "--egg")) {
            return argv[i + 1];
        }
    }

    // Otherwise, use the mlir file name with .egg extension
    std::string name = mlirFile.substr(0, mlirFile.find(".mlir"));
    return name + ".egg";
}

int main(int argc, char **argv) {
    mlir::registerAllPasses();
    mlir::registerConversionPasses();

    mlir::DialectRegistry registry;
    mlir::registerAllDialects(registry);
    registry.insert<mlir::edamlir::bf3drmt::Bf3DrmtDialect,
                    P4::P4MLIR::P4HIR::P4HIRDialect>();

    mlir::PassRegistration<EggifyPass>();

    // Equality Saturation Pass
    static llvm::cl::opt<std::string> eggFileOpt(
        "egg",
        llvm::cl::desc("Path to egg file"),
        llvm::cl::value_desc("filename"));

    std::string mlirFile = getMlirFile(argc, argv);
    std::string eggFile = getEggFile(argc, argv, mlirFile);

    llvm::outs() << "mlirFile: " << mlirFile << "\n";
    llvm::outs() << "eggFile: " << eggFile << "\n";
    
    std::map<std::string, AttrStringifyFunction> attrStringifiers = {
            {mlir::arith::FastMathFlagsAttr::name.str(), stringifyFastMathFlagsAttr},
            {P4::P4MLIR::P4HIR::IntAttr::name.str(), stringifyP4HIRIntAttr},
            {P4::P4MLIR::P4HIR::MatchKindAttr::name.str(), stringifyP4HIRMatchKindAttr},
        };
    std::map<std::string, AttrParseFunction> attrParsers = {
            {"arith_fastmath", parseFastMathFlagsAttr},
            {"p4hir_int", parseP4HIRIntAttr},
            {"p4hir_match_kind", parseP4HIRMatchKindAttr}};
    std::map<std::string, TypeStringifyFunction> typeStringifiers = {
            {mlir::RankedTensorType::name.str(), stringifyRankedTensorType},
            {P4::P4MLIR::P4HIR::BitsType::name.str(), stringifyP4HIRBitsType},
            {P4::P4MLIR::P4HIR::StructType::name.str(), stringifyP4HIRStructType},
            {P4::P4MLIR::P4HIR::HeaderType::name.str(), stringifyP4HIRHeaderType},
            {P4::P4MLIR::P4HIR::ReferenceType::name.str(), stringifyP4HIRReferenceType},
            {mlir::edamlir::bf3drmt::BitsType::name.str(), stringifyBf3DrmtBitsType},
            {mlir::edamlir::bf3drmt::ValidBitType::name.str(), stringifyBf3DrmtValidBitType},
            {mlir::edamlir::bf3drmt::StructType::name.str(), stringifyBf3DrmtStructType},
            {mlir::edamlir::bf3drmt::HeaderType::name.str(), stringifyBf3DrmtHeaderType},
            {mlir::edamlir::bf3drmt::ReferenceType::name.str(), stringifyBf3DrmtReferenceType},
            };
    std::map<std::string, TypeParseFunction> typeParsers = {
            {"RankedTensor", parseRankedTensorType},
            {"p4hir_bits", parseP4HIRBitsType},
            {"p4hir_struct", parseP4HIRStructType},
            {"p4hir_header", parseP4HIRHeaderType},
            {"p4hir_ref", parseP4HIRReferenceType},
            {"bf3drmt_bits", parseBf3DrmtBitsType},
            {"bf3drmt_valid_bit", parseBf3DrmtValidBitType},
            {"bf3drmt_struct", parseBf3DrmtStructType},
            {"bf3drmt_header", parseBf3DrmtHeaderType},
            {"bf3drmt_ref", parseBf3DrmtReferenceType},
            };

    EgglogCustomDefs funcs = {attrStringifiers, attrParsers, typeStringifiers, typeParsers};
    mlir::PassRegistration<EqualitySaturationPass>([&] {
        return createEqualitySaturationPass(mlirFile, eggFile, funcs); 
    });

#if 0
    mlir::PassRegistration<P4HIRTableKeyEqualitySaturationPass>([&] {
        return createP4HIRTableKeyEqualitySaturationPass(mlirFile, eggFile, funcs); 
    });
#endif

    mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
        return mlir::createP4HIRAdaptivePartitioningPass();
    });

    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "TANGRAM optimizer driver\n", registry));
}
