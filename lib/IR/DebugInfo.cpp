//===--- DebugInfo.cpp - Debug Information Helper Classes -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the helper classes used to build and interpret debug
// information in LLVM IR form.
//
//===----------------------------------------------------------------------===//

#include "llvm-c/DebugInfo.h"
#include "llvm/IR/DebugInfo.h"
#include "LLVMContextImpl.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GVMaterializer.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
using namespace llvm::dwarf;

DISubprogram *llvm::getDISubprogram(const MDNode *Scope) {
  if (auto *LocalScope = dyn_cast_or_null<DILocalScope>(Scope))
    return LocalScope->getSubprogram();
  return nullptr;
}

//===----------------------------------------------------------------------===//
// DebugInfoFinder implementations.
//===----------------------------------------------------------------------===//

void DebugInfoFinder::reset() {
  CUs.clear();
  SPs.clear();
  GVs.clear();
  TYs.clear();
  Scopes.clear();
  NodesSeen.clear();
}

void DebugInfoFinder::processModule(const Module &M) {
  for (auto *CU : M.debug_compile_units()) {
    addCompileUnit(CU);
    for (auto DIG : CU->getGlobalVariables()) {
      if (!addGlobalVariable(DIG))
        continue;
      auto *GV = DIG->getVariable();
      processScope(GV->getScope());
      processType(GV->getType().resolve());
    }
    for (auto *ET : CU->getEnumTypes())
      processType(ET);
    for (auto *RT : CU->getRetainedTypes())
      if (auto *T = dyn_cast<DIType>(RT))
        processType(T);
      else
        processSubprogram(cast<DISubprogram>(RT));
    for (auto *Import : CU->getImportedEntities()) {
      auto *Entity = Import->getEntity().resolve();
      if (auto *T = dyn_cast<DIType>(Entity))
        processType(T);
      else if (auto *SP = dyn_cast<DISubprogram>(Entity))
        processSubprogram(SP);
      else if (auto *NS = dyn_cast<DINamespace>(Entity))
        processScope(NS->getScope());
      else if (auto *M = dyn_cast<DIModule>(Entity))
        processScope(M->getScope());
    }
  }
  for (auto &F : M.functions()) {
    if (auto *SP = cast_or_null<DISubprogram>(F.getSubprogram()))
      processSubprogram(SP);
    // There could be subprograms from inlined functions referenced from
    // instructions only. Walk the function to find them.
    for (const BasicBlock &BB : F) {
      for (const Instruction &I : BB) {
        if (!I.getDebugLoc())
          continue;
        processLocation(M, I.getDebugLoc().get());
      }
    }
  }
}

void DebugInfoFinder::processLocation(const Module &M, const DILocation *Loc) {
  if (!Loc)
    return;
  processScope(Loc->getScope());
  processLocation(M, Loc->getInlinedAt());
}

void DebugInfoFinder::processType(DIType *DT) {
  if (!addType(DT))
    return;
  processScope(DT->getScope().resolve());
  if (auto *ST = dyn_cast<DISubroutineType>(DT)) {
    for (DITypeRef Ref : ST->getTypeArray())
      processType(Ref.resolve());
    return;
  }
  if (auto *DCT = dyn_cast<DICompositeType>(DT)) {
    processType(DCT->getBaseType().resolve());
    for (Metadata *D : DCT->getElements()) {
      if (auto *T = dyn_cast<DIType>(D))
        processType(T);
      else if (auto *SP = dyn_cast<DISubprogram>(D))
        processSubprogram(SP);
    }
    return;
  }
  if (auto *DDT = dyn_cast<DIDerivedType>(DT)) {
    processType(DDT->getBaseType().resolve());
  }
}

void DebugInfoFinder::processScope(DIScope *Scope) {
  if (!Scope)
    return;
  if (auto *Ty = dyn_cast<DIType>(Scope)) {
    processType(Ty);
    return;
  }
  if (auto *CU = dyn_cast<DICompileUnit>(Scope)) {
    addCompileUnit(CU);
    return;
  }
  if (auto *SP = dyn_cast<DISubprogram>(Scope)) {
    processSubprogram(SP);
    return;
  }
  if (!addScope(Scope))
    return;
  if (auto *LB = dyn_cast<DILexicalBlockBase>(Scope)) {
    processScope(LB->getScope());
  } else if (auto *NS = dyn_cast<DINamespace>(Scope)) {
    processScope(NS->getScope());
  } else if (auto *M = dyn_cast<DIModule>(Scope)) {
    processScope(M->getScope());
  }
}

void DebugInfoFinder::processSubprogram(DISubprogram *SP) {
  if (!addSubprogram(SP))
    return;
  processScope(SP->getScope().resolve());
  processType(SP->getType());
  for (auto *Element : SP->getTemplateParams()) {
    if (auto *TType = dyn_cast<DITemplateTypeParameter>(Element)) {
      processType(TType->getType().resolve());
    } else if (auto *TVal = dyn_cast<DITemplateValueParameter>(Element)) {
      processType(TVal->getType().resolve());
    }
  }
}

void DebugInfoFinder::processDeclare(const Module &M,
                                     const DbgDeclareInst *DDI) {
  auto *N = dyn_cast<MDNode>(DDI->getVariable());
  if (!N)
    return;

  auto *DV = dyn_cast<DILocalVariable>(N);
  if (!DV)
    return;

  if (!NodesSeen.insert(DV).second)
    return;
  processScope(DV->getScope());
  processType(DV->getType().resolve());
}

void DebugInfoFinder::processValue(const Module &M, const DbgValueInst *DVI) {
  auto *N = dyn_cast<MDNode>(DVI->getVariable());
  if (!N)
    return;

  auto *DV = dyn_cast<DILocalVariable>(N);
  if (!DV)
    return;

  if (!NodesSeen.insert(DV).second)
    return;
  processScope(DV->getScope());
  processType(DV->getType().resolve());
}

bool DebugInfoFinder::addType(DIType *DT) {
  if (!DT)
    return false;

  if (!NodesSeen.insert(DT).second)
    return false;

  TYs.push_back(const_cast<DIType *>(DT));
  return true;
}

bool DebugInfoFinder::addCompileUnit(DICompileUnit *CU) {
  if (!CU)
    return false;
  if (!NodesSeen.insert(CU).second)
    return false;

  CUs.push_back(CU);
  return true;
}

bool DebugInfoFinder::addGlobalVariable(DIGlobalVariableExpression *DIG) {
  if (!NodesSeen.insert(DIG).second)
    return false;

  GVs.push_back(DIG);
  return true;
}

bool DebugInfoFinder::addSubprogram(DISubprogram *SP) {
  if (!SP)
    return false;

  if (!NodesSeen.insert(SP).second)
    return false;

  SPs.push_back(SP);
  return true;
}

bool DebugInfoFinder::addScope(DIScope *Scope) {
  if (!Scope)
    return false;
  // FIXME: Ocaml binding generates a scope with no content, we treat it
  // as null for now.
  if (Scope->getNumOperands() == 0)
    return false;
  if (!NodesSeen.insert(Scope).second)
    return false;
  Scopes.push_back(Scope);
  return true;
}

static llvm::MDNode *stripDebugLocFromLoopID(llvm::MDNode *N) {
  assert(N->op_begin() != N->op_end() && "Missing self reference?");

  // if there is no debug location, we do not have to rewrite this MDNode.
  if (std::none_of(N->op_begin() + 1, N->op_end(), [](const MDOperand &Op) {
        return isa<DILocation>(Op.get());
      }))
    return N;

  // If there is only the debug location without any actual loop metadata, we
  // can remove the metadata.
  if (std::none_of(N->op_begin() + 1, N->op_end(), [](const MDOperand &Op) {
        return !isa<DILocation>(Op.get());
      }))
    return nullptr;

  SmallVector<Metadata *, 4> Args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = MDNode::getTemporary(N->getContext(), None);
  Args.push_back(TempNode.get());
  // Add all non-debug location operands back.
  for (auto Op = N->op_begin() + 1; Op != N->op_end(); Op++) {
    if (!isa<DILocation>(*Op))
      Args.push_back(*Op);
  }

  // Set the first operand to itself.
  MDNode *LoopID = MDNode::get(N->getContext(), Args);
  LoopID->replaceOperandWith(0, LoopID);
  return LoopID;
}

bool llvm::stripDebugInfo(Function &F) {
  bool Changed = false;
  if (F.getSubprogram()) {
    Changed = true;
    F.setSubprogram(nullptr);
  }

  llvm::DenseMap<llvm::MDNode*, llvm::MDNode*> LoopIDsMap;
  for (BasicBlock &BB : F) {
    for (auto II = BB.begin(), End = BB.end(); II != End;) {
      Instruction &I = *II++; // We may delete the instruction, increment now.
      if (isa<DbgInfoIntrinsic>(&I)) {
        I.eraseFromParent();
        Changed = true;
        continue;
      }
      if (I.getDebugLoc()) {
        Changed = true;
        I.setDebugLoc(DebugLoc());
      }
    }

    auto *TermInst = BB.getTerminator();
    if (auto *LoopID = TermInst->getMetadata(LLVMContext::MD_loop)) {
      auto *NewLoopID = LoopIDsMap.lookup(LoopID);
      if (!NewLoopID)
        NewLoopID = LoopIDsMap[LoopID] = stripDebugLocFromLoopID(LoopID);
      if (NewLoopID != LoopID)
        TermInst->setMetadata(LLVMContext::MD_loop, NewLoopID);
    }
  }
  return Changed;
}

bool llvm::StripDebugInfo(Module &M) {
  bool Changed = false;

  for (Module::named_metadata_iterator NMI = M.named_metadata_begin(),
         NME = M.named_metadata_end(); NMI != NME;) {
    NamedMDNode *NMD = &*NMI;
    ++NMI;

    // We're stripping debug info, and without them, coverage information
    // doesn't quite make sense.
    if (NMD->getName().startswith("llvm.dbg.") ||
        NMD->getName() == "llvm.gcov") {
      NMD->eraseFromParent();
      Changed = true;
    }
  }

  for (Function &F : M)
    Changed |= stripDebugInfo(F);

  for (auto &GV : M.globals()) {
    SmallVector<MDNode *, 1> MDs;
    GV.getMetadata(LLVMContext::MD_dbg, MDs);
    if (!MDs.empty()) {
      GV.eraseMetadata(LLVMContext::MD_dbg);
      Changed = true;
    }
  }

  if (GVMaterializer *Materializer = M.getMaterializer())
    Materializer->setStripDebugInfo();

  return Changed;
}

namespace {

/// Helper class to downgrade -g metadata to -gline-tables-only metadata.
class DebugTypeInfoRemoval {
  DenseMap<Metadata *, Metadata *> Replacements;

public:
  /// The (void)() type.
  MDNode *EmptySubroutineType;

private:
  /// Remember what linkage name we originally had before stripping. If we end
  /// up making two subprograms identical who originally had different linkage
  /// names, then we need to make one of them distinct, to avoid them getting
  /// uniqued. Maps the new node to the old linkage name.
  DenseMap<DISubprogram *, StringRef> NewToLinkageName;

  // TODO: Remember the distinct subprogram we created for a given linkage name,
  // so that we can continue to unique whenever possible. Map <newly created
  // node, old linkage name> to the first (possibly distinct) mdsubprogram
  // created for that combination. This is not strictly needed for correctness,
  // but can cut down on the number of MDNodes and let us diff cleanly with the
  // output of -gline-tables-only.

public:
  DebugTypeInfoRemoval(LLVMContext &C)
      : EmptySubroutineType(DISubroutineType::get(C, DINode::FlagZero, 0,
                                                  MDNode::get(C, {}))) {}

  Metadata *map(Metadata *M) {
    if (!M)
      return nullptr;
    auto Replacement = Replacements.find(M);
    if (Replacement != Replacements.end())
      return Replacement->second;

    return M;
  }
  MDNode *mapNode(Metadata *N) { return dyn_cast_or_null<MDNode>(map(N)); }

  /// Recursively remap N and all its referenced children. Does a DF post-order
  /// traversal, so as to remap bottoms up.
  void traverseAndRemap(MDNode *N) { traverse(N); }

private:
  // Create a new DISubprogram, to replace the one given.
  DISubprogram *getReplacementSubprogram(DISubprogram *MDS) {
    auto *FileAndScope = cast_or_null<DIFile>(map(MDS->getFile()));
    StringRef LinkageName = MDS->getName().empty() ? MDS->getLinkageName() : "";
    DISubprogram *Declaration = nullptr;
    auto *Type = cast_or_null<DISubroutineType>(map(MDS->getType()));
    DITypeRef ContainingType(map(MDS->getContainingType()));
    auto *Unit = cast_or_null<DICompileUnit>(map(MDS->getUnit()));
    auto Variables = nullptr;
    auto TemplateParams = nullptr;

    // Make a distinct DISubprogram, for situations that warrent it.
    auto distinctMDSubprogram = [&]() {
      return DISubprogram::getDistinct(
          MDS->getContext(), FileAndScope, MDS->getName(), LinkageName,
          FileAndScope, MDS->getLine(), Type, MDS->isLocalToUnit(),
          MDS->isDefinition(), MDS->getScopeLine(), ContainingType,
          MDS->getVirtuality(), MDS->getVirtualIndex(),
          MDS->getThisAdjustment(), MDS->getFlags(), MDS->isOptimized(), Unit,
          TemplateParams, Declaration, Variables);
    };

    if (MDS->isDistinct())
      return distinctMDSubprogram();

    auto *NewMDS = DISubprogram::get(
        MDS->getContext(), FileAndScope, MDS->getName(), LinkageName,
        FileAndScope, MDS->getLine(), Type, MDS->isLocalToUnit(),
        MDS->isDefinition(), MDS->getScopeLine(), ContainingType,
        MDS->getVirtuality(), MDS->getVirtualIndex(), MDS->getThisAdjustment(),
        MDS->getFlags(), MDS->isOptimized(), Unit, TemplateParams, Declaration,
        Variables);

    StringRef OldLinkageName = MDS->getLinkageName();

    // See if we need to make a distinct one.
    auto OrigLinkage = NewToLinkageName.find(NewMDS);
    if (OrigLinkage != NewToLinkageName.end()) {
      if (OrigLinkage->second == OldLinkageName)
        // We're good.
        return NewMDS;

      // Otherwise, need to make a distinct one.
      // TODO: Query the map to see if we already have one.
      return distinctMDSubprogram();
    }

    NewToLinkageName.insert({NewMDS, MDS->getLinkageName()});
    return NewMDS;
  }

  /// Create a new compile unit, to replace the one given
  DICompileUnit *getReplacementCU(DICompileUnit *CU) {
    // Drop skeleton CUs.
    if (CU->getDWOId())
      return nullptr;

    auto *File = cast_or_null<DIFile>(map(CU->getFile()));
    MDTuple *EnumTypes = nullptr;
    MDTuple *RetainedTypes = nullptr;
    MDTuple *GlobalVariables = nullptr;
    MDTuple *ImportedEntities = nullptr;
    return DICompileUnit::getDistinct(
        CU->getContext(), CU->getSourceLanguage(), File, CU->getProducer(),
        CU->isOptimized(), CU->getFlags(), CU->getRuntimeVersion(),
        CU->getSplitDebugFilename(), DICompileUnit::LineTablesOnly, EnumTypes,
        RetainedTypes, GlobalVariables, ImportedEntities, CU->getMacros(),
        CU->getDWOId(), CU->getSplitDebugInlining(),
        CU->getDebugInfoForProfiling());
  }

  DILocation *getReplacementMDLocation(DILocation *MLD) {
    auto *Scope = map(MLD->getScope());
    auto *InlinedAt = map(MLD->getInlinedAt());
    if (MLD->isDistinct())
      return DILocation::getDistinct(MLD->getContext(), MLD->getLine(),
                                     MLD->getColumn(), Scope, InlinedAt);
    return DILocation::get(MLD->getContext(), MLD->getLine(), MLD->getColumn(),
                           Scope, InlinedAt);
  }

  /// Create a new generic MDNode, to replace the one given
  MDNode *getReplacementMDNode(MDNode *N) {
    SmallVector<Metadata *, 8> Ops;
    Ops.reserve(N->getNumOperands());
    for (auto &I : N->operands())
      if (I)
        Ops.push_back(map(I));
    auto *Ret = MDNode::get(N->getContext(), Ops);
    return Ret;
  }

  /// Attempt to re-map N to a newly created node.
  void remap(MDNode *N) {
    if (Replacements.count(N))
      return;

    auto doRemap = [&](MDNode *N) -> MDNode * {
      if (!N)
        return nullptr;
      if (auto *MDSub = dyn_cast<DISubprogram>(N)) {
        remap(MDSub->getUnit());
        return getReplacementSubprogram(MDSub);
      }
      if (isa<DISubroutineType>(N))
        return EmptySubroutineType;
      if (auto *CU = dyn_cast<DICompileUnit>(N))
        return getReplacementCU(CU);
      if (isa<DIFile>(N))
        return N;
      if (auto *MDLB = dyn_cast<DILexicalBlockBase>(N))
        // Remap to our referenced scope (recursively).
        return mapNode(MDLB->getScope());
      if (auto *MLD = dyn_cast<DILocation>(N))
        return getReplacementMDLocation(MLD);

      // Otherwise, if we see these, just drop them now. Not strictly necessary,
      // but this speeds things up a little.
      if (isa<DINode>(N))
        return nullptr;

      return getReplacementMDNode(N);
    };
    Replacements[N] = doRemap(N);
  }

  /// Do the remapping traversal.
  void traverse(MDNode *);
};

} // Anonymous namespace.

void DebugTypeInfoRemoval::traverse(MDNode *N) {
  if (!N || Replacements.count(N))
    return;

  // To avoid cycles, as well as for efficiency sake, we will sometimes prune
  // parts of the graph.
  auto prune = [](MDNode *Parent, MDNode *Child) {
    if (auto *MDS = dyn_cast<DISubprogram>(Parent))
      return Child == MDS->getVariables().get();
    return false;
  };

  SmallVector<MDNode *, 16> ToVisit;
  DenseSet<MDNode *> Opened;

  // Visit each node starting at N in post order, and map them.
  ToVisit.push_back(N);
  while (!ToVisit.empty()) {
    auto *N = ToVisit.back();
    if (!Opened.insert(N).second) {
      // Close it.
      remap(N);
      ToVisit.pop_back();
      continue;
    }
    for (auto &I : N->operands())
      if (auto *MDN = dyn_cast_or_null<MDNode>(I))
        if (!Opened.count(MDN) && !Replacements.count(MDN) && !prune(N, MDN) &&
            !isa<DICompileUnit>(MDN))
          ToVisit.push_back(MDN);
  }
}

bool llvm::stripNonLineTableDebugInfo(Module &M) {
  bool Changed = false;

  // First off, delete the debug intrinsics.
  auto RemoveUses = [&](StringRef Name) {
    if (auto *DbgVal = M.getFunction(Name)) {
      while (!DbgVal->use_empty())
        cast<Instruction>(DbgVal->user_back())->eraseFromParent();
      DbgVal->eraseFromParent();
      Changed = true;
    }
  };
  RemoveUses("llvm.dbg.declare");
  RemoveUses("llvm.dbg.value");

  // Delete non-CU debug info named metadata nodes.
  for (auto NMI = M.named_metadata_begin(), NME = M.named_metadata_end();
       NMI != NME;) {
    NamedMDNode *NMD = &*NMI;
    ++NMI;
    // Specifically keep dbg.cu around.
    if (NMD->getName() == "llvm.dbg.cu")
      continue;
  }

  // Drop all dbg attachments from global variables.
  for (auto &GV : M.globals())
    GV.eraseMetadata(LLVMContext::MD_dbg);

  DebugTypeInfoRemoval Mapper(M.getContext());
  auto remap = [&](llvm::MDNode *Node) -> llvm::MDNode * {
    if (!Node)
      return nullptr;
    Mapper.traverseAndRemap(Node);
    auto *NewNode = Mapper.mapNode(Node);
    Changed |= Node != NewNode;
    Node = NewNode;
    return NewNode;
  };

  // Rewrite the DebugLocs to be equivalent to what
  // -gline-tables-only would have created.
  for (auto &F : M) {
    if (auto *SP = F.getSubprogram()) {
      Mapper.traverseAndRemap(SP);
      auto *NewSP = cast<DISubprogram>(Mapper.mapNode(SP));
      Changed |= SP != NewSP;
      F.setSubprogram(NewSP);
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto remapDebugLoc = [&](DebugLoc DL) -> DebugLoc {
          auto *Scope = DL.getScope();
          MDNode *InlinedAt = DL.getInlinedAt();
          Scope = remap(Scope);
          InlinedAt = remap(InlinedAt);
          return DebugLoc::get(DL.getLine(), DL.getCol(), Scope, InlinedAt);
        };

        if (I.getDebugLoc() != DebugLoc())
          I.setDebugLoc(remapDebugLoc(I.getDebugLoc()));

        // Remap DILocations in untyped MDNodes (e.g., llvm.loop).
        SmallVector<std::pair<unsigned, MDNode *>, 2> MDs;
        I.getAllMetadata(MDs);
        for (auto Attachment : MDs)
          if (auto *T = dyn_cast_or_null<MDTuple>(Attachment.second))
            for (unsigned N = 0; N < T->getNumOperands(); ++N)
              if (auto *Loc = dyn_cast_or_null<DILocation>(T->getOperand(N)))
                if (Loc != DebugLoc())
                  T->replaceOperandWith(N, remapDebugLoc(Loc));
      }
    }
  }

  // Create a new llvm.dbg.cu, which is equivalent to the one
  // -gline-tables-only would have created.
  for (auto &NMD : M.getNamedMDList()) {
    SmallVector<MDNode *, 8> Ops;
    for (MDNode *Op : NMD.operands())
      Ops.push_back(remap(Op));
 
    if (!Changed)
      continue;
 
    NMD.clearOperands();
    for (auto *Op : Ops)
      if (Op)
        NMD.addOperand(Op);
  }
  return Changed;
}

unsigned llvm::getDebugMetadataVersionFromModule(const Module &M) {
  if (auto *Val = mdconst::dyn_extract_or_null<ConstantInt>(
          M.getModuleFlag("Debug Info Version")))
    return Val->getZExtValue();
  return 0;
}

//===----------------------------------------------------------------------===//
// LLVM C API implementations.
//===----------------------------------------------------------------------===//

namespace llvm {
DEFINE_ISA_CONVERSION_FUNCTIONS(DIBuilder, LLVMDIBuilderRef)
DEFINE_ISA_CONVERSION_FUNCTIONS(Metadata, LLVMMetadataRef)

inline Metadata **unwrap(LLVMMetadataRef *Vals) {
  return reinterpret_cast<Metadata **>(Vals);
}
}

template <typename DIT> DIT *unwrapDI(LLVMMetadataRef Ref) {
  return (DIT *)(Ref ? unwrap<MDNode>(Ref) : nullptr);
}

static DINode::DIFlags fromC(LLVMDIFlags Flags) {
  return static_cast<DINode::DIFlags>(Flags);
}

extern "C" {
uint32_t LLVMDebugMetadataVersion() {
  return DEBUG_METADATA_VERSION;
}

unsigned LLVMGetModuleDebugMetadataVersion(LLVMModuleRef M) {
  return getDebugMetadataVersionFromModule(*unwrap(M));
}

uint8_t LLVMStripModuleDebugInfo(LLVMModuleRef M) {
  return StripDebugInfo(*unwrap(M));
}

uint8_t LLVMStripFunctionDebugInfo(LLVMValueRef F) {
  return stripDebugInfo(*unwrap<Function>(F));
}

LLVMDIBuilderRef LLVMDIBuilderCreate(LLVMModuleRef M, uint8_t AllowUnresolved) {
  return wrap(new DIBuilder(*unwrap(M), AllowUnresolved));
}

void LLVMDIBuilderDispose(LLVMDIBuilderRef Builder) {
  delete unwrap(Builder);
}

void LLVMDIBuilderFinalize(LLVMDIBuilderRef Builder) {
  unwrap(Builder)->finalize();
}

LLVMMetadataRef LLVMDIBuilderCreateClassType(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, LLVMMetadataRef File,
    unsigned LineNumber, uint64_t SizeInBits, uint32_t AlignInBits,
    uint64_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef *Elements,
    unsigned NumElements, LLVMMetadataRef DerivedFrom,
    LLVMMetadataRef TemplateParamsNode) {
  auto Elts = unwrap(Builder)->getOrCreateArray(
                  {unwrap(Elements), NumElements});
  return wrap(unwrap(Builder)->createClassType(
            unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File),
            LineNumber, SizeInBits, AlignInBits, OffsetInBits, fromC(Flags),
            unwrapDI<DIType>(DerivedFrom), Elts, nullptr,
            unwrap<MDNode>(TemplateParamsNode)));
}

LLVMMetadataRef LLVMDIBuilderCreateCompileUnit(
    LLVMDIBuilderRef Builder, LLVMDWARFSourceLanguage Lang,
    LLVMMetadataRef FileRef, const char *Producer, uint8_t isOptimized,
    const char *Flags, unsigned RuntimeVer, const char *SplitName,
    LLVMDWARFEmissionKind Kind, uint64_t DWOId, uint8_t SplitDebugInlining,
    uint8_t DebugInfoForProfiling) {
  auto *File = unwrapDI<DIFile>(FileRef);

  return wrap(unwrap(Builder)->createCompileUnit(
           static_cast<unsigned>(Lang), File, Producer,
           isOptimized, Flags, RuntimeVer, SplitName,
           static_cast<DICompileUnit::DebugEmissionKind>(Kind), DWOId,
           SplitDebugInlining, DebugInfoForProfiling));
}

LLVMMetadataRef
LLVMDIBuilderCreateFile(LLVMDIBuilderRef Builder, const char *Filename,
                        const char *Directory) {
  return wrap(unwrap(Builder)->createFile(Filename, Directory));
}

LLVMMetadataRef
LLVMDIBuilderCreateSubroutineType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef File,
                                  LLVMMetadataRef *ParameterTypes,
                                  unsigned NumParameterTypes) {
  auto Elts = unwrap(Builder)->getOrCreateTypeArray({unwrap(ParameterTypes),
                                                     NumParameterTypes});
  return wrap(unwrap(Builder)->createSubroutineType(Elts));
}

LLVMMetadataRef LLVMDIBuilderCreateFunction(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef *TemplateParams,
    unsigned NumTemplateParams, LLVMMetadataRef Decl) {
  auto DIB = unwrap(Builder);
  auto TParams = DIB->getOrCreateTemplateParameterArray(
                      {unwrap(TemplateParams), NumTemplateParams});
  DISubprogram *Sub = DIB->createFunction(
      unwrapDI<DIScope>(Scope), Name, LinkageName, unwrapDI<DIFile>(File),
      LineNo, unwrapDI<DISubroutineType>(Ty), IsLocalToUnit, IsDefinition,
      ScopeLine, fromC(Flags), IsOptimized, TParams,
      unwrapDI<DISubprogram>(Decl));
  unwrap<Function>(Fn)->setSubprogram(Sub);
  return wrap(Sub);
}



LLVMMetadataRef
LLVMDIBuilderCreateTempFunctionFwdDecl(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNo,
    LLVMMetadataRef Ty, uint8_t IsLocalToUnit, uint8_t IsDefinition,
    unsigned ScopeLine, LLVMDIFlags Flags, uint8_t IsOptimized,
    LLVMValueRef Fn, LLVMMetadataRef *TemplateParams,
    unsigned NumTemplateParams, LLVMMetadataRef Decl) {
  auto DIB = unwrap(Builder);
  auto TParams = DIB->getOrCreateTemplateParameterArray(
                      {unwrap(TemplateParams), NumTemplateParams});
  return wrap(DIB->createTempFunctionFwdDecl(
                unwrapDI<DIScope>(Scope), Name, LinkageName,
                unwrapDI<DIFile>(File), LineNo, unwrapDI<DISubroutineType>(Ty),
                IsLocalToUnit, IsDefinition, ScopeLine, fromC(Flags),
                IsOptimized, TParams, unwrapDI<DISubprogram>(Decl)));
}

LLVMMetadataRef
LLVMDIBuilderCreateArtificialType(LLVMDIBuilderRef Builder,
                                  LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createArtificialType(unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateAutoVariable(LLVMDIBuilderRef Builder,
                                LLVMMetadataRef Scope, const char *Name,
                                LLVMMetadataRef File, unsigned LineNo,
                                LLVMMetadataRef Type, uint8_t AlwaysPreserve,
                                LLVMDIFlags Flags, uint32_t AlignInBits) {
  return wrap(unwrap(Builder)->createAutoVariable(unwrapDI<DIScope>(Scope),
                                                  Name,
                                                  unwrapDI<DIFile>(File),
                                                  LineNo,
                                                  unwrapDI<DIType>(Type),
                                                  AlwaysPreserve, fromC(Flags),
                                                  AlignInBits));
}

LLVMMetadataRef
LLVMDIBuilderCreateBitFieldMemberType(LLVMDIBuilderRef Builder,
                                      LLVMMetadataRef Scope,
                                      const char *Name, LLVMMetadataRef File,
                                      unsigned LineNumber, uint64_t SizeInBits,
                                      uint64_t OffsetInBits,
                                      uint64_t StorageOffsetInBits,
                                      LLVMDIFlags Flags, LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createBitFieldMemberType(
                                            unwrapDI<DIScope>(Scope), Name,
                                            unwrapDI<DIFile>(File), LineNumber,
                                            SizeInBits, OffsetInBits,
                                            StorageOffsetInBits, fromC(Flags),
                                            unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateForwardDecl(LLVMDIBuilderRef Builder, unsigned Tag,
                               const char *Name, LLVMMetadataRef Scope,
                               LLVMMetadataRef File, unsigned Line) {
  return wrap(unwrap(Builder)->createForwardDecl(Tag, Name,
                                                 unwrapDI<DIScope>(Scope),
                                                 unwrapDI<DIFile>(File), Line));
}

LLVMMetadataRef
LLVMDIBuilderCreateMethod(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNumber,
    LLVMMetadataRef FuncTy, LLVMDIFlags Flags, uint8_t IsLocalToUnit,
    uint8_t IsDefinition, uint8_t IsOptimized,
    LLVMMetadataRef *TemplateParameters, unsigned NumTemplateParameters) {
  auto TParams = unwrap(Builder)->getOrCreateTemplateParameterArray(
                     {unwrap(TemplateParameters), NumTemplateParameters});
  return wrap(unwrap(Builder)->createMethod(unwrapDI<DIScope>(Scope),
                                            Name, LinkageName,
                                            unwrapDI<DIFile>(File), LineNumber,
                                            unwrapDI<DISubroutineType>(FuncTy),
                                            IsLocalToUnit, IsDefinition,
                                            0, 0, 0, nullptr, fromC(Flags),
                                            IsOptimized, TParams));
}

LLVMMetadataRef
LLVMDIBuilderCreateTypedef(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                           const char *Name, LLVMMetadataRef File,
                           unsigned Line, LLVMMetadataRef Scope) {
  return wrap(unwrap(Builder)->createTypedef(unwrapDI<DIType>(Type), Name,
                                             unwrapDI<DIFile>(File), Line,
                                             unwrapDI<DIScope>(Scope)));
}

LLVMMetadataRef
LLVMDIBuilderCreateInheritance(LLVMDIBuilderRef Builder, LLVMMetadataRef Type,
                               LLVMMetadataRef BaseType, uint64_t BaseOffset,
                               LLVMDIFlags Flags) {
  return wrap(unwrap(Builder)->createInheritance(unwrapDI<DIType>(Type),
                                                 unwrapDI<DIType>(BaseType),
                                                 BaseOffset, fromC(Flags)));
}

LLVMMetadataRef
LLVMDIBuilderCreateMacro(LLVMDIBuilderRef Builder,
                         LLVMMetadataRef ParentMacroFile, unsigned Line,
                         unsigned MacroType, const char *Name) {
  return wrap(unwrap(Builder)->createMacro(
                unwrapDI<DIMacroFile>(ParentMacroFile), Line, MacroType, Name));
}

LLVMMetadataRef
LLVMDIBuilderCreateTempMacroFile(LLVMDIBuilderRef Builder,
                                 LLVMMetadataRef ParentMacroFile,
                                 unsigned Line, LLVMMetadataRef File) {
  return wrap(unwrap(Builder)->createTempMacroFile(
                unwrapDI<DIMacroFile>(ParentMacroFile), Line,
                unwrapDI<DIFile>(File)));
}

LLVMMetadataRef
LLVMDIBuilderCreateMemberPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef PointeeType,
                                     LLVMMetadataRef ClassType,
                                     uint64_t SizeInBits,
                                     uint32_t AlignInBits,
                                     LLVMDIFlags Flags) {
  return wrap(unwrap(Builder)->createMemberPointerType(
                  unwrapDI<DIType>(PointeeType),
                  unwrapDI<DIType>(ClassType), AlignInBits, SizeInBits,
                  fromC(Flags)));
}

LLVMMetadataRef
LLVMDIBuilderCreateModule(LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
                          const char *Name, const char *ConfigurationMacros,
                          const char *IncludePath, const char *ISysRoot) {
  return wrap(unwrap(Builder)->createModule(unwrapDI<DIScope>(Scope), Name,
                                            ConfigurationMacros, IncludePath,
                                            ISysRoot));
}

LLVMMetadataRef
LLVMDIBuilderCreateNullPtrType(LLVMDIBuilderRef Builder) {
  return wrap(unwrap(Builder)->createNullPtrType());
}

LLVMMetadataRef
LLVMDIBuilderCreateParameterVariable(LLVMDIBuilderRef Builder,
    LLVMMetadataRef Scope, const char *Name, unsigned ArgNum,
    LLVMMetadataRef File, unsigned LineNum, LLVMMetadataRef Type,
    uint8_t AlwaysPreserve, LLVMDIFlags Flags) {
  return wrap(unwrap(Builder)->createParameterVariable(
                  unwrapDI<DIScope>(Scope), Name, ArgNum,
                  unwrapDI<DIFile>(File), LineNum, unwrapDI<DIType>(Type),
                  AlwaysPreserve, fromC(Flags)));
}

LLVMMetadataRef
LLVMDIBuilderCreateQualifiedType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createQualifiedType(Tag,
                                                   unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateReferenceType(LLVMDIBuilderRef Builder, unsigned Tag,
                                 LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createReferenceType(Tag,
                                                   unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateObjCIVar(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, uint64_t SizeInBits, uint32_t AlignInBits,
    uint32_t OffsetInBits, LLVMDIFlags Flags, LLVMMetadataRef Type,
    LLVMMetadataRef PropertyOrNull) {
  return wrap(unwrap(Builder)->createObjCIVar(Name, unwrapDI<DIFile>(File),
                  Line, SizeInBits, AlignInBits, OffsetInBits, fromC(Flags),
                  unwrapDI<DIType>(Type),
                  PropertyOrNull ? unwrap<MDNode>(PropertyOrNull) : nullptr));
}

LLVMMetadataRef
LLVMDIBuilderCreateObjCProperty(
    LLVMDIBuilderRef Builder, const char *Name, LLVMMetadataRef File,
    unsigned Line, const char *GetterName, const char *SetterName,
    unsigned PropertyAttributes, LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createObjCProperty(Name, unwrapDI<DIFile>(File),
                  Line, GetterName, SetterName, PropertyAttributes,
                  unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateReplaceableCompositeType(
    LLVMDIBuilderRef Builder, unsigned Tag, const char *Name,
    LLVMMetadataRef Scope, LLVMMetadataRef File, unsigned Line) {
  return wrap(unwrap(Builder)->createReplaceableCompositeType(
                  Tag, Name, unwrapDI<DIScope>(Scope),
                  unwrapDI<DIFile>(File), Line));
}

LLVMMetadataRef
LLVMDIBuilderCreateStaticMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, LLVMMetadataRef Type,
    LLVMDIFlags Flags, LLVMValueRef ConstantVal, uint32_t AlignInBits) {
  return wrap(unwrap(Builder)->createStaticMemberType(
                  unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File),
                  LineNumber, unwrapDI<DIType>(Type), fromC(Flags),
                  unwrap<Constant>(ConstantVal), AlignInBits));
}

LLVMMetadataRef
LLVMDIBuilderCreateObjectPointerType(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createObjectPointerType(unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedParameter(LLVMDIBuilderRef Builder) {
  return wrap(unwrap(Builder)->createUnspecifiedParameter());
}

LLVMMetadataRef
LLVMDIBuilderCreateTemplateTypeParameter(LLVMDIBuilderRef Builder,
                                         LLVMMetadataRef Scope,
                                         const char *Name,
                                         LLVMMetadataRef Type) {
  return wrap(unwrap(Builder)->createTemplateTypeParameter(
                  unwrapDI<DIScope>(Scope), Name, unwrapDI<DIType>(Type)));
}

LLVMMetadataRef
LLVMDIBuilderCreateUnspecifiedType(LLVMDIBuilderRef Builder, const char *Name) {
  return wrap(unwrap(Builder)->createUnspecifiedType(Name));
}

LLVMMetadataRef
LLVMDIBuilderCreateTemplateValueParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Type, LLVMValueRef ConstantValueOrNull) {
    return wrap(unwrap(Builder)->createTemplateValueParameter(
                    unwrapDI<DIScope>(Scope), Name, unwrapDI<DIType>(Type),
                    dyn_cast_or_null<Constant>(unwrap(ConstantValueOrNull))));
}

LLVMMetadataRef
LLVMDIBuilderCreateTemplateTemplateParameter(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef Type, const char *Str) {
  return wrap(unwrap(Builder)->createTemplateTemplateParameter(
                  unwrapDI<DIScope>(Scope), Name, unwrapDI<DIType>(Type),
                  Str));
}

LLVMMetadataRef
LLVMDIBuilderCreateFriend(LLVMDIBuilderRef Builder,
                          LLVMMetadataRef Type,
                          LLVMMetadataRef FriendType) {
  return wrap(unwrap(Builder)->createFriend(unwrapDI<DIType>(Type),
                                            unwrapDI<DIType>(FriendType)));
}

LLVMMetadataRef
LLVMDIBuilderCreateBasicType(LLVMDIBuilderRef Builder, const char *Name,
                             uint64_t SizeInBits,
                             LLVMDWARFTypeEncoding Encoding) {
  return wrap(unwrap(Builder)->createBasicType(Name, SizeInBits, Encoding));
}

LLVMMetadataRef LLVMDIBuilderCreatePointerType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef PointeeTy,
    uint64_t SizeInBits, uint32_t AlignInBits, unsigned AddressSpace,
    const char *Name) {
  return wrap(unwrap(Builder)->createPointerType(unwrapDI<DIType>(PointeeTy),
                                         SizeInBits, AlignInBits,
                                         AddressSpace, Name));
}

LLVMMetadataRef LLVMDIBuilderCreateStructType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags,
    LLVMMetadataRef DerivedFrom, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, LLVMMetadataRef VTableHolder,
    const char *UniqueId) {
  auto Elts = unwrap(Builder)->getOrCreateArray({unwrap(Elements),
                                                 NumElements});
  return wrap(unwrap(Builder)->createStructType(
      unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File), LineNumber,
      SizeInBits, AlignInBits, fromC(Flags), unwrapDI<DIType>(DerivedFrom),
      Elts, RunTimeLang, unwrapDI<DIType>(VTableHolder), UniqueId));
}

LLVMMetadataRef LLVMDIBuilderCreateMemberType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNo, uint64_t SizeInBits,
    uint32_t AlignInBits, uint64_t OffsetInBits, LLVMDIFlags Flags,
    LLVMMetadataRef Ty) {
  return wrap(unwrap(Builder)->createMemberType(unwrapDI<DIScope>(Scope),
                                        Name, unwrapDI<DIFile>(File), LineNo,
                                        SizeInBits, AlignInBits, OffsetInBits,
                                        fromC(Flags), unwrapDI<DIType>(Ty)));
}

LLVMMetadataRef LLVMDIBuilderCreateLexicalBlock(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope,
    LLVMMetadataRef File, unsigned Line, unsigned Col) {
  return wrap(unwrap(Builder)->createLexicalBlock(unwrapDI<DIScope>(Scope),
                                          unwrapDI<DIFile>(File), Line, Col));
}

LLVMMetadataRef
LLVMDIBuilderCreateLexicalBlockFile(LLVMDIBuilderRef Builder,
                                    LLVMMetadataRef Scope,
                                    LLVMMetadataRef File) {
  return wrap(unwrap(Builder)->createLexicalBlockFile(
            unwrapDI<DIScope>(Scope),
            unwrapDI<DIFile>(File)));
}

LLVMMetadataRef
LLVMDIBuilderCreateConstantValueExpression(LLVMDIBuilderRef Builder,
                                           uint64_t Val) {
  return wrap(unwrap(Builder)->createConstantValueExpression(Val));
}

LLVMMetadataRef
LLVMDIBuilderCreateFragmentExpression(LLVMDIBuilderRef Builder,
                                      unsigned OffsetInBits,
                                      unsigned SizeInBits) {
  return wrap(unwrap(Builder)->createFragmentExpression(
                                  OffsetInBits, SizeInBits));
}

LLVMMetadataRef
LLVMDIBuilderCreateExpression(LLVMDIBuilderRef Builder, int64_t *Addrs,
                              unsigned NumAddrs) {
  return wrap(unwrap(Builder)->createExpression(ArrayRef<int64_t>(Addrs,
                                                                  NumAddrs)));
}

LLVMMetadataRef
LLVMDIBuilderCreateGlobalVariableExpression(
    LLVMDIBuilderRef Builder,  LLVMMetadataRef Scope, const char *Name,
    const char *LinkageName, LLVMMetadataRef File, unsigned LineNumber,
    LLVMMetadataRef Ty, uint8_t isLocalToUnit, LLVMDIExpressionRef Expr) {
  auto E = unwrap(Builder)->createGlobalVariableExpression(
    unwrapDI<DIScope>(Scope), Name, LinkageName, unwrapDI<DIFile>(File),
    LineNumber, unwrapDI<DIType>(Ty), isLocalToUnit);
  return wrap(E);
}

LLVMMetadataRef
LLVMDIBuilderCreateArrayType(LLVMDIBuilderRef Builder, uint64_t Size,
                             uint32_t AlignInBits, LLVMMetadataRef Ty,
                             LLVMMetadataRef *Subscripts,
                             unsigned NumSubscripts) {
  auto Subs = unwrap(Builder)->getOrCreateArray({unwrap(Subscripts),
                                                 NumSubscripts});
  return wrap(unwrap(Builder)->createArrayType(Size, AlignInBits,
                                               unwrapDI<DIType>(Ty), Subs));
}

LLVMMetadataRef
LLVMDIBuilderCreateVectorType(LLVMDIBuilderRef Builder, uint64_t Size,
                              uint32_t AlignInBits, LLVMMetadataRef Ty,
                              LLVMMetadataRef *Subscripts,
                              unsigned NumSubscripts) {
  auto Subs = unwrap(Builder)->getOrCreateArray({unwrap(Subscripts),
                                                 NumSubscripts});
  return wrap(unwrap(Builder)->createVectorType(Size, AlignInBits,
                                                unwrapDI<DIType>(Ty), Subs));
}

LLVMMetadataRef
LLVMDIBuilderGetOrCreateSubrange(LLVMDIBuilderRef Builder, int64_t Lo,
                                 int64_t Count) {
  return wrap(unwrap(Builder)->getOrCreateSubrange(Lo, Count));
}

LLVMMetadataRef
LLVMDIBuilderGetOrCreateArray(LLVMDIBuilderRef Builder,
                              LLVMMetadataRef *Ptr, unsigned Count) {
  Metadata **DataValue = unwrap(Ptr);
  return wrap(
      unwrap(Builder)->getOrCreateArray(ArrayRef<Metadata *>(DataValue,
                                                             Count)).get());
}

LLVMValueRef LLVMDIBuilderInsertDeclareAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef V, LLVMMetadataRef VarInfo,
    int64_t *AddrOps, unsigned AddrOpsCount, LLVMMetadataRef DL,
    LLVMBasicBlockRef InsertAtEnd) {
  return wrap(unwrap(Builder)->insertDeclare(
      unwrap(V), unwrap<DILocalVariable>(VarInfo),
      unwrap(Builder)->createExpression(ArrayRef<int64_t>(AddrOps,
                                                          AddrOpsCount)),
      unwrapDI<DILocation>(DL), unwrap(InsertAtEnd)));
}

LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicAtEnd(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMBasicBlockRef InsertAtEnd) {
  return wrap(unwrap(Builder)->insertDbgValueIntrinsic(unwrap(Val), Offset,
                unwrapDI<DILocalVariable>(VarInfo),
                unwrapDI<DIExpression>(Expr), unwrapDI<DILocation>(Loc),
                unwrap(InsertAtEnd)));
}

LLVMValueRef
LLVMDIBuilderInsertDbgValueIntrinsicBefore(
    LLVMDIBuilderRef Builder, LLVMValueRef Val, uint64_t Offset,
    LLVMMetadataRef VarInfo, LLVMMetadataRef Expr, LLVMMetadataRef Loc,
    LLVMValueRef InsertBefore) {
  return wrap(unwrap(Builder)->insertDbgValueIntrinsic(unwrap(Val), Offset,
                unwrapDI<DILocalVariable>(VarInfo),
                unwrapDI<DIExpression>(Expr), unwrapDI<DILocation>(Loc),
                dyn_cast<Instruction>(unwrap(InsertBefore))));
}

LLVMMetadataRef
LLVMDIBuilderCreateEnumerator(LLVMDIBuilderRef Builder,
                                  const char *Name, int64_t Val) {
  return wrap(unwrap(Builder)->createEnumerator(Name, Val));
}

LLVMMetadataRef LLVMDIBuilderCreateEnumerationType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMMetadataRef *Elements, unsigned NumElements,
    LLVMMetadataRef ClassTy) {
  auto Elts = unwrap(Builder)->getOrCreateArray({unwrap(Elements),
                                                 NumElements});
  return wrap(unwrap(Builder)->createEnumerationType(
      unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File), LineNumber,
      SizeInBits, AlignInBits, Elts, unwrapDI<DIType>(ClassTy)));
}

LLVMMetadataRef LLVMDIBuilderCreateUnionType(
    LLVMDIBuilderRef Builder, LLVMMetadataRef Scope, const char *Name,
    LLVMMetadataRef File, unsigned LineNumber, uint64_t SizeInBits,
    uint32_t AlignInBits, LLVMDIFlags Flags, LLVMMetadataRef *Elements,
    unsigned NumElements, unsigned RunTimeLang, const char *UniqueId) {
  auto Elts = unwrap(Builder)->getOrCreateArray({unwrap(Elements),
                                                 NumElements});
  return wrap(unwrap(Builder)->createUnionType(
      unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File), LineNumber,
      SizeInBits, AlignInBits, fromC(Flags), Elts, RunTimeLang, UniqueId));
}

LLVMMetadataRef
LLVMDIBuilderCreateNameSpace(LLVMDIBuilderRef Builder,
                             LLVMMetadataRef Scope, const char *Name,
                             LLVMMetadataRef File, unsigned LineNo,
                             uint8_t ExportSymbols) {
  return wrap(unwrap(Builder)->createNameSpace(
      unwrapDI<DIScope>(Scope), Name, unwrapDI<DIFile>(File), LineNo,
      ExportSymbols));
}

void LLVMDICompositeTypeSetTypeArray(LLVMDIBuilderRef Builder,
                                     LLVMMetadataRef CompositeTy,
                                     LLVMMetadataRef *Types,
                                     unsigned NumTypes) {
  DICompositeType *Tmp = unwrapDI<DICompositeType>(CompositeTy);
  auto Tys = unwrap(Builder)->getOrCreateArray({unwrap(Types), NumTypes});
  unwrap(Builder)->replaceArrays(Tmp, Tys);
}

LLVMMetadataRef
LLVMDIBuilderCreateDebugLocation(LLVMContextRef Ctx, unsigned Line,
                                 unsigned Column, LLVMMetadataRef Scope,
                                 LLVMMetadataRef InlinedAt) {
  return wrap(DILocation::get(*unwrap(Ctx), Line, Column, unwrap(Scope),
                              unwrap(InlinedAt)));
}
}
