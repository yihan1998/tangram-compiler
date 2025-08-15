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

/// Generate the code for registering conversion passes.
// #define GEN_PASS_REGISTRATION
// #include "Conversion/Passes.h.inc"

#include "Dialect/P4HIR/P4HIR_Dialect.h"
#include "Dialect/P4HIR/P4HIR_Attrs.h"

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
    registry.insert<P4::P4MLIR::P4HIR::P4HIRDialect>();

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
        };
    std::map<std::string, AttrParseFunction> attrParsers = {
            {"arith_fastmath", parseFastMathFlagsAttr},
            {"p4hir_int", parseP4HIRIntAttr}};
    std::map<std::string, TypeStringifyFunction> typeStringifiers = {
            {mlir::RankedTensorType::name.str(), stringifyRankedTensorType},
            {P4::P4MLIR::P4HIR::BitsType::name.str(), stringifyP4HIRBitsType},
            {P4::P4MLIR::P4HIR::ReferenceType::name.str(), stringifyP4HIRReferenceType}};
    std::map<std::string, TypeParseFunction> typeParsers = {
            {"RankedTensor", parseRankedTensorType},
            {"p4hir_bits", parseP4HIRBitsType},
            {"p4hir_ref", parseP4HIRReferenceType}};

    EgglogCustomDefs funcs = {attrStringifiers, attrParsers, typeStringifiers, typeParsers};
    mlir::PassRegistration<EqualitySaturationPass>([&] {
        return createEqualitySaturationPass(mlirFile, eggFile, funcs); 
    });

    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "TANGRAM optimizer driver\n", registry));
}
