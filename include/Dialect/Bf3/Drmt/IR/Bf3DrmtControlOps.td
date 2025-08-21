//===- Bf3DrmtOps.td - BF3 dRMT dialect ops -----------*- tablegen -*-===//
//===----------------------------------------------------------------------===//

#ifndef BF3_BF3DRMT_CONTROLOPS
#define BF3_BF3DRMT_CONTROLOPS

def Bf3Drmt_ControlOp : Bf3Drmt_Op<"control",
        [Symbol, SymbolTable]> {
    let arguments = (ins SymbolNameAttr:$sym_name,
                        OptionalAttr<DictArrayAttr>:$arg_attrs,
                        OptionalAttr<DictionaryAttr>:$annotations);
    let regions = (region SizedRegion<1>:$body);
    let hasCustomAssemblyFormat = 1;

    let skipDefaultBuilders = 1;

    let builders = [
        OpBuilder<(ins "llvm::StringRef":$sym_name, 
                CArg<"llvm::ArrayRef<mlir::DictionaryAttr>", "llvm::ArrayRef<mlir::DictionaryAttr>()">:$argAttrs,
                CArg<"mlir::DictionaryAttr", "mlir::DictionaryAttr()">:$annotations)>
    ];

    let extraClassDeclaration = [{
        mlir::Region *getCallableRegion() { return &getBody(); }

        llvm::ArrayRef<mlir::Type> getResultTypes() {
        return {};
        }

        void createEntryBlock();
    }];
}

def Bf3Drmt_PipeKeyOp : Bf3Drmt_Op<"pipe_key",
        [NoRegionArguments, NoTerminator]> {
    let summary = "Represents a pipe key";
    let description = [{

    }];

    let arguments = (ins OptionalAttr<DictionaryAttr>:$annotations);
    let regions = (region SizedRegion<1>:$body);

    let hasVerifier = 0;
    let skipDefaultBuilders = 1;
    let assemblyFormat = [{
      (`annotations` $annotations^)? $body attr-dict
    }];

    let builders = [
      OpBuilder<(ins "mlir::DictionaryAttr":$annotations,
                    "llvm::function_ref<void(mlir::OpBuilder &, mlir::Location)>":$keyBuilder)>,
    ];
}

def Bf3Drmt_PipeKeyEntryOp : Bf3Drmt_Op<"match_key", []> {
    let summary = "Represents a pipe key entry";
    let description = [{

    }];

    let arguments = (ins Bf3Drmt_MatchKindAttr:$match_kind, AnyBf3DrmtType:$value,
                        OptionalAttr<DictionaryAttr>:$annotations);

    // TODO: Check
    let hasVerifier = 0;
    let assemblyFormat = [{
        $match_kind $value `:` type($value) (`annotations` $annotations^)? attr-dict
    }];

    let builders = [
        OpBuilder<(ins "llvm::StringRef":$match_kind, "mlir::Value":$value,
                    "mlir::DictionaryAttr":$annotations), [{
        build($_builder, $_state,
                bf3drmt::MatchKindAttr::get($_builder.getContext(), match_kind), value,
                annotations.empty() ? mlir::DictionaryAttr() : annotations);
        }]
        >,
    ];

}

#endif  // BF3_BF3DRMT_CONTROLOPS