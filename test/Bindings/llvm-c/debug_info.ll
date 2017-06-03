; RUN: llvm-c-test --test-dibuilder | FileCheck %s

; CHECK: ; ModuleID = 'debuginfo.c'
; CHECK: source_filename = "debuginfo.c"

; CHECK: !llvm.dbg.cu = !{!0}
; CHECK:
; CHECK: !0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "llvm-c-test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false)
; CHECK: !1 = !DIFile(filename: "debuginfo.c\00", directory: ".")
