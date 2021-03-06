; RUN: %llvm2ice -verbose inst %s | FileCheck %s
; RUN: %llvm2ice --verbose none %s | FileCheck --check-prefix=ERRORS %s

define void @dummy_icmp(i64 %foo, i64 %bar) {
; CHECK: define internal void dummy_icmp
entry:
  %r1 = icmp eq i64 %foo, %bar
  %r2 = icmp slt i64 %foo, %bar
  ret void
; CHECK:       entry:
; CHECK-NEXT:   %r1 = icmp eq i64 %foo, %bar
; CHECK-NEXT:   %r2 = icmp slt i64 %foo, %bar
; CHECK-NEXT:   ret void 
}

; ERRORS-NOT: ICE translation error
