#include "mlir/IR/Attributes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/AsmParser/AsmParser.h"
#include "Dialect/P4HIR/P4HIR_Attrs.h"
#include "Dialect/P4HIR/P4HIR_Types.h"
#include "Dialect/Bf3/Drmt/IR/Bf3DrmtTypes.h"

#include "Pass/Egglog.h"
#include "Pass/EgglogCustomDefs.h"

/** 
 * Parse the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
mlir::Attribute parseFastMathFlagsAttr(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "arith_fastmath");

    std::string flag = split[1];
    if (flag.front() == '(' && flag.back() == ')') {
        flag = flag.substr(1, flag.size() - 2);
    }

    std::string strAttr = "#arith.fastmath<" + flag + ">";
    mlir::Attribute parsedAttr = mlir::parseAttribute(strAttr, &egglog.context);

    // dump
    // llvm::outs() << "Parsing mlir::arith::FastMathFlagsAttr: " << strAttr << "\n";
    // llvm::outs() << "Parsed mlir::arith::FastMathFlagsAttr: " << parsedAttr << "\n";

    return parsedAttr;
}

/**
 * Serialize the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
std::vector<std::string> stringifyFastMathFlagsAttr(mlir::Attribute attr, Egglog& egglog) {
    std::vector<std::string> split;

    split.push_back("arith_fastmath");

    mlir::arith::FastMathFlagsAttr fastMathAttr = attr.cast<mlir::arith::FastMathFlagsAttr>();
    mlir::arith::FastMathFlags flags = fastMathAttr.getValue();

    split.push_back("(" + mlir::arith::stringifyFastMathFlags(flags) + ")");

    // dump
    llvm::outs() << "Stringified mlir::arith::FastMathFlagsAttr: ";
    for (const std::string& s: split) {
        llvm::outs() << s << " , ";
    }
    llvm::outs() << "\n";

    return split;
}

/** Parse the type (function RankedTensor (IntVec Type) Type) */
mlir::Type parseRankedTensorType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "RankedTensor");

    std::string dimVec = split[1];
    std::string type = split[2];

    // parse dimvec, form (vec-of <N1> <N2> ... <Nn>)
    std::vector<std::string> dimVecSplit = Egglog::splitExpression(dimVec);
    std::vector<int64_t> dims;
    for (unsigned int i = 1; i < dimVecSplit.size(); i++) {
        dims.push_back(std::stoll(dimVecSplit[i]));
    }

    mlir::Type parsedType = egglog.parseType(type);  // parse type
    return mlir::RankedTensorType::get(dims, parsedType);
}

/** Serialize the type (function RankedTensor (IntVec Type) Type) */
std::vector<std::string> stringifyRankedTensorType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    split.push_back("RankedTensor");

    mlir::RankedTensorType tensorType = type.cast<mlir::RankedTensorType>();
    llvm::ArrayRef<int64_t> shape = tensorType.getShape();
    mlir::Type elementType = tensorType.getElementType();

    // serialize dimvec
    std::string dimVec = "(vec-of";
    for (int64_t dim: shape) {
        dimVec += " " + std::to_string(dim);
    }
    dimVec += ")";

    split.push_back(dimVec);
    split.push_back(egglog.eggifyType(elementType));

    return split;
}

/** 
 * Parse the attribute (function arith_fastmath (FastMathFlags) Attr)
 * Where (datatype FastMathFlags (none) (reassoc) (nnan) ...)
 */
mlir::Attribute parseP4HIRIntAttr(const std::vector<std::string>& split, Egglog& egglog) {
    std::string attrType = split[0];
    assert(attrType == "p4hir_int");

    for (size_t i = 1; i < split.size(); i++) {
        llvm::outs() << "Parsing P4HIR IntAttr: " << split[i] << "\n";
    }

    // Parse the value (first argument)
    std::string valueStr = split[1];
    if (valueStr.front() == '(' && valueStr.back() == ')') {
        valueStr = valueStr.substr(1, valueStr.size() - 2);
    }
    int64_t value = std::stoll(valueStr);

    // Parse the type (second argument should be a bit type)
    std::string typeStr = split[2];
    mlir::Type type = egglog.parseType(typeStr);
    
    // Use mlir::dyn_cast instead of type.dyn_cast
    auto bitType = mlir::dyn_cast<P4::P4MLIR::P4HIR::BitsType>(type);
    assert(bitType && "P4HIR int attribute must have bit type");

    // Create APInt with the value and bit width
    llvm::APInt apValue(bitType.getWidth(), value, /*isSigned=*/true);

    // Create P4HIR integer attribute with correct parameter order: context, type, value
    return P4::P4MLIR::P4HIR::IntAttr::get(&egglog.context, type, apValue);
}

std::vector<std::string> stringifyP4HIRIntAttr(mlir::Attribute attr, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Cast to P4HIR IntAttr
    auto intAttr = mlir::dyn_cast<P4::P4MLIR::P4HIR::IntAttr>(attr);
    assert(intAttr && "Expected P4HIR IntAttr");

    // Get the value as APInt
    llvm::APInt value = intAttr.getValue();
    
    // Get the type
    mlir::Type type = intAttr.getType();

    split.push_back("p4hir_int");
    // Convert APInt to string - use getSExtValue() for signed interpretation
    split.push_back(std::to_string(value.getSExtValue()));
    // Add the type
    split.push_back(egglog.eggifyType(type));

    return split;
}

mlir::Attribute parseP4HIRMatchKindAttr(const std::vector<std::string>& split, Egglog &egglog) {
    std::string attrType = split[0];
    assert(attrType == "p4hir_match_kind");

    if (split.size() < 2) {
        llvm::errs() << "Expected value for p4hir_match_kind\n";
        return {};
    }

    // Second token is the match kind string (e.g., "exact")
    std::string valueStr = split[1];

    // Remove surrounding parentheses/quotes if needed
    if (valueStr.front() == '"' && valueStr.back() == '"')
        valueStr = valueStr.substr(1, valueStr.size() - 2);

    // Build the MLIR attribute
    return P4::P4MLIR::P4HIR::MatchKindAttr::get(
        &egglog.context, mlir::StringAttr::get(&egglog.context, valueStr));
}

std::vector<std::string> stringifyP4HIRMatchKindAttr(mlir::Attribute attr, Egglog& egglog) {
    std::vector<std::string> split;
    auto mkAttr = mlir::dyn_cast<P4::P4MLIR::P4HIR::MatchKindAttr>(attr);
    assert(mkAttr && "Expected MatchKindAttr");

    split.push_back("p4hir_match_kind");
    // Preserve quotes so Egglog knows it's a string
    split.push_back("\"" + mkAttr.getValue().getValue().str() + "\"");
    return split;
}

/** Parse P4HIR bit type (function p4hir.bits (Int) Type) */
mlir::Type parseP4HIRBitsType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_bits");

    // Parse width parameter
    std::string width = split[1];
    if (width.front() == '(' && width.back() == ')') {
        width = width.substr(1, width.size() - 2);
    }

    // Create the type string and parse it
    std::string strType = "!p4hir.bit<" + width + ">";
    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);

    return parsedType;
}

/** Stringify P4HIR bit type */
std::vector<std::string> stringifyP4HIRBitsType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the bit width from the type
    auto bitsType = type.cast<P4::P4MLIR::P4HIR::BitsType>();
    unsigned width = bitsType.getWidth();

    split.push_back("p4hir_bits");
    split.push_back(std::to_string(width));

    return split;
}

/** Parse P4HIR valid bit type (function p4hir_valid_bit Type) */
mlir::Type parseP4HIRValidBitType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_valid_bit");

    // Valid bit has no parameters
    std::string strType = "!p4hir.valid.bit";
    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);

    return parsedType;
}

/** Stringify P4HIR valid bit type */
std::vector<std::string> stringifyP4HIRValidBitType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;

    // Just return the tag
    (void)egglog; // not needed
    auto validBitType = type.cast<P4::P4MLIR::P4HIR::ValidBitType>();
    split.push_back("p4hir_valid_bit");

    return split;
}

/** Parse P4HIR error type (function p4hir_error Type) */
mlir::Type parseP4HIRErrorType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_error");

    // Error type format: p4hir_error field1 field2 field3 ...
    // Build the type string: !p4hir.error<field1, field2, field3>
    std::string strType = "!p4hir.error";
    
    if (split.size() > 1) {
        strType += "<";
        for (size_t i = 1; i < split.size(); ++i) {
            if (i > 1) {
                strType += ", ";
            }
            strType += split[i];
        }
        strType += ">";
    } else {
        // Empty error type
        strType += "<>";
    }

    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);
    return parsedType;
}

/** Stringify P4HIR error type */
std::vector<std::string> stringifyP4HIRErrorType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    (void)egglog; // not needed
    auto errorType = type.cast<P4::P4MLIR::P4HIR::ErrorType>();
    
    // Start with the type tag
    split.push_back("p4hir_error");
    
    // Add each field from the error type
    mlir::ArrayAttr fields = errorType.getFields();
    for (mlir::Attribute field : fields) {
        if (auto stringAttr = field.dyn_cast<mlir::StringAttr>()) {
            split.push_back(stringAttr.getValue().str());
        }
    }
    
    return split;
}

mlir::Type parseP4HIRHeaderType(const std::vector<std::string> &split, Egglog &egglog) {
    assert(split[0] == "p4hir_header");
    llvm::SmallVector<P4::P4MLIR::P4HIR::FieldInfo> fields;
    std::string headerName;
    bool hasValidityBit = false;

    for (size_t i = 1; i < split.size(); i++) {
        std::string attrStr = split[i];
        // Find NamedAttr
        size_t namedAttrPos = attrStr.find("NamedAttr");
        if (namedAttrPos == std::string::npos) {
            continue;
        }

        // Extract key
        size_t nameStart = attrStr.find('"', namedAttrPos);
        size_t nameEnd   = attrStr.find('"', nameStart + 1);
        if (nameStart == std::string::npos || nameEnd == std::string::npos) {
            llvm::outs() << "    -> skipped (no quoted key)\n";
            continue;
        }
        std::string key = attrStr.substr(nameStart + 1, nameEnd - nameStart - 1);

        if (key == "__valid") {
            hasValidityBit = true;
            continue;
        }

        if (key == "name") {
            // parse header name
            size_t stringAttrPos = attrStr.find("StringAttr", nameEnd);
            if (stringAttrPos != std::string::npos) {
                size_t valueStart = attrStr.find('"', stringAttrPos);
                size_t valueEnd   = attrStr.find('"', valueStart + 1);
                if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                    headerName = attrStr.substr(valueStart + 1, valueEnd - valueStart - 1);
                }
            }
        } else {
            // Parse TypeAttr field
            mlir::Type fieldType = nullptr;
            size_t typeAttrPos = attrStr.find("TypeAttr", nameEnd);
            if (typeAttrPos != std::string::npos) {
                size_t typeStart = attrStr.find('(', typeAttrPos);
                if (typeStart != std::string::npos) {
                    // match parentheses
                    int depth = 0;
                    size_t typeEnd = typeStart;
                    for (size_t j = typeStart; j < attrStr.length(); j++) {
                        if (attrStr[j] == '(') depth++;
                        else if (attrStr[j] == ')') depth--;
                        if (depth == 0) {
                            typeEnd = j;
                            break;
                        }
                    }

                    std::string typeStr = attrStr.substr(typeStart + 1, typeEnd - typeStart - 1);

                    // Special case: p4hir_bits
                    if (typeStr.find("p4hir_bits") == 0) {
                        size_t spacePos = typeStr.find(' ');
                        if (spacePos != std::string::npos) {
                            std::string widthStr = typeStr.substr(spacePos + 1);
                            unsigned width = static_cast<unsigned>(std::stoi(widthStr));
                            std::string strType = "!p4hir.bit<" + std::to_string(width) + ">";
                            fieldType = mlir::parseType(strType, &egglog.context);
                        }
                    } else {
                        fieldType = mlir::parseType(typeStr, &egglog.context);
                    }
                }
            }

            if (fieldType) {
                fields.push_back(
                    P4::P4MLIR::P4HIR::FieldInfo{
                        mlir::StringAttr::get(&egglog.context, key), 
                        fieldType
                    });
            } else {
                llvm::outs() << "    -> FAILED to parse type for field '" << key << "'\n";
            }
        }
    }
#if 0
    // Inject validity bit if not present
    if (!hasValidityBit) {
        auto validType = P4::P4MLIR::P4HIR::ValidBitType::get(&egglog.context);
        llvm::outs() << "  -> Injecting implicit __valid field\n";
        fields.push_back(
            P4::P4MLIR::P4HIR::FieldInfo{
                mlir::StringAttr::get(&egglog.context, "__valid"), 
                validType
            });
    }
#endif
    // llvm::outs() << "[parseP4HIRHeaderType] Final header = " << headerName 
    //              << ", field count = " << fields.size() << "\n";
    // for (auto &f : fields) {
    //     llvm::outs() << "    field '" << f.name.getValue() << "' : ";
    //     f.type.print(llvm::outs());
    //     llvm::outs() << "\n";
    // }

    mlir::DictionaryAttr annots = mlir::DictionaryAttr::get(&egglog.context, {});
    return P4::P4MLIR::P4HIR::HeaderType::get(&egglog.context, headerName, fields, annots);
}

std::vector<std::string> stringifyP4HIRHeaderType(mlir::Type type, Egglog &egglog) {
    std::vector<std::string> split;
    auto headerType = type.cast<P4::P4MLIR::P4HIR::HeaderType>();

    // name attribute
    split.push_back("p4hir_header (NamedAttr \"name\" (StringAttr \"" +
                    headerType.getName().str() + "\"))");

    // fields (excluding __valid)
    for (auto field : headerType.getFields()) {
        if (field.name == P4::P4MLIR::P4HIR::HeaderType::validityBit)
            continue;

        std::string fieldStr = "(NamedAttr \"" + field.name.str() + "\" (TypeAttr ";
        if (auto bitsType = field.type.dyn_cast<P4::P4MLIR::P4HIR::BitsType>()) {
            fieldStr += "(p4hir_bits " + std::to_string(bitsType.getWidth()) + ")";
        } else {
            std::string typeStr;
            llvm::raw_string_ostream rso(typeStr);
            field.type.print(rso);
            rso.flush();
            fieldStr += typeStr;
        }
        fieldStr += "))";
        split.push_back(fieldStr);
    }

    return split;
}

mlir::Type parseP4HIRStructType(const std::vector<std::string> &split,
                                Egglog &egglog) {
  assert(split[0] == "p4hir_struct");

  // First NamedAttr is the struct name
  std::string structName;
  llvm::SmallVector<P4::P4MLIR::P4HIR::FieldInfo> fields;

  for (size_t i = 1; i < split.size(); i++) {
    std::string fieldStr = split[i];

    size_t namedAttrPos = fieldStr.find("NamedAttr");
    if (namedAttrPos == std::string::npos)
      continue;

    // ---- Extract key ----
    size_t nameStart = fieldStr.find('"', namedAttrPos);
    size_t nameEnd = fieldStr.find('"', nameStart + 1);
    if (nameStart == std::string::npos || nameEnd == std::string::npos)
      continue;

    std::string key = fieldStr.substr(nameStart + 1, nameEnd - nameStart - 1);

    // ---- Special case: struct name ----
    if (key == "name") {
      size_t strStart = fieldStr.find('"', nameEnd + 1);
      size_t strEnd = fieldStr.find('"', strStart + 1);
      if (strStart != std::string::npos && strEnd != std::string::npos) {
        structName = fieldStr.substr(strStart + 1, strEnd - strStart - 1);
      }
      continue;
    }

    // ---- Otherwise it's a field ----
    // Extract type string inside (TypeAttr ...)
    size_t typeAttrPos = fieldStr.find("TypeAttr", nameEnd);
    if (typeAttrPos == std::string::npos)
      continue;

    size_t typeStart = fieldStr.find('(', typeAttrPos);
    if (typeStart == std::string::npos)
      continue;

    int depth = 0;
    size_t typeEnd = typeStart;
    for (size_t j = typeStart; j < fieldStr.size(); j++) {
      if (fieldStr[j] == '(')
        depth++;
      else if (fieldStr[j] == ')')
        depth--;
      if (depth == 0) {
        typeEnd = j;
        break;
      }
    }

    std::string typeStr =
        fieldStr.substr(typeStart + 1, typeEnd - typeStart - 1);

    // ---- Parse field type ----
    mlir::Type fieldType;

    if (typeStr.find("p4hir_header") == 0) {
      // Collect header tokens
      std::vector<std::string> headerTokens;
      headerTokens.push_back("p4hir_header");

      size_t pos = 0;
      while ((pos = typeStr.find("(NamedAttr", pos)) != std::string::npos) {
        int d = 0;
        size_t start = pos, end = pos;
        for (size_t k = pos; k < typeStr.size(); k++) {
          if (typeStr[k] == '(')
            d++;
          else if (typeStr[k] == ')')
            d--;
          if (d == 0) {
            end = k + 1;
            break;
          }
        }
        headerTokens.push_back(typeStr.substr(start, end - start));
        pos = end;
      }

      fieldType = parseP4HIRHeaderType(headerTokens, egglog);

    } else if (typeStr.find("p4hir_bits") == 0) {
      // p4hir_bits N
      size_t spacePos = typeStr.find(' ');
      if (spacePos != std::string::npos) {
        std::string widthStr = typeStr.substr(spacePos + 1);
        size_t parenPos = widthStr.find(')');
        if (parenPos != std::string::npos)
          widthStr = widthStr.substr(0, parenPos);

        unsigned width = static_cast<unsigned>(std::stoi(widthStr));
        std::string strType = "!p4hir.bit<" + std::to_string(width) + ">";
        fieldType = mlir::parseType(strType, &egglog.context);
      }
    } else {
      // Fallback: regular MLIR type
      fieldType = mlir::parseType(typeStr, &egglog.context);
    }

    if (fieldType) {
      fields.push_back(
          {mlir::StringAttr::get(&egglog.context, key), fieldType});
    }
  }

  mlir::DictionaryAttr annots =
      mlir::DictionaryAttr::get(&egglog.context, {});
  return P4::P4MLIR::P4HIR::StructType::get(&egglog.context, structName,
                                            fields, annots);
}

std::vector<std::string> stringifyP4HIRStructType(mlir::Type type,
                                                  Egglog &egglog) {
  std::vector<std::string> split;
  auto structType = type.cast<P4::P4MLIR::P4HIR::StructType>();

  split.push_back("p4hir_struct");

  // First, emit the struct name
  split.push_back("(NamedAttr \"name\" (StringAttr \"" +
                  structType.getName().str() + "\"))");

  // Then, emit fields
  for (auto field : structType.getFields()) {
    std::string fieldStr = "(NamedAttr \"" + field.name.str() +
                           "\" (TypeAttr ";

    if (auto hdrType =
            field.type.dyn_cast<P4::P4MLIR::P4HIR::HeaderType>()) {
      // Delegate header
      auto sub = stringifyP4HIRHeaderType(hdrType, egglog);
      fieldStr += "(" + llvm::join(sub, " ") + ")";
    } else if (auto bitsType =
                   field.type.dyn_cast<P4::P4MLIR::P4HIR::BitsType>()) {
      fieldStr += "(p4hir_bits " +
                  std::to_string(bitsType.getWidth()) + ")";
    } else {
      std::string typeStr;
      llvm::raw_string_ostream rso(typeStr);
      field.type.print(rso);
      rso.flush();
      fieldStr += typeStr;
    }

    fieldStr += "))";
    split.push_back(fieldStr);
  }

  return split;
}

/** Parse P4HIR reference type (function p4hir_ref (Type) Type) */
mlir::Type parseP4HIRReferenceType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "p4hir_ref");

    // Parse inner type parameter
    std::string innerTypeStr = split[1];
    
    // Use egglog's parseType method like in parseRankedTensorType
    mlir::Type innerType = egglog.parseType(innerTypeStr);
    if (!innerType) {
        llvm::errs() << "Failed to parse inner type: " << innerTypeStr << "\n";
        return nullptr;
    }

    // Create reference type using P4HIR's RefType::get
    return P4::P4MLIR::P4HIR::ReferenceType::get(innerType);
}

/** Stringify P4HIR reference type */
std::vector<std::string> stringifyP4HIRReferenceType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the inner type from the reference type
    auto refType = type.cast<P4::P4MLIR::P4HIR::ReferenceType>();
    mlir::Type innerType = refType.getObjectType();

    // Get the string representation of the inner type using egglog's eggifyType
    std::string innerTypeStr = egglog.eggifyType(innerType);
    
    split.push_back("p4hir_ref");
    // Add the inner type WITHOUT extra parentheses
    split.push_back(innerTypeStr);  // Remove the extra parentheses

    return split;
}

/* ------- bf3drmt ------- */
/** Parse Bf3Drmt bit type (function bf3drmt.bits (Int) Type) */
mlir::Type parseBf3DrmtBitsType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "bf3drmt_bits");

    llvm::outs() << "Parsing Bf3Drmt bits type: " << typeType << "\n";

    // Parse width parameter
    std::string width = split[1];
    if (width.front() == '(' && width.back() == ')') {
        width = width.substr(1, width.size() - 2);
    }

    // Create the type string and parse it
    std::string strType = "!bf3drmt.bit<" + width + ">";
    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);

    return parsedType;
}

/** Stringify Bf3Drmt bit type */
std::vector<std::string> stringifyBf3DrmtBitsType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the bit width from the type
    auto bitsType = type.cast<mlir::edamlir::bf3drmt::BitsType>();
    unsigned width = bitsType.getWidth();

    split.push_back("bf3drmt_bits");
    split.push_back(std::to_string(width));

    return split;
}

/** Parse Bf3Drmt valid bit type (function bf3drmt_valid_bit Type) */
mlir::Type parseBf3DrmtValidBitType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "bf3drmt_valid_bit");

    // Valid bit has no parameters
    std::string strType = "bf3drmt.valid.bit";
    mlir::Type parsedType = mlir::parseType(strType, &egglog.context);

    return parsedType;
}

/** Stringify Bf3Drmt valid bit type */
std::vector<std::string> stringifyBf3DrmtValidBitType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;

    // Just return the tag
    (void)egglog; // not needed
    auto validBitType = type.cast<mlir::edamlir::bf3drmt::ValidBitType>();
    split.push_back("bf3drmt_valid_bit");

    return split;
}

mlir::Type parseBf3DrmtHeaderType(const std::vector<std::string> &split, Egglog &egglog) {
    assert(split[0] == "bf3drmt_header");
    llvm::SmallVector<mlir::edamlir::bf3drmt::FieldInfo> fields;
    std::string headerName;
    bool hasValidityBit = false;

    for (size_t i = 1; i < split.size(); i++) {
        std::string attrStr = split[i];
        // Find NamedAttr
        size_t namedAttrPos = attrStr.find("NamedAttr");
        if (namedAttrPos == std::string::npos) {
            continue;
        }

        // Extract key
        size_t nameStart = attrStr.find('"', namedAttrPos);
        size_t nameEnd   = attrStr.find('"', nameStart + 1);
        if (nameStart == std::string::npos || nameEnd == std::string::npos) {
            llvm::outs() << "    -> skipped (no quoted key)\n";
            continue;
        }
        std::string key = attrStr.substr(nameStart + 1, nameEnd - nameStart - 1);

        if (key == "__valid") {
            hasValidityBit = true;
            continue;
        }

        if (key == "name") {
            // parse header name
            size_t stringAttrPos = attrStr.find("StringAttr", nameEnd);
            if (stringAttrPos != std::string::npos) {
                size_t valueStart = attrStr.find('"', stringAttrPos);
                size_t valueEnd   = attrStr.find('"', valueStart + 1);
                if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                    headerName = attrStr.substr(valueStart + 1, valueEnd - valueStart - 1);
                }
            }
        } else {
            // Parse TypeAttr field
            mlir::Type fieldType = nullptr;
            size_t typeAttrPos = attrStr.find("TypeAttr", nameEnd);
            if (typeAttrPos != std::string::npos) {
                size_t typeStart = attrStr.find('(', typeAttrPos);
                if (typeStart != std::string::npos) {
                    // match parentheses
                    int depth = 0;
                    size_t typeEnd = typeStart;
                    for (size_t j = typeStart; j < attrStr.length(); j++) {
                        if (attrStr[j] == '(') depth++;
                        else if (attrStr[j] == ')') depth--;
                        if (depth == 0) {
                            typeEnd = j;
                            break;
                        }
                    }

                    std::string typeStr = attrStr.substr(typeStart + 1, typeEnd - typeStart - 1);

                    // Special case: bf3drmt_bits
                    if (typeStr.find("bf3drmt_bits") == 0) {
                        size_t spacePos = typeStr.find(' ');
                        if (spacePos != std::string::npos) {
                            std::string widthStr = typeStr.substr(spacePos + 1);
                            unsigned width = static_cast<unsigned>(std::stoi(widthStr));
                            std::string strType = "!bf3drmt.bit<" + std::to_string(width) + ">";
                            fieldType = mlir::parseType(strType, &egglog.context);
                        }
                    } else {
                        fieldType = mlir::parseType(typeStr, &egglog.context);
                    }
                }
            }

            if (fieldType) {
                fields.push_back(mlir::edamlir::bf3drmt::FieldInfo{mlir::StringAttr::get(&egglog.context, key), fieldType});
            } else {
                llvm::outs() << "    -> FAILED to parse type for field '" << key << "'\n";
            }
        }
    }

    mlir::DictionaryAttr annots = mlir::DictionaryAttr::get(&egglog.context, {});
    return mlir::edamlir::bf3drmt::HeaderType::get(&egglog.context, headerName, fields, annots);
}

std::vector<std::string> stringifyBf3DrmtHeaderType(mlir::Type type, Egglog &egglog) {
    std::vector<std::string> split;
    auto headerType = type.cast<mlir::edamlir::bf3drmt::HeaderType>();

    // name attribute
    split.push_back("bf3drmt_header (NamedAttr \"name\" (StringAttr \"" +
                    headerType.getName().str() + "\"))");

    // fields (excluding __valid)
    for (auto field : headerType.getFields()) {
        if (field.name == mlir::edamlir::bf3drmt::HeaderType::validityBit)
            continue;

        std::string fieldStr = "(NamedAttr \"" + field.name.str() + "\" (TypeAttr ";
        if (auto bitsType = field.type.dyn_cast<mlir::edamlir::bf3drmt::BitsType>()) {
            fieldStr += "(bf3drmt_bits " + std::to_string(bitsType.getWidth()) + ")";
        } else {
            std::string typeStr;
            llvm::raw_string_ostream rso(typeStr);
            field.type.print(rso);
            rso.flush();
            fieldStr += typeStr;
        }
        fieldStr += "))";
        split.push_back(fieldStr);
    }

    return split;
}

mlir::Type parseBf3DrmtStructType(const std::vector<std::string> &split, Egglog &egglog) {
  assert(split[0] == "bf3drmt_struct");

  llvm::outs() << " >> Parsing Bf3Drmt struct type: \n";

  // First NamedAttr is the struct name
  std::string structName;
  llvm::SmallVector<mlir::edamlir::bf3drmt::FieldInfo> fields;

  for (size_t i = 1; i < split.size(); i++) {
    std::string fieldStr = split[i];

    size_t namedAttrPos = fieldStr.find("NamedAttr");
    if (namedAttrPos == std::string::npos)
      continue;

    // ---- Extract key ----
    size_t nameStart = fieldStr.find('"', namedAttrPos);
    size_t nameEnd = fieldStr.find('"', nameStart + 1);
    if (nameStart == std::string::npos || nameEnd == std::string::npos)
      continue;

    std::string key = fieldStr.substr(nameStart + 1, nameEnd - nameStart - 1);

    // ---- Special case: struct name ----
    if (key == "name") {
      size_t strStart = fieldStr.find('"', nameEnd + 1);
      size_t strEnd = fieldStr.find('"', strStart + 1);
      if (strStart != std::string::npos && strEnd != std::string::npos) {
        structName = fieldStr.substr(strStart + 1, strEnd - strStart - 1);
      }
      continue;
    }

    // ---- Otherwise it's a field ----
    // Extract type string inside (TypeAttr ...)
    size_t typeAttrPos = fieldStr.find("TypeAttr", nameEnd);
    if (typeAttrPos == std::string::npos)
      continue;

    size_t typeStart = fieldStr.find('(', typeAttrPos);
    if (typeStart == std::string::npos)
      continue;

    int depth = 0;
    size_t typeEnd = typeStart;
    for (size_t j = typeStart; j < fieldStr.size(); j++) {
      if (fieldStr[j] == '(')
        depth++;
      else if (fieldStr[j] == ')')
        depth--;
      if (depth == 0) {
        typeEnd = j;
        break;
      }
    }

    std::string typeStr =
        fieldStr.substr(typeStart + 1, typeEnd - typeStart - 1);

    // ---- Parse field type ----
    mlir::Type fieldType;

    if (typeStr.find("bf3drmt_header") == 0) {
      // Collect header tokens
      std::vector<std::string> headerTokens;
      headerTokens.push_back("bf3drmt_header");

      size_t pos = 0;
      while ((pos = typeStr.find("(NamedAttr", pos)) != std::string::npos) {
        int d = 0;
        size_t start = pos, end = pos;
        for (size_t k = pos; k < typeStr.size(); k++) {
          if (typeStr[k] == '(')
            d++;
          else if (typeStr[k] == ')')
            d--;
          if (d == 0) {
            end = k + 1;
            break;
          }
        }
        headerTokens.push_back(typeStr.substr(start, end - start));
        pos = end;
      }

      fieldType = parseBf3DrmtHeaderType(headerTokens, egglog);

    } else if (typeStr.find("bf3drmt_bits") == 0) {
      // p4hir_bits N
      size_t spacePos = typeStr.find(' ');
      if (spacePos != std::string::npos) {
        std::string widthStr = typeStr.substr(spacePos + 1);
        size_t parenPos = widthStr.find(')');
        if (parenPos != std::string::npos)
          widthStr = widthStr.substr(0, parenPos);

        unsigned width = static_cast<unsigned>(std::stoi(widthStr));
        std::string strType = "!bf3drmt.bit<" + std::to_string(width) + ">";
        fieldType = mlir::parseType(strType, &egglog.context);
      }
    } else {
      // Fallback: regular MLIR type
      fieldType = mlir::parseType(typeStr, &egglog.context);
    }

    if (fieldType) {
      fields.push_back(
          {mlir::StringAttr::get(&egglog.context, key), fieldType});
    }
  }

  mlir::DictionaryAttr annots = mlir::DictionaryAttr::get(&egglog.context, {});
  return mlir::edamlir::bf3drmt::StructType::get(&egglog.context, structName, fields, annots);
}

std::vector<std::string> stringifyBf3DrmtStructType(mlir::Type type, Egglog &egglog) {
    llvm::outs() << "[stringifyBf3DrmtStructType] type = " << type << "\n";
  std::vector<std::string> split;
  auto structType = type.cast<mlir::edamlir::bf3drmt::StructType>();

  split.push_back("bf3drmt_struct");

  // First, emit the struct name
  split.push_back("(NamedAttr \"name\" (StringAttr \"" +
                  structType.getName().str() + "\"))");

  // Then, emit fields
  for (auto field : structType.getFields()) {
    std::string fieldStr = "(NamedAttr \"" + field.name.str() +
                           "\" (TypeAttr ";

    if (auto hdrType =
            field.type.dyn_cast<mlir::edamlir::bf3drmt::HeaderType>()) {
      // Delegate header
      auto sub = stringifyBf3DrmtHeaderType(hdrType, egglog);
      fieldStr += "(" + llvm::join(sub, " ") + ")";
    } else if (auto bitsType =
                   field.type.dyn_cast<mlir::edamlir::bf3drmt::BitsType>()) {
      fieldStr += "(bf3drmt_bits " +
                  std::to_string(bitsType.getWidth()) + ")";
    } else {
      std::string typeStr;
      llvm::raw_string_ostream rso(typeStr);
      field.type.print(rso);
      rso.flush();
      fieldStr += typeStr;
    }

    fieldStr += "))";
    split.push_back(fieldStr);
  }

  return split;
}

/** Parse Bf3Drmt reference type (function p4hir_ref (Type) Type) */
mlir::Type parseBf3DrmtReferenceType(const std::vector<std::string>& split, Egglog& egglog) {
    std::string typeType = split[0];
    assert(typeType == "bf3drmt_ref");

    llvm::outs() << " >> Parsing Bf3Drmt reference type: \n";

    // Parse inner type parameter
    std::string innerTypeStr = split[1];
    
    // Use egglog's parseType method like in parseRankedTensorType
    mlir::Type innerType = egglog.parseType(innerTypeStr);
    if (!innerType) {
        llvm::errs() << "Failed to parse inner type: " << innerTypeStr << "\n";
        return nullptr;
    }

    // Create reference type using P4HIR's RefType::get
    return mlir::edamlir::bf3drmt::ReferenceType::get(innerType);
}

/** Stringify P4HIR reference type */
std::vector<std::string> stringifyBf3DrmtReferenceType(mlir::Type type, Egglog& egglog) {
    std::vector<std::string> split;
    
    // Get the inner type from the reference type
    auto refType = type.cast<mlir::edamlir::bf3drmt::ReferenceType>();
    mlir::Type innerType = refType.getObjectType();

    // Get the string representation of the inner type using egglog's eggifyType
    std::string innerTypeStr = egglog.eggifyType(innerType);
    
    split.push_back("bf3drmt_ref");
    // Add the inner type WITHOUT extra parentheses
    split.push_back(innerTypeStr);  // Remove the extra parentheses

    return split;
}