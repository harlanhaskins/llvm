//===------------ DebugInfo.h - LLVM C API Debug Info API -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// This file declares the C API endpoints for generating DWARF Debug Info
///
/// Note: This interface is experimental. It is *NOT* stable, and may be
///       changed without warning.
///
//===----------------------------------------------------------------------===//

#include "llvm-c/Core.h"

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
#define HANDLE_DW_LANG(ID, NAME, VERSION, VENDOR) \
    LLVMDWARFSourceLanguage##NAME = ID,
    #include "llvm/Support/Dwarf.def"
} LLVMDWARFSourceLanguage;

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

/// Strip debug info in the module if it exists.
///
/// To do this, we remove all calls to the debugger intrinsics and any named
/// metadata for debugging. We also remove debug locations for instructions.
/// Return true if module is modified.
LLVMBool LLVMStripModuleDebugInfo(LLVMModuleRef Module);

/// Construct a builder for a module, and do not allow for unresolved nodes
/// attached to the module.
LLVMDIBuilderRef LLVMCreateDIBuilderDisallowUnresolved(LLVMModuleRef M);

/// Construct a builder for a module and collect unresolved nodes attached
/// to the module in order to resolve cycles during a call to
/// \c LLVMDIBuilderFinalize.
LLVMDIBuilderRef LLVMCreateDIBuilder(LLVMModuleRef M);

/// Deallocates the DIBuilder and everything it owns.
/// @note You must call \c LLVMDIBuilderFinalize before this
void LLVMDisposeDIBuilder(LLVMDIBuilderRef Builder);

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
/// \param ProducerLen   The length of the C string passed to \c Producer.
/// \param isOptimized   A boolean flag which indicates whether optimization
///                      is enabled or not.
/// \param Flags         This string lists command line options. This
///                      string is directly embedded in debug info
///                      output which may be used by a tool
///                      analyzing generated debugging information.
/// \param FlagsLen      The length of the C string passed to \c Flags.
/// \param RuntimeVer    This indicates runtime version for languages like
///                      Objective-C.
/// \param SplitName     The name of the file that we'll split debug info
///                      out into.
/// \param SplitNameLen  The length of the C string passed to \c SplitName.
/// \param Kind          The kind of debug information to generate.
/// \param DWOId         The DWOId if this is a split skeleton compile unit.
/// \param SplitDebugInlining    Whether to emit inline debug info.
/// \param DebugInfoForProfiling Whether to emit extra debug info for
///                              profile collection.
LLVMMetadataRef LLVMDIBuilderCreateCompileUnit(
    LLVMDIBuilderRef Builder, LLVMDWARFSourceLanguage Lang,
    LLVMMetadataRef FileRef, const char *Producer, uint64_t ProducerLen,
    LLVMBool isOptimized, const char *Flags, size_t FlagsLen,
    unsigned RuntimeVer, const char *SplitName, size_t SplitNameLen,
    LLVMDWARFEmissionKind Kind, uint64_t DWOId, LLVMBool SplitDebugInlining,
    LLVMBool DebugInfoForProfiling);

/// Create a file descriptor to hold debugging information for a file.
/// \param Builder      The DIBuilder.
/// \param Filename     File name.
/// \param FilenameLen  The length of the C string passed to \c Filename.
/// \param Directory    Directory.
/// \param DirectoryLen The length of the C string passed to \c Directory.
LLVMMetadataRef
LLVMDIBuilderCreateFile(LLVMDIBuilderRef Builder, const char *Filename,
                        size_t FilenameLen, const char *Directory,
                        size_t DirectoryLen);

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
