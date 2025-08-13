#ifndef TANGRAM_CODEGEN_CSOURCEEMITTER_H
#define TANGRAM_CODEGEN_CSOURCEEMITTER_H

#include "mlir/IR/Operation.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ArrayRef.h"
#include <string>
#include <vector>
#include <set>

namespace mlir {
namespace bf3drmt {

class CSourceEmitter {
public:
  CSourceEmitter();
  
  // Main emission methods
  void emitFunctionDecl(llvm::StringRef name, llvm::ArrayRef<Type> args, Type result);
  void emitOperation(Operation *op);
  void emitVariableDecl(llvm::StringRef name, Type type);
  void emitVariableDecl(llvm::StringRef name, llvm::StringRef typeName);
  
  // Statement emission
  void emitStatement(llvm::StringRef statement);
  void emitComment(llvm::StringRef comment);
  
  // Control flow
  void pushScope();
  void popScope();
  
  // Type conversion
  std::string getTypeName(Type type);
  std::string getDefaultValue(Type type);
  
  // File management
  void writeToFile(llvm::StringRef path);
  std::string getSource() const { return source; }
  
  // Utility methods
  void addBlankLine();
  
private:
  // Internal state
  std::string source;
  std::vector<std::string> includes;
  std::set<std::string> declaredVars;
  int indentLevel = 0;
  
  void indent();
  void addLine(llvm::StringRef line);
  void addInclude(llvm::StringRef include);
  void ensureInclude(llvm::StringRef include);
};

} // namespace bf3drmt
} // namespace mlir

#endif // TANGRAM_CODEGEN_CSOURCEEMITTER_H