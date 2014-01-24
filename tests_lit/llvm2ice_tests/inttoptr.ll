; RUN: %llvm2ice %s | FileCheck %s

define void @dummy_inttoptr(i32 %addr_arg) {
entry:
  %ptr = inttoptr i32 %addr_arg to i32*
  ret void
; CHECK: %ptr = i32 %addr_arg
}