#ifndef P4HIR_ADAPTIVE_PARTITIONING_PASS_H
#define P4HIR_ADAPTIVE_PARTITIONING_PASS_H

#include <unordered_set>
#include <stack>

#include "mlir/Pass/Pass.h"
#include "mlir/IR/PatternMatch.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/Support/GraphWriter.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/PassManager.h"

namespace mlir {

#include <vector>
#include <algorithm>
#include <string>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
// #include <llvm/Support/raw_fd_ostream.h>
#include <system_error>

// Forward declaration for Operation
class Operation;

class CFGBlock {
public:
    llvm::SmallVector<Operation*> ops;
    unsigned blockID;

    class AdjacentBlock {
    public:
        enum Kind {
            AB_Normal,
            AB_Default,
            AB_Alternate,
        };

    private:
        CFGBlock *block;
        Kind kind;

    public:
        AdjacentBlock(CFGBlock *B, Kind K = AB_Normal)
            : block(B), kind(K) {}

        CFGBlock* getBlock() const { return block; }
        Kind getKind() const { return kind; }

        // Comparison operators for searching/removing
        bool operator==(const AdjacentBlock &other) const {
            return block == other.block && kind == other.kind;
        }
        bool operator==(const CFGBlock* otherBlock) const {
            return block == otherBlock;
        }
        operator CFGBlock*() const { return block; }
    };

    llvm::SmallVector<AdjacentBlock> preds;
    llvm::SmallVector<AdjacentBlock> succs;

    CFGBlock(unsigned id, const llvm::SmallVector<Operation*>& opsVec = {})
        : ops(opsVec), blockID(id) {}

    llvm::SmallVector<Operation*> getOperations() {
        return ops;
    }

    void addOperation(Operation* operation) {
        ops.push_back(operation);
    }

    auto pred_begin() { return preds.begin(); }
    auto pred_end()   { return preds.end(); }
    auto pred_begin() const { return preds.begin(); }
    auto pred_end()   const { return preds.end(); }

    auto succ_begin() { return succs.begin(); }
    auto succ_end()   { return succs.end(); }
    auto succ_begin() const { return succs.begin(); }
    auto succ_end()   const { return succs.end(); }

    void print(llvm::raw_ostream &os) const {
        os << "CFGBlock " << blockID << " {\n";

        // Print operations
        os << "  Operations:\n";
        if (ops.empty()) {
            os << "    (no operation)\n";
        } else {
            for (auto *op : ops) {
                os << "    ";
                op->print(os);
                os << "\n";
            }
        }

        // Print predecessor block IDs
        os << "  Predecessors: ";
        if (preds.empty()) {
            os << "(none)";
        } else {
            for (const auto &pred : preds) {
                if (pred.getBlock()) os << pred.getBlock()->blockID << " ";
            }
        }
        os << "\n";

        // Print successor block IDs
        os << "  Successors: ";
        if (succs.empty()) {
            os << "(none)";
        } else {
            for (const auto &succ : succs) {
                if (succ.getBlock()) os << succ.getBlock()->blockID << " ";
            }
        }
        os << "\n";
        os << "}\n";
    }
};

class ControlFlowGraph {
public:
    std::vector<std::unique_ptr<CFGBlock>> blocks;

    CFGBlock* createBlock(const llvm::SmallVector<Operation*>& opsVec = {}) {
        auto id = blocks.size();
        blocks.push_back(std::make_unique<CFGBlock>(id, opsVec));
        return blocks.back().get();
    }

    // Add successor/predecessor with AdjacentBlock kind
    void addSuccessor(CFGBlock *B, CFGBlock *S, CFGBlock::AdjacentBlock::Kind kind = CFGBlock::AdjacentBlock::AB_Normal) {
        B->succs.emplace_back(S, kind);
    }

    void addPredecessor(CFGBlock *B, CFGBlock *P, CFGBlock::AdjacentBlock::Kind kind = CFGBlock::AdjacentBlock::AB_Normal) {
        B->preds.emplace_back(P, kind);
    }

    // Remove successor by block pointer (removes all kinds of edges to S)
    void removeSuccessor(CFGBlock* B, CFGBlock *S) {
        B->succs.erase(
            std::remove_if(B->succs.begin(), B->succs.end(),
                [S](const CFGBlock::AdjacentBlock& adj) { return adj.getBlock() == S; }),
            B->succs.end());
    }

    void removePredecessor(CFGBlock* B, CFGBlock* P) {
        B->preds.erase(
            std::remove_if(B->preds.begin(), B->preds.end(),
                [P](const CFGBlock::AdjacentBlock& adj) { return adj.getBlock() == P; }),
            B->preds.end());
    }

    void exportDOT(llvm::raw_ostream &os) const {
        os << "digraph ControlFlowGraph {\n";
        os << "  node [shape=box, style=filled, fontname=\"Courier\"];\n";

        // Print nodes
        for (const auto &blkPtr : blocks) {
            const CFGBlock *blk = blkPtr.get();
            std::string label;
            llvm::raw_string_ostream labelStream(label);

            labelStream << "Block " << blk->blockID << "\\n";
            if (!blk->ops.empty()) {
                std::string opStr;
                {
                    llvm::raw_string_ostream opStream(opStr);
                    for (auto *op : blk->ops) {
                        op->print(opStream);
                        opStream << "\n";
                    }
                }

                std::string escapedStr;
                for (char c : opStr) {
                    if (c == '"') {
                        escapedStr += "'";     // Replace quotes with single quotes
                    } else if (c == '\n') {
                        escapedStr += "\\l";   // Use left-aligned line breaks in DOT
                    } else {
                        escapedStr += c;
                    }
                }
                
                labelStream << escapedStr << "\\l";
            } else {
                labelStream << "(no operation)\\l";
            }
            labelStream.flush();

            os << "  \"" << blk << "\" [label=\"" << label
            << "\", fillcolor=lightgray];\n";
        }

        // Print edges with kind as label (always show label)
        for (const auto &blkPtr : blocks) {
            const CFGBlock *blk = blkPtr.get();
            for (const auto &succ : blk->succs) {
                std::string edgeLabel;
                switch (succ.getKind()) {
                    case CFGBlock::AdjacentBlock::AB_Normal:
                        edgeLabel = ""; // Don't print label for normal
                        break;
                    case CFGBlock::AdjacentBlock::AB_Default:
                        edgeLabel = "default";
                        break;
                    case CFGBlock::AdjacentBlock::AB_Alternate:
                        edgeLabel = "alternate";
                        break;
                }
                os << "  \"" << blk << "\" -> \"" << succ.getBlock() << "\"";
                if (!edgeLabel.empty())
                    os << " [label=\"" << edgeLabel << "\"]";
                os << ";\n";
            }
        }

        os << "}\n";
    }

    void writeDOTFile(const std::string &filename) const {
        std::error_code EC;
        llvm::raw_fd_ostream file(filename, EC);
        if (EC) {
            llvm::errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
            return;
        }
        exportDOT(file);
    }
};

class CFGIterator {
public:
    using BlockPtr = CFGBlock*;

    CFGIterator(const ControlFlowGraph &cfg)
        : cfg_(cfg)
    {
        // Start from entry (block 0 if exists)
        if (!cfg_.blocks.empty()) {
            to_visit_.push(cfg_.blocks.front().get());
            visited_.insert(cfg_.blocks.front().get());
        }
        advance();
    }

    // Advance to next block in DFS order
    void advance() {
        if (to_visit_.empty()) {
            current_ = nullptr;
            return;
        }
        current_ = to_visit_.top();
        to_visit_.pop();

        // Push successors if not visited
        for (const auto &succAdj : current_->succs) {
            BlockPtr succ = succAdj.getBlock();
            if (succ && visited_.insert(succ).second) {
                to_visit_.push(succ);
            }
        }
    }

    // Dereference
    CFGBlock* operator*() const { return current_; }

    // Prefix increment
    CFGIterator& operator++() {
        advance();
        return *this;
    }

    // At end?
    bool atEnd() const { return current_ == nullptr; }

private:
    const ControlFlowGraph &cfg_;
    std::unordered_set<BlockPtr> visited_;
    std::stack<BlockPtr> to_visit_;
    BlockPtr current_ = nullptr;
};

enum class BlockKind { Datapath, Control };

struct BlockContext {
    BlockKind kind;
    std::string label;  // Optional human-readable label

    // Original P4HIR operations
    llvm::SmallVector<mlir::Operation*> orig_ops;

    llvm::SmallVector<BlockContext*> parents;
    llvm::SmallVector<BlockContext*> children;
    
    explicit BlockContext(BlockKind k) : kind(k) {}

    bool isEmpty() const { return orig_ops.empty(); }
    bool isControl() const { return kind == BlockKind::Control; }
    bool isDatapath() const { return kind == BlockKind::Datapath; }

    void addOrigOp(mlir::Operation* operation) {
        if (operation)
            orig_ops.push_back(operation);
    }

    // Updated to use children instead of nexts
    void addChild(BlockContext* child) {
        if (child && std::find(children.begin(), children.end(), child) == children.end()) {
            children.push_back(child);
            child->parents.push_back(this);
        }
    }
    
    // Get a descriptive name for this block
    std::string getName() const {
        if (!label.empty()) return label;
        return std::string(isControl() ? "Control" : "Datapath") + "_" + std::to_string(reinterpret_cast<uintptr_t>(this));
    }
};

struct AnalysisEnv {
    llvm::SmallVector<std::unique_ptr<BlockContext>> nodes;
    llvm::SmallVector<BlockContext*> stack;

    Operation* lastOpInRegionBlock = nullptr;

    int indentLevel = 0;

    void pushIndent() { indentLevel++; }
    void popIndent()  { if (indentLevel > 0) indentLevel--; }

    void printIndent() {
        for (int i = 0; i < indentLevel; i++)
            llvm::outs() << "  "; // 2 spaces per indent level
    }

    // Trace entry (increments indent afterwards)
    void traceEnter(const std::string &label) {
        printIndent();
        llvm::outs() << "▶ " << label << "\n";
        pushIndent();
    }

    // Trace exit (decrements indent first)
    void traceExit(const std::string &label) {
        popIndent();
        printIndent();
        llvm::outs() << "◀ " << label << "\n";
    }

    // Trace a single line (no indent change)
    void traceLine(const std::string &msg) {
        printIndent();
        llvm::outs() << msg << "\n";
    }

    BlockContext* createNode(BlockKind kind, const std::string& label = "") {
        nodes.push_back(std::make_unique<BlockContext>(kind));
        if (!label.empty()) {
            nodes.back()->label = label;
        }
        return nodes.back().get();
    }

    // Helper function to find leaf nodes
    std::vector<BlockContext*> findLeafNodes(BlockContext* root) {
        std::vector<BlockContext*> leaves;
        std::function<void(BlockContext*)> traverse = [&](BlockContext* node) {
            if (node->children.empty()) {
                leaves.push_back(node);
            } else {
                for (auto* child : node->children) {
                    traverse(child);
                }
            }
        };
        
        traverse(root);
        return leaves;
    }

    // Add explicit edge between two nodes
    void addEdge(BlockContext* parent, BlockContext* child) {
        if (parent && child) {
            parent->addChild(child);
        }
    }

    BlockContext* pushNode(BlockKind kind, const std::string& label = "") {
        BlockContext* node = createNode(kind, label);
        if (!stack.empty()) {
            // Find all leaf nodes of the current top block
            auto leafNodes = findLeafNodes(stack.back());
            
            // Connect all leaf nodes to the new node
            for (auto* leaf : leafNodes) {
                leaf->addChild(node);
            }
        }
        stack.push_back(node);
        return node;
    }

    void addOpToCurrent(mlir::Operation* op) {
        if (stack.empty())
            pushNode(BlockKind::Datapath);
        stack.back()->addOrigOp(op);
    }

    void addOpToBlock(mlir::Operation* op, BlockContext* block) {
        if (block) {
            block->addOrigOp(op);
        } else {
            llvm::errs() << "Error: Attempted to add operation to a null block context.\n";
        }
    }

    BlockContext* popNode() {
        if (stack.empty()) return nullptr;
        BlockContext* node = stack.back();
        stack.pop_back();
        return node;
    }

    BlockContext* current() {
        return stack.empty() ? nullptr : stack.back();
    }

    void printGraph() {
        llvm::outs() << "\n=== BF3 Block Context Graph ===\n";
        llvm::outs() << "Total nodes: " << nodes.size() << "\n";
        
        for (const auto &nodePtr : nodes) {
            const BlockContext *node = nodePtr.get();
            llvm::outs() << "BF3Block " << node->getName()
                        << " (" << (node->isControl() ? "Control" : "Datapath")
                        << ") with " << node->orig_ops.size() << " orig ops\n";

            // Print original P4HIR ops (abbreviated)
            for (size_t i = 0; i < node->orig_ops.size(); ++i) {
                auto *op = node->orig_ops[i];
                if (op) {
                    llvm::outs() << "  - " << op->getName() << "\n";
                } else {
                    llvm::outs() << "  - (null op)\n";
                }
            }

            // Print children (updated from nexts)
            if (!node->children.empty()) {
                llvm::outs() << "  -> Children: ";
                for (auto *child : node->children) {
                    llvm::outs() << child->getName() << " ";
                }
                llvm::outs() << "\n";
            }
            
            // Print parents
            if (!node->parents.empty()) {
                llvm::outs() << "  <- Parents: ";
                for (auto *parent : node->parents) {
                    llvm::outs() << parent->getName() << " ";
                }
                llvm::outs() << "\n";
            }
        }
    }

    void exportGraphDOT(llvm::raw_ostream &os) {
        os << "digraph BF3Pipeline {\n";
        os << "  node [shape=box, style=filled, fontname=\"Helvetica\"];\n";

        // Print all BF3 blocks
        for (const auto &nodePtr : nodes) {
            const BlockContext *node = nodePtr.get();
            std::string label;
            llvm::raw_string_ostream labelStream(label);

            // Block type and name
            labelStream << (node->isControl() ? "Control" : "Datapath");
            if (!node->label.empty()) {
                labelStream << "\\n(" << node->label << ")";
            }
            labelStream << "\\n";

            // Print each MLIR op on its own line, with arrows between ops
            for (size_t i = 0; i < node->orig_ops.size(); ++i) {
                auto *op = node->orig_ops[i];
                if (op) {
                    std::string opStr;
                    {
                        llvm::raw_string_ostream opStream(opStr);
                        // Use generic, local-scope printing for compact one-liners
                        op->print(opStream, mlir::OpPrintingFlags().useLocalScope().printGenericOpForm());
                    }
                    // Escape chars for DOT and prettify
                    std::string escapedStr;
                    for (char c : opStr) {
                        if (c == '"') {
                            escapedStr += "'"; // Replace quotes with single quotes
                        } else if (c == '\n' || c == '\r') {
                            escapedStr += "\\l";
                        } else {
                            escapedStr += c;
                        }
                    }
                    labelStream << escapedStr;
                    // Add turn arrow if not last op
                    if (i < node->orig_ops.size() - 1) {
                        labelStream << " \\u2192 "; // Unicode arrow for clarity
                    }
                    labelStream << "\\l"; // Left-align after each op
                } else {
                    labelStream << "(null op)\\l";
                }
            }
            labelStream.flush();

            // Color per BF3 block type
            std::string color = node->isControl() ? "lightblue" : "lightgreen";
            os << "  \"" << node << "\" [label=\"" << label << "\", fillcolor=\"" << color << "\"];\n";
        }

        // Print edges using children relationship
        for (const auto &nodePtr : nodes) {
            const BlockContext *node = nodePtr.get();
            for (auto *child : node->children) {
                os << "  \"" << node << "\" -> \"" << child << "\";\n";
            }
        }

        os << "}\n";
    }

    void writeDOTFile(const std::string &filename) {
        std::error_code EC;
        llvm::raw_fd_ostream file(filename, EC);
        if (EC) {
            llvm::errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
            return;
        }
        exportGraphDOT(file);
    }
    
    // Find all root nodes (nodes with no parents)
    std::vector<BlockContext*> getRootNodes() {
        std::vector<BlockContext*> roots;
        for (const auto& nodePtr : nodes) {
            if (nodePtr->parents.empty()) {
                roots.push_back(nodePtr.get());
            }
        }
        return roots;
    }
    
    // Find all leaf nodes (nodes with no children)
    std::vector<BlockContext*> getLeafNodes() {
        std::vector<BlockContext*> leaves;
        for (const auto& nodePtr : nodes) {
            if (nodePtr->children.empty()) {
                leaves.push_back(nodePtr.get());
            }
        }
        return leaves;
    }
};

class P4HIRAdaptivePartitioningPass 
    : public PassWrapper<P4HIRAdaptivePartitioningPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(P4HIRAdaptivePartitioningPass)
  
  StringRef getArgument() const final { return "p4hir-adaptive-partition"; }
  StringRef getDescription() const final {
    return "Traverse and analyze P4HIR control operations";
  }

  void runOnOperation() override;
  void getDependentDialects(DialectRegistry& registry) const override;

private:
    std::string egglogInputFile = "/local/yihan/tangram-compiler/temp/intermediate.mlir";

    // mlir::OpPassManager createEgglogPM(mlir::Operation *op, const std::string &mlirFile);

  // P4HIR traversal result
  struct P4HIRTraversalResult {
    SmallVector<Operation*> drmtCandidates;
    SmallVector<Operation*> dpaCandidates;
    SmallVector<Operation*> unknownOps;
    
    // Analysis counters
    int tableOps = 0;
    int controlFlowOps = 0;
    int dataOps = 0;
    int actionOps = 0;
  };
  
  // Main traversal methods
#if 0
    SmallVector<Operation*> findP4HIRControls(ModuleOp module);
#endif
    Operation& findLastP4HIRControl(ModuleOp module);
    void analyzeP4HIRControl(Operation* controlOp);
    void traverseP4HIRPipeline(Operation* controlOp, Operation* controlApplyOp, P4HIRTraversalResult& result);

    void analyzeTableActionsOp(struct AnalysisEnv* env, Operation* op, P4HIRTraversalResult& result, BlockContext* keyBlock);
    void analyzeTableOp(struct AnalysisEnv *env, Operation* op, P4HIRTraversalResult& result);
    void traverseRegion(AnalysisEnv* env, Operation* regionOp, P4HIRTraversalResult& result);
    void traverseOperation(AnalysisEnv* env, Operation* op, P4HIRTraversalResult& result);

  // Target assignment (simplified)
  void assignToTarget(Operation* op, P4HIRTraversalResult& result);
  
  // Reporting
  void reportP4HIRResults(const P4HIRTraversalResult& result, StringRef controlName);
};

std::unique_ptr<Pass> createP4HIRAdaptivePartitioningPass();

} // namespace mlir


namespace llvm {

// === Single BF3BlockContext GraphTraits ===
template <>
struct GraphTraits<mlir::BlockContext*> {
    using NodeRef = mlir::BlockContext*;
    using ChildIteratorType = llvm::SmallVectorImpl<mlir::BlockContext*>::iterator;

    static NodeRef getEntryNode(mlir::BlockContext* N) { return N; }
    static ChildIteratorType child_begin(mlir::BlockContext* N) { return N->children.begin(); }
    static ChildIteratorType child_end(mlir::BlockContext* N)   { return N->children.end(); }
};

// === Whole Graph GraphTraits (works on AnalysisEnv*) ===
template <>
struct GraphTraits<mlir::AnalysisEnv*> : public GraphTraits<mlir::BlockContext*> {
    using nodes_iterator = llvm::mapped_iterator<
        decltype(mlir::AnalysisEnv::nodes.begin()),
        std::function<mlir::BlockContext*(const std::unique_ptr<mlir::BlockContext>&)>>;

    static mlir::BlockContext* getEntryNode(mlir::AnalysisEnv* G) {
        return G->nodes.empty() ? nullptr : G->nodes.front().get();
    }

    static nodes_iterator nodes_begin(mlir::AnalysisEnv* G) {
        auto mapper = [](const std::unique_ptr<mlir::BlockContext>& ptr) -> mlir::BlockContext* {
            return ptr.get();
        };
        return nodes_iterator(G->nodes.begin(), mapper);
    }

    static nodes_iterator nodes_end(mlir::AnalysisEnv* G) {
        auto mapper = [](const std::unique_ptr<mlir::BlockContext>& ptr) -> mlir::BlockContext* {
            return ptr.get();
        };
        return nodes_iterator(G->nodes.end(), mapper);
    }

    static size_t size(mlir::AnalysisEnv* G) { return G->nodes.size(); }
};

// === DOTGraphTraits for pretty Graphviz output ===
template <>
struct DOTGraphTraits<mlir::AnalysisEnv*> : public DefaultDOTGraphTraits {
    DOTGraphTraits(bool isSimple = false)
        : DefaultDOTGraphTraits(isSimple) {}

    static std::string getNodeLabel(mlir::BlockContext* N, const mlir::AnalysisEnv*) {
        std::string label;
        if (N->isControl())
            label += "[BF3-CTRL] ";
        else
            label += "[BF3-DATA] ";

        // Append original P4HIR op names
        label += "\\nOrig ops:\\n";
        bool first = true;
        for (auto *op : N->orig_ops) {
            if (!first) label += "\\n";
            if (op) {
                label += op->getName().getStringRef().str();
            } else {
                label += "(null op)";
            }
            first = false;
        }

        return label.empty() ? "(empty)" : label;
    }

    static std::string getNodeAttributes(mlir::BlockContext* N, const mlir::AnalysisEnv*) {
        std::string color = N->isControl() ? "lightblue" : "lightgreen";
        return "fillcolor=\"" + color + "\", style=filled";
    }

    static std::string getGraphName() {
        return "BF3Pipeline";
    }
};

} // namespace llvm

#endif // P4HIR_ADAPTIVE_PARTITIONING_PASS_H