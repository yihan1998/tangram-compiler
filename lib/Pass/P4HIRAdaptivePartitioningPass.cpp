#include "Pass/P4HIRAdaptivePartitioningPass.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/Support/raw_ostream.h"

#include "Dialect/Bf3/Drmt/IR/Bf3DrmtAttrs.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtOps.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

#include "Dialect/P4HIR/P4HIR_Ops.h"
#include "Dialect/P4HIR/P4HIR_Dialect.h"

#include "Pass/EqualitySaturationPass.h"
#include "Pass/EgglogCustomDefs.h"

using namespace mlir;

#if 0
SmallVector<Operation*> P4HIRAdaptivePartitioningPass::findP4HIRControls(ModuleOp module) {
    SmallVector<Operation*> controls;
    
    llvm::outs() << "🔍 Scanning for P4HIR controls...\n";
    
    module.walk([&](Operation* op) {
        // Use proper dialect checking
        if (auto controlOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::ControlOp>(op)) {
            controls.push_back(controlOp);
            auto controlName = controlOp->getAttrOfType<StringAttr>("sym_name");
            llvm::outs() << "  📋 Found control: " 
                        << (controlName ? controlName.getValue() : "unnamed") << "\n";
        }
    });
    
    llvm::outs() << "✅ Total P4HIR controls found: " << controls.size() << "\n\n";
    return controls;
}
#endif

Operation& P4HIRAdaptivePartitioningPass::findLastP4HIRControl(ModuleOp module) {
    Operation* lastControl = nullptr;

    llvm::outs() << "🔍 Scanning for P4HIR controls (sequential iteration)...\n";

    // Sequential iteration over top-level ops in the module
    for (auto& op : module.getBody()->getOperations()) {
        if (auto controlOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::ControlOp>(&op)) {
            lastControl = controlOp;
            auto controlName = controlOp->getAttrOfType<StringAttr>("sym_name");
            llvm::outs() << "  📋 Found control: "
                         << (controlName ? controlName.getValue() : "unnamed") << "\n";
        }
    }

    if (lastControl)
        llvm::outs() << "✅ Last P4HIR control found: "
                     << (lastControl->getAttrOfType<StringAttr>("sym_name") ?
                         lastControl->getAttrOfType<StringAttr>("sym_name").getValue() : "unnamed")
                     << "\n\n";
    else
        llvm::outs() << "❌ No P4HIR controls found\n\n";

    assert(lastControl && "No P4HIR control found in the module!");
    return *lastControl;
}

void P4HIRAdaptivePartitioningPass::analyzeP4HIRControl(Operation* controlOp) {
    auto controlName = controlOp->getAttrOfType<StringAttr>("sym_name");
    StringRef name = controlName ? controlName.getValue() : "unnamed";
    
    llvm::outs() << "📋 Analyzing P4HIR Control: " << name << "\n";
    
    // Find control_apply using proper type checking
    Operation* controlApplyOp = nullptr;
    controlOp->walk([&](Operation* op) {
        if (isa<P4::P4MLIR::P4HIR::ControlApplyOp>(op)) {
            controlApplyOp = op;
            llvm::outs() << "  🚀 Found control_apply block\n";
            return WalkResult::interrupt();
        }
        return WalkResult::advance();
    });
    
    if (!controlApplyOp) {
        llvm::outs() << "  ⚠️  No control_apply found in this control\n";
        return;
    }
    
    // Traverse the pipeline
    P4HIRTraversalResult result;
    traverseP4HIRPipeline(controlOp, controlApplyOp, result);
    reportP4HIRResults(result, name);
}
#if 0
void P4HIRAdaptivePartitioningPass::traverseOperation(AnalysisEnv* env, Operation* op, P4HIRTraversalResult& result) {
    env->traceLine("🔍 " + op->getName().getStringRef().str());

    if (auto tableApplyOp = dyn_cast<P4::P4MLIR::P4HIR::TableApplyOp>(op)) {
        if (auto symRef = op->getAttrOfType<mlir::FlatSymbolRefAttr>("callee")) {
            env->traceLine("🏪 Table: " + symRef.getValue().str());
            Operation *tableOpDef = mlir::SymbolTable::lookupNearestSymbolFrom(op, symRef);
            if (auto tableOp = dyn_cast<P4::P4MLIR::P4HIR::TableOp>(tableOpDef)) {

                P4::P4MLIR::P4HIR::TableKeyOp keyOp = nullptr;
                llvm::SmallVector<P4::P4MLIR::P4HIR::TableActionOp> actionOps;

                for (unsigned r = 0; r < tableOp->getNumRegions(); ++r) {
                    auto &region = tableOp->getRegion(r);
                    if (region.empty())
                        continue;
                    for (auto &block : region) {
                        for (auto &innerOp : block) {
                            if (auto op = dyn_cast<P4::P4MLIR::P4HIR::TableKeyOp>(&innerOp)) {
                                keyOp = op;
                            }
                            else if (auto op = dyn_cast<P4::P4MLIR::P4HIR::TableActionsOp>(&innerOp)) {
                                op->walk([&](Operation* actionOp) {
                                    if (auto action = dyn_cast<P4::P4MLIR::P4HIR::TableActionOp>(actionOp)) {
                                        actionOps.push_back(action);
                                    }
                                });
                            }
                        }
                    }
                }
            }
        }
    }
    else if (auto ifElseOp = dyn_cast<P4::P4MLIR::P4HIR::IfOp>(op)) {
        env->traceLine("🔀 If-Else: " + op->getName().getStringRef().str() + " has " + std::to_string(op->getNumRegions()) + " region(s)");
         for (unsigned r = 0; r < op->getNumRegions(); ++r) {
            Region &region = op->getRegion(r);
            if (region.empty()) continue;

            for (Block &block : region) {
                env->traceLine("  Block " + std::to_string(r) + " : ");
                if (block.empty()) continue;
                for (Operation &innerOp : block) {
                    traverseOperation(env, &innerOp, result);
                    if (innerOp.getNumRegions() > 0) {
                        traverseRegion(env, &innerOp, result);
                    }
                }
            }
        }
    }
}

void P4HIRAdaptivePartitioningPass::traverseRegion(AnalysisEnv* env, Operation* regionOp, P4HIRTraversalResult& result) {
    // Process each region of the container operation
    env->traceEnter("Region of " + regionOp->getName().getStringRef().str());

    for (auto& region : regionOp->getRegions()) {
        for (auto& block : region) {
            for (auto& op : block) {
                // Process this operation
                traverseOperation(env, &op, result);
                // Recursively traverse this operation's regions
                if (op.getNumRegions() > 0) {
                    traverseRegion(env, &op, result);
                }
            }
        }
    }
    env->traceExit("Region of " + regionOp->getName().getStringRef().str());
}
#endif

void expandCallOp(ControlFlowGraph &cfg) {
    std::vector<CFGBlock*> toRemove;

    auto expandCallInBlock = [&](CFGBlock* ablock) {
        llvm::SmallVector<Operation*> expandedOps;
        llvm::outs() << "Expanding calls in block " << ablock->blockID << "\n";
        llvm::outs() << "  📦 (before) operation: \n";

        // For each op in the block
        for (auto* op : ablock->ops) {
            llvm::outs() << "  📦 Processing operation: \n";
            op->dump();
            if (auto actionOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::TableActionOp>(op)) {
                // For each inner op in the TableActionOp
                actionOp.walk([&](Operation* innerOp) {
                    if (innerOp == actionOp) return;
                    if (auto callOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::CallOp>(innerOp)) {
                        llvm::outs() << "  📞 Expanding CallOp in action: " << callOp->getName() << "\n";
                        auto symRef = callOp->getAttrOfType<mlir::FlatSymbolRefAttr>("callee");
                        if (!symRef) return;
                        Operation *calleeDef = mlir::SymbolTable::lookupNearestSymbolFrom(callOp, symRef);
                        if (!calleeDef) return;
                        // Inline all ops in first region of callee
                        for (unsigned r = 0; r < calleeDef->getNumRegions(); ++r) {
                            auto &region = calleeDef->getRegion(r);
                            if (region.empty()) continue;
                            for (auto &innerBlock : region) {
                                for (auto &innerInnerOp : innerBlock) {
                                    if (llvm::isa<P4::P4MLIR::P4HIR::ReturnOp>(innerInnerOp)) continue;
                                    llvm::outs() << "    📦 Adding inner op: " << innerInnerOp.getName() << "\n";
                                    expandedOps.push_back(&innerInnerOp);
                                }
                            }
                        }
                    } else {
                        llvm::outs() << "  📦 Keeping inner op: " << innerOp->getName() << "\n";
                        expandedOps.push_back(innerOp);
                    }
                });
            } else if (auto actionOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::TableDefaultActionOp>(op)) {
                // For each inner op in the TableActionOp
                actionOp.walk([&](Operation* innerOp) {
                    if (innerOp == actionOp) return;
                    if (auto callOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::CallOp>(innerOp)) {
                        llvm::outs() << "  📞 Expanding CallOp in action: " << callOp->getName() << "\n";
                        auto symRef = callOp->getAttrOfType<mlir::FlatSymbolRefAttr>("callee");
                        if (!symRef) return;
                        Operation *calleeDef = mlir::SymbolTable::lookupNearestSymbolFrom(callOp, symRef);
                        if (!calleeDef) return;
                        // Inline all ops in first region of callee
                        for (unsigned r = 0; r < calleeDef->getNumRegions(); ++r) {
                            auto &region = calleeDef->getRegion(r);
                            if (region.empty()) continue;
                            for (auto &innerBlock : region) {
                                for (auto &innerInnerOp : innerBlock) {
                                    if (llvm::isa<P4::P4MLIR::P4HIR::ReturnOp>(innerInnerOp)) continue;
                                    llvm::outs() << "    📦 Adding inner op: " << innerInnerOp.getName() << "\n";
                                    expandedOps.push_back(&innerInnerOp);
                                }
                            }
                        }
                    } else {
                        llvm::outs() << "  📦 Keeping inner op: " << innerOp->getName() << "\n";
                        expandedOps.push_back(innerOp);
                    }
                });
            } else if (auto callOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::CallOp>(op)) {
                // Direct CallOp in block, not wrapped in TableActionOp
                llvm::outs() << "  📞 Expanding CallOp: " << callOp->getName() << "\n";
                auto symRef = callOp->getAttrOfType<mlir::FlatSymbolRefAttr>("callee");
                if (!symRef) continue;
                Operation *calleeDef = mlir::SymbolTable::lookupNearestSymbolFrom(callOp, symRef);
                if (!calleeDef) continue;
                for (unsigned r = 0; r < calleeDef->getNumRegions(); ++r) {
                    auto &region = calleeDef->getRegion(r);
                    if (region.empty()) continue;
                    for (auto &innerBlock : region) {
                        for (auto &innerInnerOp : innerBlock) {
                            if (llvm::isa<P4::P4MLIR::P4HIR::ReturnOp>(innerInnerOp)) continue;
                            llvm::outs() << "    📦 Adding inner op: " << innerInnerOp.getName() << "\n";
                            expandedOps.push_back(&innerInnerOp);
                        }
                    }
                }
            } else {
                llvm::outs() << "  📦 Keeping op: " << op->getName() << "\n";
                // Any other op: keep as is
                expandedOps.push_back(op);
            }
        }
        ablock->ops = expandedOps;
        llvm::outs() << "  📦 (after)  operation: \n";
        for (auto* op : ablock->ops) {
            op->dump();
        }
    };

    for (CFGIterator it(cfg); !it.atEnd(); ++it) {
        CFGBlock* block = *it;
        block->print(llvm::outs());

        // if (!llvm::isa<P4::P4MLIR::P4HIR::TableApplyOp>(block->ops.front()))
        //     continue;

        Operation *op = block->ops.front();

        if (llvm::isa<P4::P4MLIR::P4HIR::TableApplyOp>(op)) {
            auto tableApplyOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::TableApplyOp>(op);
            if (!tableApplyOp) continue;

            auto symRef = op->getAttrOfType<mlir::FlatSymbolRefAttr>("callee");
            if (!symRef) continue;

            Operation *tableOpDef = mlir::SymbolTable::lookupNearestSymbolFrom(op, symRef);
            auto tableOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::TableOp>(tableOpDef);
            if (!tableOp) continue;

            llvm::outs() << "Expanding TableApplyOp in block " << block->blockID << " for table: " << symRef.getValue() << "\n";

            P4::P4MLIR::P4HIR::TableKeyOp keyOp = nullptr;
            llvm::SmallVector<P4::P4MLIR::P4HIR::TableActionOp> actionOps;
            P4::P4MLIR::P4HIR::TableDefaultActionOp defaultActionOp = nullptr;

            // Collect table block ops
            for (unsigned r = 0; r < tableOp->getNumRegions(); ++r) {
                auto &region = tableOp->getRegion(r);
                if (region.empty()) continue;
                for (auto &innerBlock : region) {
                    for (auto &innerOp : innerBlock) {
                        if (auto kop = dyn_cast<P4::P4MLIR::P4HIR::TableKeyOp>(&innerOp)) {
                            keyOp = kop;
                            llvm::outs() << "  📦 Found TableKeyOp: \n";
                            kop.dump();
                        }
                        else if (auto aop = dyn_cast<P4::P4MLIR::P4HIR::TableActionsOp>(&innerOp)) {
                            // actionOps.push_back(aop);
                            // llvm::outs() << "  📦 Found TableActionsOp: \n";
                            // aop.dump();
                            aop->walk([&](Operation* actionOp) {
                                if (auto action = dyn_cast<P4::P4MLIR::P4HIR::TableActionOp>(actionOp)) {
                                    actionOps.push_back(action);
                                }
                            });
                        }
                        else if (auto dop = dyn_cast<P4::P4MLIR::P4HIR::TableDefaultActionOp>(&innerOp)) {
                            defaultActionOp = dop;
                            llvm::outs() << "  📦 Found TableDefaultActionOp: \n";
                            dop.dump();
                        }
                    }
                }
            }

            // Create a separate CFGBlock for each action op
            llvm::SmallVector<CFGBlock*> actionBlocks;
            for (auto aop : actionOps) {
                CFGBlock* ablock = cfg.createBlock({aop});
                expandCallInBlock(ablock);
                actionBlocks.push_back(ablock);
                llvm::outs() << "  📦 Created action block: " << ablock->blockID << "\n";
                ablock->print(llvm::outs());
            }

            // Create a separate block for default action
            CFGBlock* defaultBlock = nullptr;
            if (defaultActionOp) {
                defaultBlock = cfg.createBlock({defaultActionOp});
                expandCallInBlock(defaultBlock);
                llvm::outs() << "  📦 Created default action block: " << defaultBlock->blockID << "\n";
                defaultBlock->print(llvm::outs());
            }

            // Create key block if present
            CFGBlock* keyBlock = nullptr;
            if (keyOp) {
                keyBlock = cfg.createBlock({keyOp});
                llvm::outs() << "  📦 Key block created for table: " << symRef.getValue() << " as block " << keyBlock->blockID << "\n";
                keyBlock->print(llvm::outs());
            }

            if (keyBlock) {
                // Predecessors of TableApplyOp now point to keyBlock
                for (const auto &predAdj : block->preds) {
                    CFGBlock *pred = predAdj.getBlock();
                    if (!pred) continue;
                    llvm::outs() << "  📦 Predecessor block: " << pred->blockID << "\n";
                    cfg.removeSuccessor(pred, block);
                    cfg.addSuccessor(pred, keyBlock);
                    cfg.addPredecessor(keyBlock, pred);
                }
                // KeyBlock points to actions/default blocks
                for (auto *ablock : actionBlocks) {
                    if (!ablock) continue;
                    cfg.addSuccessor(keyBlock, ablock, CFGBlock::AdjacentBlock::AB_Alternate);
                    cfg.addPredecessor(ablock, keyBlock, CFGBlock::AdjacentBlock::AB_Alternate);
                }
                if (defaultBlock) {
                    cfg.addSuccessor(keyBlock, defaultBlock, CFGBlock::AdjacentBlock::AB_Default);
                    cfg.addPredecessor(defaultBlock, keyBlock, CFGBlock::AdjacentBlock::AB_Default);
                }
            } else {
                llvm::outs() << "  ⚠️ No key block created for table: " << symRef.getValue() << "\n";
                // No key: predecessors attach directly to actions/default
                for (const auto &predAdj : block->preds) {
                    CFGBlock *pred = predAdj.getBlock();
                    if (!pred) continue;
                    llvm::outs() << "  📦 Predecessor block: " << pred->blockID << "\n";
                    cfg.removeSuccessor(pred, block);
                    for (auto *ablock : actionBlocks) {
                        if (!ablock) continue;
                        cfg.addSuccessor(pred, ablock, CFGBlock::AdjacentBlock::AB_Alternate);
                        cfg.addPredecessor(ablock, pred, CFGBlock::AdjacentBlock::AB_Alternate);
                    }
                    if (defaultBlock) {
                        cfg.addSuccessor(pred, defaultBlock, CFGBlock::AdjacentBlock::AB_Default);
                        cfg.addPredecessor(defaultBlock, pred, CFGBlock::AdjacentBlock::AB_Default);
                    }
                }
            }
            // Actions/default point to original successors
            for (const auto &succAdj : block->succs) {
                CFGBlock *succ = succAdj.getBlock();
                if (!succ) continue;
                llvm::outs() << "  📦 Successor block: " << succ->blockID << "\n";
                for (auto *ablock : actionBlocks) {
                    if (!ablock) continue;
                    cfg.addSuccessor(ablock, succ);
                    cfg.addPredecessor(succ, ablock);
                }
                if (defaultBlock) {
                    cfg.addSuccessor(defaultBlock, succ);
                    cfg.addPredecessor(succ, defaultBlock);
                }
                cfg.removePredecessor(succ, block);
            }
            toRemove.push_back(block);
        } else {
            expandCallInBlock(block);
        }   
    }

    llvm::outs() << "🗑️ Removing " << toRemove.size() << " TableApplyOp blocks from CFG\n";

    // Remove TableApplyOp blocks from CFG
    for (auto *b : toRemove) {
        llvm::outs() << "  🗑️ Removing block: " << b->blockID << "\n";
        auto it = std::find_if(cfg.blocks.begin(), cfg.blocks.end(),
            [b](const std::unique_ptr<CFGBlock>& ptr) { return ptr.get() == b; });
        if (it != cfg.blocks.end()) cfg.blocks.erase(it);
    }
}
#if 0
void traverseBlock(Block& block, ControlFlowGraph& cfg, CFGBlock*& currentBlock, llvm::SmallVector<Operation*>& opsInBlock, std::function<bool(Operation*)> isTerminator) {
    for (Operation &op : block) {
        llvm::outs() << "🔍 Processing operation: " << op.getName() << "\n";
        if (llvm::isa<P4::P4MLIR::P4HIR::ControlApplyOp>(&op)
            || llvm::isa<P4::P4MLIR::P4HIR::YieldOp>(&op)) {
            continue;
        }

        opsInBlock.push_back(&op);

        if (isTerminator(&op)) {
            llvm::outs() << "🔚 Found terminator operation: " << op.getName() << "\n";
            // Create a new block for all ops collected so far
            CFGBlock* blockObj = cfg.createBlock(opsInBlock);
            opsInBlock.clear();

            // Connect to previous block
            if (currentBlock) {
                cfg.addSuccessor(currentBlock, blockObj);
                cfg.addPredecessor(blockObj, currentBlock);
            }
            currentBlock = blockObj;

            // If it's an IfOp, recurse into its regions
            if (auto ifElseOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::IfOp>(&op)) {
                // True branch
                if (!ifElseOp.getThenRegion().empty()) {
                    for (Block &trueBlock : ifElseOp.getThenRegion()) {
                        llvm::SmallVector<Operation*> trueOps;
                        CFGBlock* trueBranchBlock = nullptr;
                        traverseBlock(trueBlock, cfg, trueBranchBlock, trueOps, isTerminator);
                        if (trueBranchBlock) {
                            cfg.addSuccessor(currentBlock, trueBranchBlock);
                            cfg.addPredecessor(trueBranchBlock, currentBlock);
                        }
                    }
                }
                // False branch
                if (!ifElseOp.getElseRegion().empty()) {
                    for (Block &falseBlock : ifElseOp.getElseRegion()) {
                        llvm::SmallVector<Operation*> falseOps;
                        CFGBlock* falseBranchBlock = nullptr;
                        traverseBlock(falseBlock, cfg, falseBranchBlock, falseOps, isTerminator);
                        if (falseBranchBlock) {
                            cfg.addSuccessor(currentBlock, falseBranchBlock);
                            cfg.addPredecessor(falseBranchBlock, currentBlock);
                        }
                    }
                }
            } else if (auto tableApplyOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::TableApplyOp>(&op)) {
                llvm::outs() << "🔚 Found TableApplyOp: " << op.getName() << "\n";
                CFGBlock* blockObj = cfg.createBlock(opsInBlock);
                opsInBlock.clear();

                if (currentBlock) {
                    cfg.addSuccessor(currentBlock, blockObj);
                    cfg.addPredecessor(blockObj, currentBlock);
                }
                currentBlock = blockObj;

                continue;
            }
        }
    }
}
#endif

std::pair<CFGBlock*, CFGBlock*> traverseBlock(Block& block, ControlFlowGraph& cfg, std::function<bool(Operation*)> isTerminator) {
    CFGBlock* startBlock = nullptr;
    CFGBlock* lastBlock = nullptr;
    llvm::SmallVector<Operation*> currentOps;

    for (auto& op : block) {
        llvm::outs() << "🔍 Processing operation: " << op.getName() << "\n";
        if (llvm::isa<P4::P4MLIR::P4HIR::ControlApplyOp>(&op)) {
            continue;
        }

        // TableApplyOp: emit a block for previous ops (if any), then TableApplyOp as its own block
        if (llvm::isa<P4::P4MLIR::P4HIR::TableApplyOp>(&op)) {
            if (!currentOps.empty()) {
                CFGBlock* blockBeforeTable = cfg.createBlock(currentOps);
                if (!startBlock) startBlock = blockBeforeTable;
                if (lastBlock) {
                    cfg.addSuccessor(lastBlock, blockBeforeTable);
                    cfg.addPredecessor(blockBeforeTable, lastBlock);
                }
                lastBlock = blockBeforeTable;
                currentOps.clear();
            }
            // TableApplyOp block
            llvm::SmallVector<Operation*> tableOps;
            tableOps.push_back(&op);
            CFGBlock* tableBlock = cfg.createBlock(tableOps);
            if (!startBlock) startBlock = tableBlock;
            if (lastBlock) {
                cfg.addSuccessor(lastBlock, tableBlock);
                cfg.addPredecessor(tableBlock, lastBlock);
            }
            lastBlock = tableBlock;
            continue;
        } else if (auto yieldOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::YieldOp>(&op)) {
            if (!currentOps.empty()) {
                CFGBlock* yieldBlock = cfg.createBlock(currentOps);
                if (!startBlock) startBlock = yieldBlock;
                if (lastBlock) {
                    cfg.addSuccessor(lastBlock, yieldBlock);
                    cfg.addPredecessor(yieldBlock, lastBlock);
                }
                lastBlock = yieldBlock;
                currentOps.clear();
            }
            continue;
        }

        currentOps.push_back(&op);

        if (auto ifOp = llvm::dyn_cast<P4::P4MLIR::P4HIR::IfOp>(&op)) {
            CFGBlock* ifBlock = cfg.createBlock(currentOps);
            if (!startBlock) startBlock = ifBlock;
            if (lastBlock) {
                cfg.addSuccessor(lastBlock, ifBlock);
                cfg.addPredecessor(ifBlock, lastBlock);
            }
            lastBlock = ifBlock;
            currentOps.clear();

            // Handle branches
            CFGBlock *thenStart = nullptr, *thenEnd = nullptr;
            if (!ifOp.getThenRegion().empty()) {
                for (Block &thenBlock : ifOp.getThenRegion()) {
                    auto [s, e] = traverseBlock(thenBlock, cfg, isTerminator);
                    if (!thenStart) thenStart = s;
                    thenEnd = e;
                }
            }
            CFGBlock *elseStart = nullptr, *elseEnd = nullptr;
            if (!ifOp.getElseRegion().empty()) {
                for (Block &elseBlock : ifOp.getElseRegion()) {
                    auto [s, e] = traverseBlock(elseBlock, cfg, isTerminator);
                    if (!elseStart) elseStart = s;
                    elseEnd = e;
                }
            }
            // Attach branches
            if (thenStart) {
                cfg.addSuccessor(ifBlock, thenStart);
                cfg.addPredecessor(thenStart, ifBlock);
            }
            if (elseStart) {
                cfg.addSuccessor(ifBlock, elseStart);
                cfg.addPredecessor(elseStart, ifBlock);
            }
            continue;
        }
    }

    return {startBlock, lastBlock};
}

void P4HIRAdaptivePartitioningPass::traverseP4HIRPipeline(Operation* controlOp, Operation* controlApplyOp, P4HIRTraversalResult& result) {
    llvm::outs() << "  🔄 Traversing control_apply pipeline...\n";

    AnalysisEnv env;
    ControlFlowGraph cfg;

    auto isTerminator = [](Operation* op) {
        return llvm::isa<P4::P4MLIR::P4HIR::IfOp>(op) ||
               llvm::isa<P4::P4MLIR::P4HIR::TableApplyOp>(op);
    };

    // CFGBlock* currentBlock = nullptr;
    // llvm::SmallVector<Operation*> opsInBlock;

    for (Block &block : controlApplyOp->getRegion(0)) {
        // traverseBlock(block, cfg, currentBlock, opsInBlock, isTerminator);
        traverseBlock(block, cfg, isTerminator);
    }

    llvm::outs() << "  📦 CFG Blocks (DFS):\n";
    for (CFGIterator it(cfg); !it.atEnd(); ++it) {
        CFGBlock* block = *it;
        llvm::outs() << "    - Block@" << block << " with ops:\n";
        for (auto* op : block->getOperations()) {
            llvm::outs() << "      * " << op->getName().getStringRef() << "\n";
        }
    }

    expandCallOp(cfg);

    std::string headerStr;
    {
        llvm::raw_string_ostream headerStream(headerStr);
        controlOp->print(headerStream, mlir::OpPrintingFlags().skipRegions());
    }

    // Iterate and emit each block
    for (CFGIterator it(cfg); !it.atEnd(); ++it) {
        CFGBlock* blk = *it;
        std::ostringstream fname;
        fname << "Block" << blk->blockID << ".mlir";
        std::ofstream fout(fname.str());
        if (!fout.is_open()) continue;

        // Emit header
        fout << headerStr << " {\n";
        fout << "  // Block " << blk->blockID << "\n";

        // Emit ops in block
        for (auto *op : blk->ops) {
            if (!op) continue;
            std::string opStr;
            llvm::raw_string_ostream opStream(opStr);
            op->print(opStream);

            // Indent each line of the op for MLIR readability
            std::istringstream inStream(opStr);
            std::string line;
            while (std::getline(inStream, line)) {
                fout << "  " << line << "\n";
            }
        }

        fout << "}\n";
        fout.close();
    }

    cfg.writeDOTFile("cfg.dot");
    env.writeDOTFile("pipeline.dot");
}

#if 0
void P4HIRAdaptivePartitioningPass::traverseP4HIRPipeline(Operation* controlOp, Operation* controlApplyOp, P4HIRTraversalResult& result) {
    AnalysisEnv env;
    ControlFlowGraph cfg;

    auto isTerminator = [](Operation* op) {
        return llvm::isa<P4::P4MLIR::P4HIR::IfOp>(op) ||
               llvm::isa<P4::P4MLIR::P4HIR::TableApplyOp>(op);
    };

    CFGBlock* currentBlock = nullptr;
    llvm::SmallVector<Operation*> opsInBlock;

    for (Block& block : controlApplyOp->getRegion(0)) {
        traverseBlock(block, cfg, currentBlock, opsInBlock, isTerminator);
    }

    // Handle trailing ops after last terminator
    if (!opsInBlock.empty()) {
        CFGBlock* block = cfg.createBlock(opsInBlock);
        expandCallsInBlock(block);
        if (currentBlock) {
            cfg.addSuccessor(currentBlock, block);
            cfg.addPredecessor(block, currentBlock);
        }
    }

    // Now expand TableApplyOps (including their TableActionOps and recursive calls)
    expandTableApplyOps(cfg);

    cfg.writeDOTFile("cfg.dot");
    env.writeDOTFile("pipeline.dot");
}
#endif

void P4HIRAdaptivePartitioningPass::analyzeTableOp(struct AnalysisEnv* env, Operation* op, P4HIRTraversalResult& result) {
    if (auto tableApplyOp = dyn_cast<P4::P4MLIR::P4HIR::TableApplyOp>(op)) {
        llvm::outs() << "\n      📊 Table Apply Analysis:";
        op->dump();
    }
}

void P4HIRAdaptivePartitioningPass::reportP4HIRResults(const P4HIRTraversalResult& result, 
                                                       StringRef controlName) {
  llvm::outs() << "\n📊 P4HIR Traversal Results for " << controlName << ":\n";
  llvm::outs() << "=" << std::string(60, '=') << "\n";
  
  // Operation counts
  llvm::outs() << "📈 Operation Categories:\n";
  llvm::outs() << "  🏪 Table operations:     " << result.tableOps << "\n";
  llvm::outs() << "  🔀 Control flow ops:     " << result.controlFlowOps << "\n";
  llvm::outs() << "  💾 Data operations:      " << result.dataOps << "\n";
  llvm::outs() << "  🎭 Action operations:    " << result.actionOps << "\n";
  
  // Target assignment
  llvm::outs() << "\n🎯 Target Assignment:\n";
  llvm::outs() << "  🔷 DRMT candidates:      " << result.drmtCandidates.size() << "\n";
  llvm::outs() << "  🔶 DPA candidates:       " << result.dpaCandidates.size() << "\n";
  llvm::outs() << "  ❓ Unknown operations:   " << result.unknownOps.size() << "\n";
  
  // Unknown operations details
  if (!result.unknownOps.empty()) {
    llvm::outs() << "\n⚠️  Unknown P4HIR operations:\n";
    for (auto* op : result.unknownOps) {
      llvm::outs() << "  - " << op->getName().getStringRef() << "\n";
    }
  }
  
  llvm::outs() << "=" << std::string(60, '=') << "\n";
}

void P4HIRAdaptivePartitioningPass::getDependentDialects(DialectRegistry& registry) const {
  // Add P4HIR dialect when available
}
#if 0
OpPassManager P4HIRAdaptivePartitioningPass::createEgglogPM(
    Operation *op, const std::string &mlirFile) {
    std::map<std::string, AttrStringifyFunction> attrStringifiers = {
        {arith::FastMathFlagsAttr::name.str(), stringifyFastMathFlagsAttr},
        {P4::P4MLIR::P4HIR::IntAttr::name.str(), stringifyP4HIRIntAttr},
        {P4::P4MLIR::P4HIR::MatchKindAttr::name.str(), stringifyP4HIRMatchKindAttr},
    };
    std::map<std::string, AttrParseFunction> attrParsers = {
        {"arith_fastmath", parseFastMathFlagsAttr},
        {"p4hir_int", parseP4HIRIntAttr}
        {"p4hir_match_kind", parseP4HIRMatchKindAttr}
    };
    std::map<std::string, TypeStringifyFunction> typeStringifiers = {
        {RankedTensorType::name.str(), stringifyRankedTensorType},
        {P4::P4MLIR::P4HIR::BitsType::name.str(), stringifyP4HIRBitsType},
        {P4::P4MLIR::P4HIR::ValidBitType::name.str(), stringifyP4HIRValidBitType},
        {P4::P4MLIR::P4HIR::StructType::name.str(), stringifyP4HIRStructType},
        {P4::P4MLIR::P4HIR::HeaderType::name.str(), stringifyP4HIRHeaderType},
        {P4::P4MLIR::P4HIR::ReferenceType::name.str(), stringifyP4HIRReferenceType}
    };
    std::map<std::string, TypeParseFunction> typeParsers = {
        {"RankedTensor", parseRankedTensorType},
        {"p4hir_bits", parseP4HIRBitsType},
        {"p4hir_validity_bit", parseP4HIRValidBitType},
        {"p4hir_struct", parseP4HIRStructType},
        {"p4hir_header", parseP4HIRHeaderType},
        {"p4hir_ref", parseP4HIRReferenceType}
    };

    EgglogCustomDefs funcs = {attrStringifiers, attrParsers, typeStringifiers, typeParsers};

    // OpPassManager must be constructed with the op name we want to run on
    llvm::outs() << "EqSat on OP:" << op->getName().getStringRef() << "\n";
    OpPassManager pm(op->getName().getStringRef());
    pm.addPass(createP4HIRControlEqualitySaturationPass(
      mlirFile,
      "/local/yihan/tangram-compiler/tests/p4tobf3drmt/input.egg",
      funcs
    ));
    return pm;
}
#endif
#if 0
void P4HIRAdaptivePartitioningPass::runOnOperation() {
    auto module = getOperation();
    
    llvm::outs() << "📋 P4HIR Adaptive Partitioning Pass\n";
    llvm::outs() << "===================================\n";

    auto p4hirControls = findP4HIRControls(module);
    
    if (p4hirControls.empty()) {
        llvm::outs() << "❌ No P4HIR controls found in module\n";
        return;
    }

    for (auto* control : p4hirControls) {
        analyzeP4HIRControl(control);
        llvm::outs() << "\n";
    }
    
    llvm::outs() << "🎉 P4HIR traversal complete!\n";
}
#endif

void P4HIRAdaptivePartitioningPass::runOnOperation() {
    auto module = getOperation();

    llvm::outs() << "📋 P4HIR Adaptive Partitioning Pass\n";
    llvm::outs() << "===================================\n";

    // Use the new sequential function that returns a single reference
    Operation& lastControl = findLastP4HIRControl(module);

    // Analyze only the last found control
    analyzeP4HIRControl(&lastControl);
    llvm::outs() << "\n";

    llvm::outs() << "🎉 P4HIR traversal complete!\n";
}

std::unique_ptr<Pass> mlir::createP4HIRAdaptivePartitioningPass() {
  return std::make_unique<P4HIRAdaptivePartitioningPass>();
}