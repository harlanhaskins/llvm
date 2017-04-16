//===------------ DebugInfo.h - LLVM C API Debug Info API -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the C API endpoints for generating DWARF Debug Info
//
//===----------------------------------------------------------------------===//

#include "llvm-c/Core.h"

typedef struct LLVMOpaqueMetadata *LLVMMetadataRef;
typedef struct LLVMOpaqueDIBuilder *LLVMDIBuilderRef;
typedef struct LLVMOpaqueDIExpression *LLVMDIExpressionRef;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
#define HANDLE_DI_FLAG(ID, NAME) LLVMDIFlag##NAME = ID,
    #include "llvm/IR/DebugInfoFlags.def"
    LLVMDIFlagAccessibility = LLVMDIFlagPrivate
                            | LLVMDIFlagProtected
                            | LLVMDIFlagPublic
} LLVMDIFlags;

uint32_t LLVMDebugMetadataVersion();

void LLVMAddModuleFlag(LLVMModuleRef M, const char *Name,
                       uint32_t Value);

LLVMDIBuilderRef LLVMDIBuilderCreate(LLVMModuleRef M);

void LLVMDIBuilderDispose(LLVMDIBuilderRef Builder);

void LLVMDIBuilderFinalize(LLVMDIBuilderRef Builder);

LLVMMetadataRef LLVMDIBuilderCreateClassType(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, LLVMMetadataRef File,
    unsigned LineNumber, uint64_t SizeInBits, uint32_t AlignInBits,
    uint64_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef DerivedFrom,
    LLVMMetadataRef *Elements, unsigned NumElements);

LLVMMetadataRef LLVMDIBuilderCreateCompileUnit(
    LLVMDIBuilderRef Builder, unsigned Lang, LLVMMetadataRef FileRef,
    const char *Producer, uint8_t isOptimized, const char *Flags,
    unsigned RuntimeVer, const char *SplitName);

LLVMMetadataRef
LLVMDIBuilderCreateFile(LLVMDIBuilderRef Builder, const char *Filename,
                        const char *Directory);

LLVMMetadataRef
LLVMDIBuilderCreateSubroutineType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef File,
                                  LLVMMetadataRef *ParameterTypes,
                                  unsigned NumParameterTypes);

LLVMMetadataRef LLVMDIBuilderCreateFunction(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef *TemplateParams,
    unsigned NumTemplateParams, LLVMMetadataRef Decl);

LLVMMetadataRef LLVMDIBuilderCreateNullPtrType(LLVMDIBuilderRef Builder);

LLVMMetadataRef
LLVMDIBuilderCreateModule(LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
                          const char *Name, const char *ConfigurationMacros,
                          const char *IncludePath, const char *ISysRoot);

LLVMMetadataRef
LLVMDIBuilderCreateArtificialType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateAutoVariable(LLVMDIBuilderRef Builder,
                                LLVMMetadataRef Scope, const char *Name,
                                LLVMMetadataRef File, unsigned LineNo,
                                LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateBitFieldMemberType(LLVMDIBuilderRef Builder,
                                      LLVMMetadataRef Scope,
                                      const char *Name, LLVMMetadataRef File,
                                      unsigned LineNumber, uint64_t SizeInBits,
                                      uint64_t OffsetInBits,
                                      uint64_t StorageOffsetInBits,
                                      LLVMDIFlags Flags, LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateForwardDecl(LLVMDIBuilderRef Builder, unsigned Tag,
                               const char *Name, LLVMMetadataRef Scope,
                               LLVMMetadataRef File, unsigned Line);

LLVMMetadataRef
LLVMDIBuilderCreateMethod(LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
                          const char *Name, const char *LinkageName,
                          LLVMMetadataRef File, unsigned LineNumber,
                          LLVMMetadataRef FuncTy, uint8_t LocalToUnit,
                          uint8_t IsDefinition);

LLVMMetadataRef
LLVMDIBuilderCreateTypedef(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                           const char *Name, LLVMMetadataRef File,
                           unsigned Line, LLVMMetadataRef Scope);

LLVMMetadataRef
LLVMDIBuilderCreateInheritance(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                               LLVMMetadataRef BaseType, uint64_t BaseOffset,
                               LLVMDIFlags Flags);

LLVMMetadataRef
LLVMDIBuilderCreateMacro(LLVMDIBuilderRef Builder,
                         LLVMMetadataRef ParentMacroFile, unsigned Line,
                         unsigned MacroType, const char *Name);

LLVMMetadataRef
LLVMDIBuilderCreateTempMacroFile(LLVMDIBuilderRef Builder,
                                 LLVMMetadataRef ParentMacroFile,
                                 unsigned Line, LLVMMetadataRef File);

LLVMMetadataRef
LLVMDIBuilderCreateFriend(LLVMDIBuilderRef Builder,
                          LLVMMetadataRef Type,
                          LLVMMetadataRef FriendType);

LLVMMetadataRef
LLVMDIBuilderCreateObjCIVar(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, uint64_t SizeInBits, uint32_t AlignInBits,
    uint32_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef Type,
    LLVMMetadataRef PropertyOrNull);

LLVMMetadataRef
LLVMDIBuilderCreateObjCProperty(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, const char *GetterName, const char *SetterName,
    unsigned PropertyAttributes, LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateBasicType(LLVMDIBuilderRef Builder, const char *Name,
                             uint64_t SizeInBits, unsigned Encoding);

LLVMMetadataRef LLVMDIBuilderCreatePointerType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef PointeeTy,
    uint64_t SizeInBits, uint32_t AlignInBits, unsigned AddressSpace,
    const char *Name);

LLVMMetadataRef LLVMDIBuilderCreateStructType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags,
    LLVMMetadataRef DerivedFrom, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, LLVMMetadataRef VTableHolder,
    const char *UniqueId);

LLVMMetadataRef LLVMDIBuilderCreateMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNo, uint64_t SizeInBits,
    uint32_t AlignInBits, uint64_t OffsetInBits, LLVMDIFlags Flags,
    LLVMMetadataRef Ty);

LLVMMetadataRef
LLVMDIBuilderCreateStaticMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, LLVMMetadataRef Type,
    LLVMDIFlags Flags, LLVMValueRef ConstantVal);

LLVMMetadataRef
LLVMDIBuilderCreateMemberPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef PointeeType,
                                     LLVMMetadataRef ClassType,
                                     uint64_t SizeInBits);

LLVMMetadataRef
LLVMDIBuilderCreateObjectPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateQualifiedType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateReferenceType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateNullPtrType(LLVMDIBuilderRef Builder);

LLVMMetadataRef
LLVMDIBuilderCreateReplaceableCompositeType(
    LLVMDIBuilderRef Builder, unsigned Tag, const char *Name,
    LLVMMetadataRef Scope, LLVMMetadataRef File, unsigned Line);

LLVMMetadataRef
LLVMDIBuilderCreateTmpFunctionFwdDecl(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned Line,
    LLVMMetadataRef Type, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine);

LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedParameter(LLVMDIBuilderRef Builder);

LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedType(LLVMDIBuilderRef Builder, const char *Name);

LLVMMetadataRef LLVMDIBuilderCreateLexicalBlock(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
    LLVMMetadataRef File, unsigned Line, unsigned Col);

LLVMMetadataRef
LLVMDIBuilderCreateParameterVariable(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, unsigned ArgNum,
    LLVMMetadataRef File, unsigned LineNum, LLVMMetadataRef Type);

LLVMMetadataRef
LLVMDIBuilderCreateLexicalBlockFile(LLVMDIBuilderRef Builder,
                                    LLVMMetadataRef Scope,
                                    LLVMMetadataRef File);

LLVMMetadataRef
LLVMDIBuilderCreateConstantValueExpression(LLVMDIBuilderRef Builder,
                                           uint64_t Val);

LLVMMetadataRef
LLVMDIBuilderCreateFragmentExpression(LLVMDIBuilderRef Builder,
                                      unsigned OffsetInBits,
                                      unsigned SizeInBits);
LLVMMetadataRef
LLVMDIBuilderCreateExpression(LLVMDIBuilderRef Builder, int64_t *Addrs,
                              unsigned NumAddrs);

LLVMMetadataRef LLVMDIBuilderCreateStaticVariable(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Context, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, LLVMValueRef V,
    LLVMMetadataRef Decl, uint32_t AlignInBits);

LLVMMetadataRef LLVMDIBuilderCreateVariable(
    LLVMDIBuilderRef Builder, unsigned Tag, LLVMMetadataRef Scope,
    const char *Name, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t AlwaysPreserve, LLVMDIFlags Flags,
    unsigned ArgNo, uint32_t AlignInBits);

LLVMMetadataRef
LLVMDIBuilderCreateArrayType(LLVMDIBuilderRef Builder, uint64_t Size,
                             uint32_t AlignInBits, LLVMMetadataRef Ty,
                             LLVMMetadataRef *Subscripts,
                             unsigned NumSubscripts);

LLVMMetadataRef
LLVMDIBuilderCreateVectorType(LLVMDIBuilderRef Builder, uint64_t Size,
                              uint32_t AlignInBits, LLVMMetadataRef Ty,
                              LLVMMetadataRef *Subscripts,
                              unsigned NumSubscripts);

LLVMMetadataRef
LLVMDIBuilderGetOrCreateSubrange(LLVMDIBuilderRef Builder, int64_t Lo,
                                 int64_t Count);

LLVMMetadataRef
LLVMDIBuilderGetOrCreateArray(LLVMDIBuilderRef Builder,
                              LLVMMetadataRef *Ptr, unsigned Count);

LLVMValueRef LLVMDIBuilderInsertDeclareAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef V, LLVMMetadataRef VarInfo,
    int64_t *AddrOps, unsigned AddrOpsCount, LLVMValueRef DL,
    LLVMBasicBlockRef InsertAtEnd);

LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMBasicBlockRef InsertAtEnd);

LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicBefore(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMValueRef InsertBefore);

LLVMMetadataRef
LLVMDIBuilderCreateEnumerator(LLVMDIBuilderRef Builder,
                              const char *Name, uint64_t Val);

LLVMMetadataRef LLVMDIBuilderCreateEnumerationType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMMetadataRef *Elements, unsigned NumElements,
    LLVMMetadataRef ClassTy);

LLVMMetadataRef LLVMDIBuilderCreateUnionType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, const char *UniqueId);

LLVMMetadataRef LLVMDIBuilderCreateTemplateTypeParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Ty);

LLVMMetadataRef
LLVMDIBuilderCreateTemplateValueParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Type, LLVMValueRef ConstantValueOrNull);

LLVMMetadataRef
LLVMDIBuilderCreateTemplateTemplateParameter(
     LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
     LLVMMetadataRef Type, const char *Str);

LLVMMetadataRef
LLVMDIBuilderCreateNameSpace(LLVMDIBuilderRef Builder,
                             LLVMMetadataRef Scope, const char *Name,
                             LLVMMetadataRef File, unsigned LineNo,
                             uint8_t ExportSymbols);

void LLVMDICompositeTypeSetTypeArray(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef CompositeTy,
                                     LLVMMetadataRef *Types, unsigned NumTypes);

LLVMValueRef
LLVMDIBuilderCreateDebugLocation(LLVMContextRef ContextRef, unsigned Line,
                                 unsigned Column, LLVMMetadataRef Scope,
                                 LLVMMetadataRef InlinedAt);

#ifdef __cplusplus
} // end extern "C"
#endif
