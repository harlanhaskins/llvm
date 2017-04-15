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
    LLVMMetadataRef Elements);

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
                                  LLVMMetadataRef ParameterTypes);

LLVMMetadataRef LLVMDIBuilderCreateFunction(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef TParam, LLVMMetadataRef Decl);

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
LLVMDIBuilderCreateFriend(LLVMDIBuilderRef Builder,
                          LLVMMetadataRef Type,
                          LLVMMetadataRef FriendType);

LLVMMetadataRef
LLVMDIBuilderCreateBasicType(LLVMDIBuilderRef Builder, const char *Name,
                             uint64_t SizeInBits, unsigned Encoding);

LLVMMetadataRef LLVMDIBuilderCreatePointerType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef PointeeTy,
    uint64_t SizeInBits, uint32_t AlignInBits, const char *Name);

LLVMMetadataRef LLVMDIBuilderCreateStructType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags,
    LLVMMetadataRef DerivedFrom, LLVMMetadataRef Elements,
    unsigned RunTimeLang, LLVMMetadataRef VTableHolder, const char *UniqueId);

LLVMMetadataRef LLVMDIBuilderCreateMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNo, uint64_t SizeInBits,
    uint32_t AlignInBits, uint64_t OffsetInBits, LLVMDIFlags Flags,
    LLVMMetadataRef Ty);

LLVMMetadataRef LLVMDIBuilderCreateLexicalBlock(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
    LLVMMetadataRef File, unsigned Line, unsigned Col);

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
                             LLVMMetadataRef Subscripts);

LLVMMetadataRef
LLVMDIBuilderCreateVectorType(LLVMDIBuilderRef Builder, uint64_t Size,
                              uint32_t AlignInBits, LLVMMetadataRef Ty,
                              LLVMMetadataRef Subscripts);

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
    uint32_t AlignInBits, LLVMMetadataRef Elements,
    LLVMMetadataRef ClassTy);

LLVMMetadataRef LLVMDIBuilderCreateUnionType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags, LLVMMetadataRef Elements,
    unsigned RunTimeLang, const char *UniqueId);

LLVMMetadataRef LLVMDIBuilderCreateTemplateTypeParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Ty, LLVMMetadataRef File, unsigned LineNo,
    unsigned ColumnNo);

LLVMMetadataRef
LLVMDIBuilderCreateNameSpace(LLVMDIBuilderRef Builder,
                             LLVMMetadataRef Scope, const char *Name,
                             LLVMMetadataRef File, unsigned LineNo);

void
LLVMDICompositeTypeSetTypeArray(LLVMDIBuilderRef Builder,
                                LLVMMetadataRef CompositeTy,
                                LLVMMetadataRef TyArray);

LLVMValueRef
LLVMDIBuilderCreateDebugLocation(LLVMContextRef ContextRef, unsigned Line,
                                 unsigned Column, LLVMMetadataRef Scope,
                                 LLVMMetadataRef InlinedAt);

#ifdef __cplusplus
} // end extern "C"
#endif
