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

/// Represents a piece of debug metadata attached to a Value.
/// @see llvm::Metadata
typedef struct LLVMOpaqueMetadata *LLVMMetadataRef;

/// Represents a builder that can create debug metadata.
/// @see llvm::DIBuilder
typedef struct LLVMOpaqueDIBuilder *LLVMDIBuilderRef;

#ifdef __cplusplus
extern "C" {
#endif

/// Debug info flags.
typedef enum {
#define HANDLE_DI_FLAG(ID, NAME) LLVMDIFlag##NAME = ID,
    #include "llvm/IR/DebugInfoFlags.def"
    LLVMDIFlagAccessibility = LLVMDIFlagPrivate
                            | LLVMDIFlagProtected
                            | LLVMDIFlagPublic
} LLVMDIFlags;

/// Source languages known by DWARF.
typedef enum {
#define HANDLE_DW_LANG(ID, NAME) LLVMDWARFSourceLanguage##NAME = ID,
    #include "llvm/Support/Dwarf.def"
} LLVMDWARFSourceLanguage;

/// Qualifiers for types, e.g. \c const.
typedef enum {
#define HANDLE_DW_TAG(ID, NAME) LLVMDWARFTypeQualifier##NAME = ID,
    #include "llvm/Support/Dwarf.def"
} LLVMDWARFTypeQualifierTag;

/// Special encodings for known types in DWARF.
typedef enum {
#define HANDLE_DW_ATE(ID, NAME) LLVMDWARFTypeEncoding_##NAME = ID,
    #include "llvm/Support/Dwarf.def"
} LLVMDWARFTypeEncoding;

/// The amount of debug information to emit.
typedef enum {
    LLVMDWARFEmissionNone = 0,
    LLVMDWARFEmissionFull,
    LLVMDWARFEmissionLineTablesOnly
} LLVMDWARFEmissionKind;

/// The current debug metadata version number.
uint32_t LLVMDebugMetadataVersion();

/// The version of debug metadata that's present in the provided \c Module.
unsigned LLVMGetModuleDebugMetadataVersion(LLVMModuleRef Module);

/// \brief Strip debug info in the module if it exists.
///
/// To do this, we remove all calls to the debugger intrinsics and any named
/// metadata for debugging. We also remove debug locations for instructions.
/// Return true if module is modified.
uint8_t LLVMStripModuleDebugInfo(LLVMModuleRef Module);

/// \brief Find subprogram that is enclosing this scope.
LLVMMetadataRef
LLVMGetSubprogram(LLVMMetadataRef Scope);

/// Construct a builder for a module.
///
/// If \c AllowUnresolved, collect unresolved nodes attached to the module
/// in order to resolve cycles during a call to \c LLVMDIBuilderFinalize.
LLVMDIBuilderRef LLVMDIBuilderCreate(LLVMModuleRef M, uint8_t AllowUnresolved);

/// Deallocates the DIBuilder and everything it owns.
void LLVMDIBuilderDispose(LLVMDIBuilderRef Builder);

/// Construct any deferred debug info descriptors.
void LLVMDIBuilderFinalize(LLVMDIBuilderRef Builder);

/// A CompileUnit provides an anchor for all debugging
/// information generated during this instance of compilation.
/// \param Lang          Source programming language, eg.
///                      \c LLVMDWARFSourceLanguageC99
/// \param File          File info.
/// \param Producer      Identify the producer of debugging information
///                      and code.  Usually this is a compiler
///                      version string.
/// \param isOptimized   A boolean flag which indicates whether optimization
///                      is enabled or not.
/// \param Flags         This string lists command line options. This
///                      string is directly embedded in debug info
///                      output which may be used by a tool
///                      analyzing generated debugging information.
/// \param RuntimeVer    This indicates runtime version for languages like
///                      Objective-C.
/// \param SplitName     The name of the file that we'll split debug info
///                      out into.
/// \param Kind          The kind of debug information to generate.
/// \param DWOId         The DWOId if this is a split skeleton compile unit.
/// \param SplitDebugInlining    Whether to emit inline debug info.
/// \param DebugInfoForProfiling Whether to emit extra debug info for
///                              profile collection.
LLVMMetadataRef LLVMDIBuilderCreateCompileUnit(
    LLVMDIBuilderRef Builder, LLVMDWARFSourceLanguage Lang,
    LLVMMetadataRef FileRef, const char *Producer, uint8_t isOptimized,
    const char *Flags, unsigned RuntimeVer, const char *SplitName,
    LLVMDWARFEmissionKind Kind, uint64_t DWOId, uint8_t SplitDebugInlining,
    uint8_t DebugInfoForProfiling);

/// Create a file descriptor to hold debugging information for a file.
/// \param Builder   The DIBuilder.
/// \param Filename  File name.
/// \param Directory Directory.
LLVMMetadataRef
LLVMDIBuilderCreateFile(LLVMDIBuilderRef Builder, const char *Filename,
                        const char *Directory);

/// Create debugging information temporary entry for a macro file.
/// List of macro node direct children will be calculated by DIBuilder,
/// using the \p Parent relationship.
/// \param Builder   The DIBuilder.
/// \param Parent     Macro file parent (could be NULL).
/// \param Line       Source line number where the macro file is included.
/// \param File       File descriptor containing the name of the macro file.
LLVMMetadataRef
LLVMDIBuilderCreateTempMacroFile(LLVMDIBuilderRef Builder,
                                 LLVMMetadataRef ParentMacroFile,
                                 unsigned Line, LLVMMetadataRef File);

/// Create subroutine type.
/// \param ParameterTypes  An array of subroutine parameter types. This
///                        includes return type at 0th index.
/// \param Flags           E.g.: LValueReference.
///                        These flags are used to emit dwarf attributes.
LLVMMetadataRef
LLVMDIBuilderCreateSubroutineType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef File,
                                  LLVMMetadataRef *ParameterTypes,
                                  unsigned NumParameterTypes);

/// Create a new descriptor for the specified subprogram.
/// See comments in DISubprogram* for descriptions of these fields.
/// \param Builder        The DIBuilder.
/// \param Scope          Function scope.
/// \param Name           Function name.
/// \param LinkageName    Mangled function name.
/// \param File           File where this variable is defined.
/// \param LineNo         Line number.
/// \param Ty             Function type.
/// \param IsLocalToUnit  True if this function is not externally visible.
/// \param IsDefinition   True if this is a function definition.
/// \param ScopeLine      Set to the beginning of the scope this starts
/// \param Flags          e.g. is this function prototyped or not.
///                       These flags are used to emit dwarf attributes.
/// \param IsOptimized    True if optimization is ON.
/// \param TemplateParams Function template parameters.
LLVMMetadataRef LLVMDIBuilderCreateFunction(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef *TemplateParams,
    unsigned NumTemplateParams, LLVMMetadataRef Decl);


/// Identical to LLVMDIBuilderCreateFunction,
/// except that the resulting DbgNode is meant to be RAUWed.
LLVMMetadataRef
LLVMDIBuilderCreateTempFunctionFwdDecl(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef *TemplateParams,
    unsigned NumTemplateParams, LLVMMetadataRef Decl);

/// Create C++11 nullptr type.
LLVMMetadataRef LLVMDIBuilderCreateNullPtrType(LLVMDIBuilderRef Builder);

/// This creates new descriptor for a module with the specified
/// parent scope.
/// \param Builder     The DIBuilder.
/// \param Scope       Parent scope
/// \param Name        Name of this module
/// \param ConfigurationMacros
///                    A space-separated shell-quoted list of -D macro
///                    definitions as they would appear on a command line.
/// \param IncludePath The path to the module map file.
/// \param ISysRoot    The clang system root (value of -isysroot).
LLVMMetadataRef
LLVMDIBuilderCreateModule(LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
                          const char *Name, const char *ConfigurationMacros,
                          const char *IncludePath, const char *ISysRoot);

/// Create debugging information entry for a class.
/// \param Scope        Scope in which this class is defined.
/// \param Name         class name.
/// \param File         File where this member is defined.
/// \param LineNumber   Line number.
/// \param SizeInBits   Member size.
/// \param AlignInBits  Member alignment.
/// \param OffsetInBits Member offset.
/// \param Flags        Flags to encode member attribute, e.g. private
/// \param Elements     class members.
/// \param DerivedFrom  Debug info of the base class of this type.
/// \param TemplateParms Template type parameters.
LLVMMetadataRef LLVMDIBuilderCreateClassType(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, LLVMMetadataRef File,
    unsigned LineNumber, uint64_t SizeInBits, uint32_t AlignInBits,
    uint64_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef *Elements,
    unsigned NumElements, LLVMMetadataRef DerivedFrom,
    LLVMMetadataRef TemplateParamsNode);

/// Create a new DIType* with "artificial" flag set.
LLVMMetadataRef
LLVMDIBuilderCreateArtificialType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef Type);

/// Create a new descriptor for an auto variable.  This is a local variable
/// that is not a subprogram parameter.
///
/// \c Scope must be a \a DILocalScope, and thus its scope chain eventually
/// leads to a \a DISubprogram.
///
/// If \c AlwaysPreserve, this variable will be referenced from its
/// containing subprogram, and will survive some optimizations.
LLVMMetadataRef
LLVMDIBuilderCreateAutoVariable(LLVMDIBuilderRef Builder,
                                LLVMMetadataRef Scope, const char *Name,
                                LLVMMetadataRef File, unsigned LineNo,
                                LLVMMetadataRef Type, uint8_t AlwaysPreserve,
                                LLVMDIFlags Flags, uint32_t AlignInBits);

/// Create a new descriptor for the specified variable.
/// \param Context     Variable scope.
/// \param Name        Name of the variable.
/// \param LinkageName Mangled  name of the variable.
/// \param File        File where this variable is defined.
/// \param LineNo      Line number.
/// \param Ty          Variable Type.
/// \param isLocalToUnit Boolean flag indicate whether this variable is
///                      externally visible or not.
/// \param Expr        The location of the global relative to the attached
///                    GlobalVariable.
/// \param Decl        Reference to the corresponding declaration.
/// \param AlignInBits Variable alignment(or 0 if no alignment attr was
///                    specified)
LLVMMetadataRef
LLVMDIBuilderCreateGlobalVariableExpression(
    LLVMDIBuilderRef Builder,  LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNumber,
    LLVMMetadataRef Ty, uint8_t isLocalToUnit, LLVMMetadataRef Expr);


/// Create debugging information entry for a bit field member.
/// \param Builder             The DIBuilder.
/// \param Scope               Member scope.
/// \param Name                Member name.
/// \param File                File where this member is defined.
/// \param LineNo              Line number.
/// \param SizeInBits          Member size.
/// \param OffsetInBits        Member offset.
/// \param StorageOffsetInBits Member storage offset.
/// \param Flags               Flags to encode member attribute.
/// \param Type                Parent type.
LLVMMetadataRef
LLVMDIBuilderCreateBitFieldMemberType(LLVMDIBuilderRef Builder,
                                      LLVMMetadataRef Scope,
                                      const char *Name, LLVMMetadataRef File,
                                      unsigned LineNumber, uint64_t SizeInBits,
                                      uint64_t OffsetInBits,
                                      uint64_t StorageOffsetInBits,
                                      LLVMDIFlags Flags, LLVMMetadataRef Type);

/// Create a permanent forward-declared type.
LLVMMetadataRef
LLVMDIBuilderCreateForwardDecl(LLVMDIBuilderRef Builder, unsigned Tag,
                               const char *Name, LLVMMetadataRef Scope,
                               LLVMMetadataRef File, unsigned Line);

/// Create a new descriptor for the specified C++ method.
/// See comments in \a DISubprogram* for descriptions of these fields.
/// \param Scope          Function scope.
/// \param Name           Function name.
/// \param LinkageName    Mangled function name.
/// \param File           File where this variable is defined.
/// \param LineNo         Line number.
/// \param FuncTy         Function type.
/// \param Flags          e.g. is this function prototyped or not.
///                       This flags are used to emit dwarf attributes.
/// \param IsLocalToUnit  True if this function is not externally visible..
/// \param IsDefinition   True if this is a function definition.
/// \param IsOptimized    True if optimization is ON.
/// \param TParams        Function template parameters.
LLVMMetadataRef
LLVMDIBuilderCreateMethod(LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
                          const char *Name, const char *LinkageName,
                          LLVMMetadataRef File, unsigned LineNumber,
                          LLVMMetadataRef FuncTy,
                          LLVMDIFlags Flags, uint8_t IsLocalToUnit,
                          uint8_t IsDefinition, uint8_t IsOptimized,
                          LLVMMetadataRef *TemplateParameters,
                          unsigned NumTemplateParameters);

/// Create debugging information entry for a typedef.
/// \param Builder     The DIBuilder.
/// \param Ty          Original type.
/// \param Name        Typedef name.
/// \param File        File where this type is defined.
/// \param LineNo      Line number.
/// \param Scope       The surrounding context for the typedef.
LLVMMetadataRef
LLVMDIBuilderCreateTypedef(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                           const char *Name, LLVMMetadataRef File,
                           unsigned Line, LLVMMetadataRef Scope);

/// Create debugging information entry to establish
/// inheritance relationship between two types.
/// \param Builder      The DIBuilder.
/// \param Type         Original type.
/// \param BaseType     Base type from which \c Type inherits.
/// \param BaseOffset   Base offset.
/// \param Flags        Flags to describe inheritance attribute,
///                     e.g. private
LLVMMetadataRef
LLVMDIBuilderCreateInheritance(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                               LLVMMetadataRef BaseType, uint64_t BaseOffset,
                               LLVMDIFlags Flags);

/// Create debugging information entry for a macro.
/// \param Builder    The DIBuilder.
/// \param Parent     Macro parent (could be nullptr).
/// \param Line       Source line number where the macro is defined.
/// \param MacroType  DW_MACINFO_define or DW_MACINFO_undef.
/// \param Name       Macro name.
/// \param Value      Macro value.
LLVMMetadataRef
LLVMDIBuilderCreateMacro(LLVMDIBuilderRef Builder,
                         LLVMMetadataRef ParentMacroFile, unsigned Line,
                         unsigned MacroType, const char *Name);

/// Create debugging information entry for a 'friend'.
LLVMMetadataRef
LLVMDIBuilderCreateFriend(LLVMDIBuilderRef Builder,
                          LLVMMetadataRef Type,
                          LLVMMetadataRef FriendType);

/// Create debugging information entry for Objective-C
/// instance variable.
/// \param Builder      The DIBuilder.
/// \param Name         Member name.
/// \param File         File where this member is defined.
/// \param LineNo       Line number.
/// \param SizeInBits   Member size.
/// \param AlignInBits  Member alignment.
/// \param OffsetInBits Member offset.
/// \param Flags        Flags to encode member attribute, e.g. private
/// \param Ty           Parent type.
/// \param PropertyNode Property associated with this ivar.
LLVMMetadataRef
LLVMDIBuilderCreateObjCIVar(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, uint64_t SizeInBits, uint32_t AlignInBits,
    uint32_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef Type,
    LLVMMetadataRef PropertyOrNull);

/// Create debugging information entry for Objective-C
/// property.
/// \param Builder      The DIBuilder.
/// \param Name         Property name.
/// \param File         File where this property is defined.
/// \param LineNumber   Line number.
/// \param GetterName   Name of the Objective C property getter selector.
/// \param SetterName   Name of the Objective C property setter selector.
/// \param PropertyAttributes Objective C property attributes.
/// \param Ty           Type.
LLVMMetadataRef
LLVMDIBuilderCreateObjCProperty(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, const char *GetterName, const char *SetterName,
    unsigned PropertyAttributes, LLVMMetadataRef Type);

/// Create debugging information entry for a basic
/// type.
/// \param Builder     The DIBuilder.
/// \param Name        Type name.
/// \param SizeInBits  Size of the type.
/// \param Encoding    DWARF encoding code, e.g. \c LLVMDWARFTypeEncoding_float.
LLVMMetadataRef
LLVMDIBuilderCreateBasicType(LLVMDIBuilderRef Builder, const char *Name,
                             uint64_t SizeInBits,
                             LLVMDWARFTypeEncoding Encoding);

/// Create debugging information entry for a pointer.
/// \param Builder     The DIBuilder.
/// \param PointeeTy         Type pointed by this pointer.
/// \param SizeInBits        Size.
/// \param AlignInBits       Alignment. (optional, pass 0 to ignore)
/// \param DWARFAddressSpace DWARF address space. (optional, pass 0 to ignore)
/// \param Name              Pointer type name. (optional)
LLVMMetadataRef LLVMDIBuilderCreatePointerType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef PointeeTy,
    uint64_t SizeInBits, uint32_t AlignInBits, unsigned AddressSpace,
    const char *Name);

/// Create debugging information entry for a struct.
/// \param Builder     The DIBuilder.
/// \param Scope        Scope in which this struct is defined.
/// \param Name         Struct name.
/// \param File         File where this member is defined.
/// \param LineNumber   Line number.
/// \param SizeInBits   Member size.
/// \param AlignInBits  Member alignment.
/// \param Flags        Flags to encode member attribute, e.g. private
/// \param Elements     Struct elements.
/// \param RunTimeLang  Optional parameter, Objective-C runtime version.
/// \param VTableHolder The object containing the vtable for the struct.
/// \param UniqueIdentifier A unique identifier for the struct.
LLVMMetadataRef LLVMDIBuilderCreateStructType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags,
    LLVMMetadataRef DerivedFrom, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, LLVMMetadataRef VTableHolder,
    const char *UniqueId);

/// Create debugging information entry for a member.
/// \param Builder      The DIBuilder.
/// \param Scope        Member scope.
/// \param Name         Member name.
/// \param File         File where this member is defined.
/// \param LineNo       Line number.
/// \param SizeInBits   Member size.
/// \param AlignInBits  Member alignment.
/// \param OffsetInBits Member offset.
/// \param Flags        Flags to encode member attribute, e.g. private
/// \param Ty           Parent type.
LLVMMetadataRef LLVMDIBuilderCreateMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNo, uint64_t SizeInBits,
    uint32_t AlignInBits, uint64_t OffsetInBits, LLVMDIFlags Flags,
    LLVMMetadataRef Ty);


/// Create debugging information entry for a
/// C++ static data member.
/// \param Builder      The DIBuilder.
/// \param Scope        Member scope.
/// \param Name         Member name.
/// \param File         File where this member is declared.
/// \param LineNo       Line number.
/// \param Ty           Type of the static member.
/// \param Flags        Flags to encode member attribute, e.g. private.
/// \param Val          Const initializer of the member.
/// \param AlignInBits  Member alignment.
LLVMMetadataRef
LLVMDIBuilderCreateStaticMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, LLVMMetadataRef Type,
    LLVMDIFlags Flags, LLVMValueRef ConstantVal, uint32_t AlignInBits);


/// Create debugging information entry for a pointer to member.
/// \param Builder      The DIBuilder.
/// \param PointeeType Type pointed to by this pointer.
/// \param Class Type for which this pointer points to members of.
/// \param SizeInBits  Size.
/// \param AlignInBits Alignment. (optional)
/// \param Flags Flags.
LLVMMetadataRef
LLVMDIBuilderCreateMemberPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef PointeeType,
                                     LLVMMetadataRef ClassType,
                                     uint64_t SizeInBits,
                                     uint32_t AlignInBits,
                                     LLVMDIFlags Flags);

/// Create a new DIType* with the "object pointer"
/// flag set.
LLVMMetadataRef
LLVMDIBuilderCreateObjectPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef Type);

/// Create debugging information entry for a qualified
/// type, e.g. 'const int'.
/// \param Tag         Tag identifing type,
///                    e.g. LLVMDWARFTypeQualifier_volatile_type
/// \param FromTy      Base Type.
LLVMMetadataRef
LLVMDIBuilderCreateQualifiedType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type);


/// Create debugging information entry for a c++
/// style reference or rvalue reference type.
LLVMMetadataRef
LLVMDIBuilderCreateReferenceType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type);

/// Create C++11 nullptr type.
LLVMMetadataRef
LLVMDIBuilderCreateNullPtrType(LLVMDIBuilderRef Builder);

/// Create a temporary forward-declared type.
LLVMMetadataRef
LLVMDIBuilderCreateReplaceableCompositeType(
    LLVMDIBuilderRef Builder, unsigned Tag, const char *Name,
    LLVMMetadataRef Scope, LLVMMetadataRef File, unsigned Line);

/// Create unspecified parameter type
/// for a subroutine type.
LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedParameter(LLVMDIBuilderRef Builder);

/// Create a DWARF unspecified type.
LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedType(LLVMDIBuilderRef Builder, const char *Name);

/// This creates a descriptor for a lexical block with the
/// specified parent context.
/// \param Builder      The DIBuilder.
/// \param Scope         Parent lexical scope.
/// \param File          Source file.
/// \param Line          Line number.
/// \param Col           Column number.
LLVMMetadataRef LLVMDIBuilderCreateLexicalBlock(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
    LLVMMetadataRef File, unsigned Line, unsigned Col);

/// Create a new descriptor for a parameter variable.
///
/// \c Scope must be a \a DILocalScope, and thus its scope chain eventually
/// leads to a \a DISubprogram.
///
/// \c ArgNo is the index (starting from \c 1) of this variable in the
/// subprogram parameters.  \c ArgNo should not conflict with other
/// parameters of the same subprogram.
///
/// If \c AlwaysPreserve, this variable will be referenced from its
/// containing subprogram, and will survive some optimizations.
LLVMMetadataRef
LLVMDIBuilderCreateParameterVariable(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, unsigned ArgNum,
    LLVMMetadataRef File, unsigned LineNum, LLVMMetadataRef Type,
    uint8_t AlwaysPreserve, LLVMDIFlags Flags);

/// This creates a descriptor for a lexical block with a new file
/// attached. This merely extends the existing
/// lexical block as it crosses a file.
/// \param Builder      The DIBuilder.
/// \param Scope       Lexical block.
/// \param File        Source file.
/// \param Discriminator DWARF path discriminator value.
LLVMMetadataRef
LLVMDIBuilderCreateLexicalBlockFile(LLVMDIBuilderRef Builder,
                                    LLVMMetadataRef Scope,
                                    LLVMMetadataRef File);

/// Create an expression for a variable that does not have an address, but
/// does have a constant value.
LLVMMetadataRef
LLVMDIBuilderCreateConstantValueExpression(LLVMDIBuilderRef Builder,
                                           uint64_t Val);

/// Create a descriptor to describe one part
/// of aggregate variable that is fragmented across multiple Values.
///
/// \param Builder      The DIBuilder.
/// \param OffsetInBits Offset of the piece in bits.
/// \param SizeInBits   Size of the piece in bits.
LLVMMetadataRef
LLVMDIBuilderCreateFragmentExpression(LLVMDIBuilderRef Builder,
                                      unsigned OffsetInBits,
                                      unsigned SizeInBits);

/// Create a new descriptor for the specified
/// variable which has a complex address expression for its address.
/// \param Builder      The DIBuilder.
/// \param Addrs        One or more complex address operations.
/// \param NumAddrs     The number of addresses that \c Addrs points to.
LLVMMetadataRef
LLVMDIBuilderCreateExpression(LLVMDIBuilderRef Builder, int64_t *Addrs,
                              unsigned NumAddrs);

/// Create debugging information entry for an array.
/// \param Builder      The DIBuilder.
/// \param Size         Array size.
/// \param AlignInBits  Alignment.
/// \param Ty           Element type.
/// \param Subscripts   Subscripts.
LLVMMetadataRef
LLVMDIBuilderCreateArrayType(LLVMDIBuilderRef Builder, uint64_t Size,
                             uint32_t AlignInBits, LLVMMetadataRef Ty,
                             LLVMMetadataRef *Subscripts,
                             unsigned NumSubscripts);

/// Create debugging information entry for a vector type.
/// \param Size         Array size.
/// \param AlignInBits  Alignment.
/// \param Ty           Element type.
/// \param Subscripts   Subscripts.
LLVMMetadataRef
LLVMDIBuilderCreateVectorType(LLVMDIBuilderRef Builder, uint64_t Size,
                              uint32_t AlignInBits, LLVMMetadataRef Ty,
                              LLVMMetadataRef *Subscripts,
                              unsigned NumSubscripts);

/// Create a descriptor for a value range.  This
/// implicitly uniques the values returned.
LLVMMetadataRef
LLVMDIBuilderGetOrCreateSubrange(LLVMDIBuilderRef Builder, int64_t Lo,
                                 int64_t Count);

/// Get a DINodeArray, create one if required.
LLVMMetadataRef
LLVMDIBuilderGetOrCreateArray(LLVMDIBuilderRef Builder,
                              LLVMMetadataRef *Ptr, unsigned Count);

/// Insert a new llvm.dbg.declare intrinsic call.
/// \param Builder     The DIBuilder.
/// \param Storage     LLVMValueRef of the variable
/// \param VarInfo     Variable's debug info descriptor.
/// \param Expr        A complex location expression.
/// \param DL          Debug info location.
/// \param InsertAtEnd Location for the new intrinsic.
LLVMValueRef LLVMDIBuilderInsertDeclareAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef V, LLVMMetadataRef VarInfo,
    int64_t *AddrOps, unsigned AddrOpsCount, LLVMMetadataRef DL,
    LLVMBasicBlockRef InsertAtEnd);

/// Insert a new llvm.dbg.value intrinsic call.
/// \param Builder      The DIBuilder.
/// \param Val          LLVMValueRef of the variable
/// \param Offset       Offset
/// \param VarInfo      Variable's debug info descriptor.
/// \param Expr         A complex location expression.
/// \param DL           Debug info location.
/// \param InsertAtEnd Location for the new intrinsic.
LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMBasicBlockRef InsertAtEnd);

/// Insert a new llvm.dbg.value intrinsic call.
/// \param Builder      The DIBuilder.
/// \param Val          LLVMValueRef of the variable
/// \param Offset       Offset
/// \param VarInfo      Variable's debug info descriptor.
/// \param Expr         A complex location expression.
/// \param DL           Debug info location.
/// \param InsertBefore Location for the new intrinsic.
LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicBefore(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMValueRef InsertBefore);

/// Create a single enumerator value.
LLVMMetadataRef
LLVMDIBuilderCreateEnumerator(LLVMDIBuilderRef Builder,
                              const char *Name, int64_t Val);

/// Create debugging information entry for an
/// enumeration.
/// \param Builder        The DIBuilder.
/// \param Scope          Scope in which this enumeration is defined.
/// \param Name           Union name.
/// \param File           File where this member is defined.
/// \param LineNumber     Line number.
/// \param SizeInBits     Member size.
/// \param AlignInBits    Member alignment.
/// \param Elements       Enumeration elements.
/// \param NumElements    Number of enumeration elements.
/// \param UnderlyingType Underlying type of a C++11/ObjC fixed enum.
/// \param UniqueIdentifier A unique identifier for the enum.
LLVMMetadataRef LLVMDIBuilderCreateEnumerationType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMMetadataRef *Elements, unsigned NumElements,
    LLVMMetadataRef ClassTy);

/// Create debugging information entry for an union.
/// \param Builder      The DIBuilder.
/// \param Scope        Scope in which this union is defined.
/// \param Name         Union name.
/// \param File         File where this member is defined.
/// \param LineNumber   Line number.
/// \param SizeInBits   Member size.
/// \param AlignInBits  Member alignment.
/// \param Flags        Flags to encode member attribute, e.g. private
/// \param Elements     Union elements.
/// \param NumElements  Number of union elements.
/// \param RunTimeLang  Optional parameter, Objective-C runtime version.
/// \param UniqueIdentifier A unique identifier for the union.
LLVMMetadataRef LLVMDIBuilderCreateUnionType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, const char *UniqueId);

/// Create debugging information for template
/// type parameter.
/// \param Builder      The DIBuilder.
/// \param Scope        Scope in which this type is defined.
/// \param Name         Type parameter name.
/// \param Ty           Parameter type.
LLVMMetadataRef LLVMDIBuilderCreateTemplateTypeParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Ty);

/// Create debugging information for template
/// value parameter.
/// \param Builder      The DIBuilder.
/// \param Scope        Scope in which this type is defined.
/// \param Name         Value parameter name.
/// \param Ty           Parameter type.
/// \param Val          Constant parameter value (optional).
LLVMMetadataRef
LLVMDIBuilderCreateTemplateValueParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Type, LLVMValueRef ConstantValueOrNull);

/// Create debugging information for a template template parameter.
/// \param Builder      The DIBuilder.
/// \param Scope        Scope in which this type is defined.
/// \param Name         Value parameter name.
/// \param Ty           Parameter type.
/// \param Val          The fully qualified name of the template.
LLVMMetadataRef
LLVMDIBuilderCreateTemplateTemplateParameter(
     LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
     LLVMMetadataRef Type, const char *Str);

/// This creates new descriptor for a namespace with the specified
/// parent scope.
/// \param Builder      The DIBuilder.
/// \param Scope       Namespace scope
/// \param Name        Name of this namespace
/// \param File        Source file
/// \param LineNo      Line number
/// \param ExportSymbols True for C++ inline namespaces.
LLVMMetadataRef
LLVMDIBuilderCreateNameSpace(LLVMDIBuilderRef Builder,
                             LLVMMetadataRef Scope, const char *Name,
                             LLVMMetadataRef File, unsigned LineNo,
                             uint8_t ExportSymbols);

/// Replace arrays on a composite type.
///
/// If \c T is resolved, but the arrays aren't -- which can happen if \c T
/// has a self-reference -- \a DIBuilder needs to track the array to
/// resolve cycles.
void LLVMDICompositeTypeSetTypeArray(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef CompositeTy,
                                     LLVMMetadataRef *Types, unsigned NumTypes);

/// Creates a new DebugLocation that describes a source location.
/// \param Line The line in the source file.
/// \param Column The column in the source file.
/// \param Scope The scope in which the location resides.
/// \param InlinedAt The scope where this location was inlined, if at all.
///                  (optional).
LLVMMetadataRef
LLVMDIBuilderCreateDebugLocation(LLVMContextRef Ctx, unsigned Line,
                                 unsigned Column, LLVMMetadataRef Scope,
                                 LLVMMetadataRef InlinedAt);

#ifdef __cplusplus
} // end extern "C"
#endif
