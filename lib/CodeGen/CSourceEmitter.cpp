#include "tangram/CodeGen/CSourceEmitter.h"
#include "mlir/IR/BuiltinTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include <fstream>

using namespace mlir;
using namespace mlir::bf3drmt;

CSourceEmitter::CSourceEmitter() {
  // Add standard includes
  ensureInclude("stdint.h");
  ensureInclude("stdlib.h");
}

void CSourceEmitter::emitFunctionDecl(llvm::StringRef name, 
                                      llvm::ArrayRef<Type> args, 
                                      Type result) {
  std::string decl = getTypeName(result) + " " + name.str() + "(";
  
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) decl += ", ";
    decl += getTypeName(args[i]) + " arg" + std::to_string(i);
  }
  
  decl += ")";
  addLine(decl + " {");
  pushScope();
}

void CSourceEmitter::emitVariableDecl(llvm::StringRef name, Type type) {
  emitVariableDecl(name, getTypeName(type));
}

void CSourceEmitter::emitVariableDecl(llvm::StringRef name, llvm::StringRef typeName) {
  std::string nameStr = name.str();
  if (declaredVars.find(nameStr) != declaredVars.end()) {
    return; // Already declared
  }
  
  std::string decl = typeName.str() + " " + nameStr + ";";
  addLine(decl);
  declaredVars.insert(nameStr);
}

void CSourceEmitter::emitStatement(llvm::StringRef statement) {
  addLine(statement.str());
}

void CSourceEmitter::emitComment(llvm::StringRef comment) {
  addLine("// " + comment.str());
}

void CSourceEmitter::pushScope() {
  indentLevel++;
}

void CSourceEmitter::popScope() {
  indentLevel--;
  addLine("}");
}

std::string CSourceEmitter::getTypeName(Type type) {
  if (auto intType = type.dyn_cast<IntegerType>()) {
    unsigned width = intType.getWidth();
    if (width == 1) return "bool";
    if (width == 8) return "uint8_t";
    if (width == 16) return "uint16_t";
    if (width == 32) return "uint32_t";
    if (width == 64) return "uint64_t";
    return "uint32_t"; // Default fallback
  }
  
  if (type.isIndex()) {
    return "size_t";
  }
  
  return "int"; // Default fallback
}

std::string CSourceEmitter::getDefaultValue(Type type) {
  if (auto intType = type.dyn_cast<IntegerType>()) {
    return "0";
  }
  return "0";
}

void CSourceEmitter::writeToFile(llvm::StringRef path) {
  std::error_code EC;
  llvm::raw_fd_ostream file(path, EC);
  if (EC) {
    llvm::errs() << "Error opening file " << path << ": " << EC.message() << "\n";
    return;
  }
  
  // Write includes
  for (const auto& include : includes) {
    file << "#include <" << include << ">\n";
  }
  file << "\n";
  
  // Write source
  file << source;
  file.close();
}

void CSourceEmitter::indent() {
  for (int i = 0; i < indentLevel; ++i) {
    source += "    "; // 4 spaces per indent level
  }
}

void CSourceEmitter::addLine(llvm::StringRef line) {
  indent();
  source += line.str() + "\n";
}

void CSourceEmitter::addInclude(llvm::StringRef include) {
  includes.push_back(include.str());
}

void CSourceEmitter::ensureInclude(llvm::StringRef include) {
  std::string includeStr = include.str();
  if (std::find(includes.begin(), includes.end(), includeStr) == includes.end()) {
    includes.push_back(includeStr);
  }
}

void CSourceEmitter::addBlankLine() {
  source += "\n";
}