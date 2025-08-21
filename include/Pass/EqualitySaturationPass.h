#ifndef EQUALITYSATURATIONPASS_H
#define EQUALITYSATURATIONPASS_H

#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>
#include <chrono>

#include "llvm/Support/FileSystem.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"
#include "Dialect/P4HIR/P4HIR_Ops.h"

#include "Pass/Egglog.h"
#include "Pass/Utils.h"
#if 1
// struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass, mlir::OperationPass<mlir::func::FuncOp>> {
// struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass, mlir::OperationPass<mlir::LLVM::LLVMFuncOp>> {
// struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass, mlir::OperationPass<P4::P4MLIR::P4HIR::FuncOp>> {
struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass, mlir::OperationPass<P4::P4MLIR::P4HIR::ControlOp>> {
// struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass, mlir::OperationPass<P4::P4MLIR::P4HIR::TableKeyOp>> {
    std::string mlirFilePath;
    std::string eggFilePath;

    std::string egglogExtractedFuncFilename = "egglog-func-extract.txt";
    std::string egglogExtractedFilename = "egglog-extract.txt";
    std::string egglogLogFilename = "egglog-log.txt";
    
    EgglogCustomDefs customFunctions;

    std::map<std::string, EgglogOpDef> supportedOps;
    std::set<std::string> supportedDialects;

    double mlirToEgglogTime = 0.0;
    double egglogExecTime = 0.0;
    double egglogToMlirTime = 0.0;

    EqualitySaturationPass(const std::string&, const std::string&, const EgglogCustomDefs&);

    mlir::StringRef getArgument() const override { return "eq-sat"; }
    mlir::StringRef getDescription() const override { return "Performs equality saturation on each block in the given file. The language definition is egglog."; }
    
    void init();
    void runOnOperation() override;
    // void convertRootOpToBf3drmt(P4::P4MLIR::P4HIR::ControlOp oldControlOp);
    void runOnBlock(mlir::Block& block, const std::string& blockName);
    // void runOnFunction(P4::P4MLIR::P4HIR::ControlOp& funcOp);
    void runOnFunction(mlir::LLVM::LLVMFuncOp& funcOp);
    // void runEgglog(const std::vector<EggifiedOp*>& block, const std::string& blockName);
    void runEgglog(const std::vector<EggifiedOp*>& block, const std::vector<EggifiedOp*>& rootBlock, const std::string& blockName);
};

std::unique_ptr<mlir::Pass> createEqualitySaturationPass(const std::string&, const std::string&, const EgglogCustomDefs&);
#else
template<typename OpType>
struct EqualitySaturationPass : public mlir::PassWrapper<EqualitySaturationPass<OpType>, mlir::OperationPass<OpType>> {
    std::string mlirFilePath;
    std::string eggFilePath;

    std::string egglogExtractedFuncFilename = "egglog-func-extract.txt";
    std::string egglogExtractedFilename = "egglog-extract.txt";
    std::string egglogLogFilename = "egglog-log.txt";
    
    EgglogCustomDefs customFunctions;

    std::map<std::string, EgglogOpDef> supportedOps;
    std::set<std::string> supportedDialects;

    double mlirToEgglogTime = 0.0;
    double egglogExecTime = 0.0;
    double egglogToMlirTime = 0.0;

    EqualitySaturationPass(const std::string& mlirFile, const std::string& eggFile, const EgglogCustomDefs& funcs)
        : mlirFilePath(mlirFile), eggFilePath(eggFile), customFunctions(funcs) {}

    mlir::StringRef getArgument() const override { 
        return getPassArgument(); 
    }
    
    mlir::StringRef getDescription() const override { 
        return "Performs equality saturation on each block in the given file. The language definition is egglog."; 
    }
    
    void init() {
        // Make sure both files exist
        if (!llvm::sys::fs::exists(mlirFilePath)) {
            llvm::errs() << "MLIR file does not exist: " << mlirFilePath << "\n";
            exit(1);
        }
        if (!llvm::sys::fs::exists(eggFilePath)) {
            llvm::errs() << "Egg file does not exist: " << eggFilePath << "\n";
            exit(1);
        }

        // mlirFilePath without extension
        std::string name = mlirFilePath.substr(0, mlirFilePath.find(".mlir"));
        egglogExtractedFuncFilename = name + "-egglog-func-extract.log";
        egglogExtractedFilename = name + "-egglog-extract.log";
        egglogLogFilename = name + "-egglog.log";
        
        std::ifstream opFile(eggFilePath);
        std::string line;

        while (std::getline(opFile, line)) {
            llvm::outs() << "Parsing line: " << line << "\n";
            if (EgglogOpDef::isOpFunction(line)) {
                EgglogOpDef parsedOp = EgglogOpDef::parseOpFunction(line);

                supportedOps.emplace(parsedOp.dialect + "." + parsedOp.name + (parsedOp.version.empty() ? "" : "." + parsedOp.version), parsedOp);
                supportedOps.emplace(parsedOp.dialect + "_" + parsedOp.name + (parsedOp.version.empty() ? "" : "_" + parsedOp.version), parsedOp);
                supportedDialects.insert(parsedOp.dialect);
            }
        }

        opFile.close();

        // dump
        llvm::outs() << "Supported ops: ";
        for (const auto& [op, _]: supportedOps) {
            llvm::outs() << op << ", ";
        }

        llvm::outs() << "\nSupported dialects: ";
        for (const std::string& dialect: supportedDialects) {
            llvm::outs() << dialect << ", ";
        }

        llvm::outs() << "\n\n";
    }
    
    void runOnOperation() override {
        init();

        OpType rootOp = this->getOperation();
        llvm::StringRef rootOpName = getOperationName(rootOp);

        llvm::outs() << "Running on operation: " << rootOpName << "\n";
        llvm::outs() << "-----------------------------------------\n";

        // Handle operations with regions differently
        if constexpr (hasRegion<OpType>()) {
            // Perform equality saturation on all operations of each block
            for (mlir::Block& block: rootOp.getRegion().getBlocks()) {
                std::string parentOpName = block.getParentOp()->getName().getStringRef().str();
                std::string blockName = rootOpName.str() + "_" + parentOpName;
                runOnBlock(block, blockName);

                // Temporary dead code elimination
                bool clean = false;
                while (!clean) {
                    clean = true;
                    
                    block.walk([&](mlir::Operation* op) {
                        if (mlir::isOpTriviallyDead(op)) {
                            clean = false;
                            op->erase();
                        }
                    });
                }
            }
        } else {
            // For operations without regions, process the operation itself
            // This might need customization based on your specific needs
            llvm::outs() << "Processing operation without regions\n";
        }

        llvm::outs() << "-----------------------------------------\n";
        llvm::outs() << "Done running on operation: " << rootOpName << "\n";
        llvm::outs() << "mlirToEgglogTime = " << mlirToEgglogTime << "s\n";
        llvm::outs() << "egglogExecTime = " << egglogExecTime << "s\n";
        llvm::outs() << "egglogToMlirTime = " << egglogToMlirTime << "s\n";
        llvm::outs() << "-----------------------------------------\n";
    }

    void runOnBlock(mlir::Block& block, const std::string& blockName) {
        llvm::outs() << "Running on block: " << blockName << "\n";
        auto start = std::chrono::high_resolution_clock::now();

        // Eggify the block
        mlir::MLIRContext& context = this->getContext();
        Egglog egglog(context, customFunctions, supportedOps);

        // register all block arguments
        for (mlir::Value value: block.getArguments()) {
            EggifiedOp* eggifiedValue = egglog.eggifyValue(value);
            llvm::outs() << "=> Eggify block arguments\n";
            eggifiedValue->print(llvm::outs());
        }

        for (mlir::Operation& op: block.getOperations()) {
            EggifiedOp* eggifiedOp = egglog.eggifyOperation(&op);
            llvm::outs() << "=> Eggify block operations\n";
            eggifiedOp->print(llvm::outs());
        }

        auto end = std::chrono::high_resolution_clock::now();
        mlirToEgglogTime += std::chrono::duration<double>(end - start).count();

        // dump all ops
        for (const EggifiedOp* eggOp: egglog.eggifiedBlock) {
            eggOp->print(llvm::outs());
        }

        runEgglog(egglog.eggifiedBlock, blockName);

        start = std::chrono::high_resolution_clock::now();

        std::ifstream file(egglogExtractedFilename);

        // Parse the extracted egglog file and replace the MLIR operations
        for (const EggifiedOp* eggOp: egglog.eggifiedBlock) {
            llvm::outs() << "=> Extract EggifiedOp: " << eggOp->mlirOp << "\n";
            if (!eggOp->shouldBeExtracted()) {
                continue;
            }

            std::string line;
            std::getline(file, line);

            mlir::Operation* prevOp = eggOp->mlirOp;
            mlir::OpBuilder builder(prevOp);
            mlir::Operation* newOp = egglog.parseOperation(line, builder);

            if(newOp == nullptr) { // If the operation is an opaque value, replace it with the value
                mlir::Value value = egglog.parseValue(line);
                prevOp->getResult(0).replaceAllUsesWith(value);
                llvm::outs() << "=> Erasing prev op " << prevOp->dump() << " with " << value;
                prevOp->erase();
            } else if (newOp != prevOp) { // Check if the whole operation is different, if so, replace it
                llvm::outs() << "=> Replacing prev op " << prevOp->dump() << " with " << newOp->dump();
                prevOp->replaceAllUsesWith(newOp);
                prevOp->erase();
            }
        }

        end = std::chrono::high_resolution_clock::now();
        egglogToMlirTime += std::chrono::duration<double>(end - start).count();

        // dump parsed ops cache
        for (const auto& [opStr, op]: egglog.parsedOps) {
            llvm::outs() << opStr << " : " << *op << "\n";
        }

        file.close();
        llvm::outs() << "\n";
    }

    void runEgglog(const std::vector<EggifiedOp*>& block, const std::string& blockName) {
        std::ifstream eggFile(eggFilePath);
        std::vector<std::string> egglogLines;

        // Read the egglog file
        std::string opsTarget = ";; OPS HERE ;;";
        std::string extractsTarget = ";; EXTRACTS HERE ;;";

        bool insertedOps = false;
        bool insertedExtracts = false;

        auto start = std::chrono::high_resolution_clock::now();

        std::string line;
        while (std::getline(eggFile, line)) {
            egglogLines.push_back(line);

            if (!insertedOps && line == opsTarget) {
                egglogLines.push_back("; " + blockName);
                for (const EggifiedOp* op: block) {  // Insert the operations
                    egglogLines.push_back(op->egglogLet());
                }

                insertedOps = true;
            } else if (!insertedExtracts && line == extractsTarget) {
                for (const EggifiedOp* op: block) {  // Extract the results of the egglog run
                    if (op->shouldBeExtracted()) {
                        egglogLines.push_back("(extract " + op->getPrintId() + ")");
                    }
                }

                insertedExtracts = true;
            }
        }
        eggFile.close();

        // Write the extracted egglog to a new file with the same name and ext .ops.egg
        std::string opsEggFilePath = eggFilePath.substr(0, eggFilePath.find_last_of(".")) + ".ops.egg";
        std::ofstream eggFileOut(opsEggFilePath);
        for (const std::string& line: egglogLines) {
            eggFileOut << line << "\n";
        }
        eggFileOut.close();
        
        auto end = std::chrono::high_resolution_clock::now();
        mlirToEgglogTime += std::chrono::duration<double>(end - start).count();

        // Run egglog and extract the results
        std::string egglogCmd = "egglog " + opsEggFilePath + " > " + egglogExtractedFilename + " 2> " + egglogLogFilename;

        llvm::outs() << "\nRunning egglog: " << egglogCmd << "\n"
                     << "\n";

        start = std::chrono::high_resolution_clock::now();
        std::system(egglogCmd.c_str());
        end = std::chrono::high_resolution_clock::now();

        egglogExecTime += std::chrono::duration<double>(end - start).count();

        // dump output
        printFileContents(egglogLogFilename);
        printFileContents(egglogExtractedFilename);

        llvm::outs() << "\n\nDone running egglog\n"
                     << "\n";
    }

private:
    // Helper to check if operation type has regions
    template<typename T>
    static constexpr bool hasRegion() {
        if constexpr (std::is_same_v<T, mlir::func::FuncOp>) {
            return true;
        } else if constexpr (std::is_same_v<T, mlir::LLVM::LLVMFuncOp>) {
            return true;
        } else if constexpr (std::is_same_v<T, P4::P4MLIR::P4HIR::FuncOp>) {
            return true;
        } else if constexpr (std::is_same_v<T, P4::P4MLIR::P4HIR::ControlOp>) {
            return true;
        } else if constexpr (std::is_same_v<T, mlir::ModuleOp>) {
            return true;
        } else {
            return false;
        }
    }

    // Get operation name based on the operation type
    llvm::StringRef getOperationName(OpType& op) {
        if constexpr (std::is_same_v<OpType, mlir::func::FuncOp>) {
            return op.getName();
        } else if constexpr (std::is_same_v<OpType, mlir::LLVM::LLVMFuncOp>) {
            return op.getName();
        } else if constexpr (std::is_same_v<OpType, P4::P4MLIR::P4HIR::FuncOp>) {
            return op.getName();
        } else if constexpr (std::is_same_v<OpType, P4::P4MLIR::P4HIR::ControlOp>) {
            return op.getName();
        } else if constexpr (std::is_same_v<OpType, mlir::ModuleOp>) {
            return op.getName().value_or("unnamed_module");
        } else {
            // For other operation types, use the operation name
            return op->getName().getStringRef();
        }
    }

    // Get pass argument based on operation type
    mlir::StringRef getPassArgument() const {
        if constexpr (std::is_same_v<OpType, mlir::func::FuncOp>) {
            return "eq-sat-func";
        } else if constexpr (std::is_same_v<OpType, mlir::LLVM::LLVMFuncOp>) {
            return "eq-sat-llvm-func";
        } else if constexpr (std::is_same_v<OpType, P4::P4MLIR::P4HIR::FuncOp>) {
            return "eq-sat-p4hir-func";
        } else if constexpr (std::is_same_v<OpType, P4::P4MLIR::P4HIR::ControlOp>) {
            return "eq-sat";  // Keep original for backward compatibility
        } else if constexpr (std::is_same_v<OpType, mlir::ModuleOp>) {
            return "eq-sat-module";
        } else {
            return "eq-sat-generic";
        }
    }

    void printFileContents(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            llvm::outs() << line << "\n";
        }
        file.close();
    }
};

// Type aliases for commonly used passes
using FuncEqualitySaturationPass = EqualitySaturationPass<mlir::func::FuncOp>;
using LLVMFuncEqualitySaturationPass = EqualitySaturationPass<mlir::LLVM::LLVMFuncOp>;
using P4HIRFuncEqualitySaturationPass = EqualitySaturationPass<P4::P4MLIR::P4HIR::FuncOp>;
using P4HIRTableKeyEqualitySaturationPass = EqualitySaturationPass<P4::P4MLIR::P4HIR::TableKeyOp>;
using P4HIRControlEqualitySaturationPass = EqualitySaturationPass<P4::P4MLIR::P4HIR::ControlOp>;
using ModuleEqualitySaturationPass = EqualitySaturationPass<mlir::ModuleOp>;

// Factory functions
template<typename OpType>
std::unique_ptr<mlir::Pass> createEqualitySaturationPass(const std::string& mlirFile, const std::string& eggFile, const EgglogCustomDefs& funcs) {
    return std::make_unique<EqualitySaturationPass<OpType>>(mlirFile, eggFile, funcs);
}

std::unique_ptr<mlir::Pass> createOrigEqualitySaturationPass(const std::string& mlirFile, const std::string& eggFile, const EgglogCustomDefs& funcs);

std::unique_ptr<mlir::Pass> createP4HIRControlEqualitySaturationPass(const std::string& mlirFile, const std::string& eggFile, const EgglogCustomDefs& funcs);

std::unique_ptr<mlir::Pass> createP4HIRTableKeyEqualitySaturationPass(const std::string& mlirFile, const std::string& eggFile, const EgglogCustomDefs& funcs);
#endif

#endif  // EQUALITYSATURATIONPASS_H
