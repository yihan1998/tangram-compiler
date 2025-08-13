#include "tangram/CodeGen/BF3DRMTToCCodegen.h"
#include "tangram/CodeGen/CSourceEmitter.h"
#include "tangram/Dialect/BF3DRMT/BF3DRMTOps.h"
#include "tangram/Dialect/BF3DRMT/BF3DRMTDialect.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "bf3drmt-to-c-codegen"

using namespace mlir;
using namespace mlir::bf3drmt;

namespace {

#define GEN_PASS_DEF_BF3DRMTTOCCODEGENPASS
#include "tangram/CodeGen/Passes.h.inc"

struct BF3DRMTToCCodegenPass
    : public impl::BF3DRMTToCCodegenPassBase<BF3DRMTToCCodegenPass> {
  
  using impl::BF3DRMTToCCodegenPassBase<BF3DRMTToCCodegenPass>::BF3DRMTToCCodegenPassBase;
  
  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();
    
    // Set up source emitter
    CSourceEmitter emitter;
    
    // Process each function
    moduleOp.walk([&](func::FuncOp funcOp) {
      processFunctionOp(funcOp, emitter);
    });
    
    // Write output files
    emitter.writeToFile(outputPath);
  }
  
private:
  void processFunctionOp(func::FuncOp funcOp, CSourceEmitter &emitter) {
    // Get function signature
    auto funcType = funcOp.getFunctionType();
    
    emitter.emitComment("Function: " + funcOp.getName().str());
    emitter.emitFunctionDecl(funcOp.getName(), funcType.getInputs(), 
                             funcType.getResults().empty() ? 
                             IntegerType::get(&getContext(), 32) : funcType.getResults()[0]);
    
    // Declare local variables for function arguments
    for (auto arg : funcOp.getArguments()) {
      std::string argName = "arg" + std::to_string(arg.getArgNumber());
      valueNames[arg] = argName;
    }
    
    // Process function body
    for (auto &block : funcOp.getBody()) {
      for (auto &op : block) {
        processOperation(&op, emitter);
      }
    }
    
    // Add return statement if needed
    if (funcType.getResults().empty()) {
      emitter.emitStatement("return 0;");
    }
    
    emitter.popScope();
    emitter.addBlankLine();
  }
  
  void processOperation(Operation *op, CSourceEmitter &emitter) {
    if (auto constantOp = dyn_cast<ConstantOp>(op)) {
      handleConstantOp(constantOp, emitter);
    } else if (auto addOp = dyn_cast<AddIOp>(op)) {
      handleAddIOp(addOp, emitter);
    } else if (auto subOp = dyn_cast<SubIOp>(op)) {
      handleSubIOp(subOp, emitter);
    } else if (auto shlOp = dyn_cast<ShLIOp>(op)) {
      handleShLIOp(shlOp, emitter);
    } else if (auto shrOp = dyn_cast<ShrUIOp>(op)) {
      handleShrUIOp(shrOp, emitter);
    } else if (auto returnOp = dyn_cast<func::ReturnOp>(op)) {
      // Handle return operation
      if (returnOp.getNumOperands() > 0) {
        std::string returnValue = getValueName(returnOp.getOperand(0));
        emitter.emitStatement("return " + returnValue + ";");
      } else {
        emitter.emitStatement("return 0;");
      }
    } else {
      emitter.emitComment("Unhandled operation: " + op->getName().getStringRef().str());
    }
  }
  
  void handleConstantOp(ConstantOp op, CSourceEmitter &emitter) {
    std::string resultName = getTemporaryName();
    valueNames[op.getResult()] = resultName;
    
    // Extract the constant value
    std::string value;
    if (auto intAttr = op.getValue().dyn_cast<IntegerAttr>()) {
      value = std::to_string(intAttr.getInt());
    } else {
      value = "0"; // Default fallback
    }
    
    emitter.emitVariableDecl(resultName, op.getResult().getType());
    emitter.emitStatement(resultName + " = " + value + ";");
  }

  void handleAddIOp(AddIOp op, CSourceEmitter &emitter) {
    std::string resultName = getTemporaryName();
    valueNames[op.getResult()] = resultName;
    
    std::string lhs = getValueName(op.getLhs());
    std::string rhs = getValueName(op.getRhs());
    
    emitter.emitVariableDecl(resultName, op.getResult().getType());
    emitter.emitStatement(resultName + " = " + lhs + " + " + rhs + ";");
  }

  void handleSubIOp(SubIOp op, CSourceEmitter &emitter) {
    std::string resultName = getTemporaryName();
    valueNames[op.getResult()] = resultName;
    
    std::string lhs = getValueName(op.getLhs());
    std::string rhs = getValueName(op.getRhs());
    
    emitter.emitVariableDecl(resultName, op.getResult().getType());
    emitter.emitStatement(resultName + " = " + lhs + " - " + rhs + ";");
  }

  void handleShLIOp(ShLIOp op, CSourceEmitter &emitter) {
    std::string resultName = getTemporaryName();
    valueNames[op.getResult()] = resultName;
    
    std::string lhs = getValueName(op.getLhs());
    std::string rhs = getValueName(op.getRhs());
    
    emitter.emitVariableDecl(resultName, op.getResult().getType());
    emitter.emitStatement(resultName + " = " + lhs + " << " + rhs + ";");
  }

  void handleShrUIOp(ShrUIOp op, CSourceEmitter &emitter) {
    std::string resultName = getTemporaryName();
    valueNames[op.getResult()] = resultName;
    
    std::string lhs = getValueName(op.getLhs());
    std::string rhs = getValueName(op.getRhs());
    
    emitter.emitVariableDecl(resultName, op.getResult().getType());
    emitter.emitStatement(resultName + " = " + lhs + " >> " + rhs + ";");
  }
  
  std::string getValueName(Value value) {
    auto it = valueNames.find(value);
    if (it != valueNames.end()) {
      return it->second;
    }
    
    // Generate a new name
    std::string name = "temp" + std::to_string(tempCounter++);
    valueNames[value] = name;
    return name;
  }

  std::string getTemporaryName() {
    return "temp" + std::to_string(tempCounter++);
  }
  
  // State
  int tempCounter = 0;
  llvm::DenseMap<Value, std::string> valueNames;
};

} // namespace

std::unique_ptr<Pass> mlir::bf3drmt::createBF3DRMTToCCodegenPass() {
  return std::make_unique<BF3DRMTToCCodegenPass>();
}