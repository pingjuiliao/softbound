; NOTE: Assertions have been autogenerated by utils/update_mir_test_checks.py

; RUN: llc -march=mips -mcpu=mips32r2 -O0 -relocation-model=pic -mattr=+fp64 \
; RUN:   -stop-before=prologepilog %s -o - | FileCheck %s

declare double @bar(double)

define  double @foo(double %self) {
  ; CHECK-LABEL: name: foo
  ; CHECK: bb.0.start:
  ; CHECK:   successors: %bb.1(0x80000000)
  ; CHECK:   liveins: $d12_64, $t9, $v0
  ; CHECK:   renamable $at = ADDu killed $v0, killed $t9
  ; CHECK:   renamable $d6_64 = COPY killed $d12_64
  ; CHECK:   ADJCALLSTACKDOWN 16, 0, implicit-def $sp, implicit $sp
  ; CHECK:   renamable $t9 = LW killed renamable $at, target-flags(mips-got) @bar
  ; CHECK:   dead $ra = JALR killed $t9, csr_o32_fp64, target-flags(mips-jalr) <mcsymbol bar>, implicit-def dead $ra, implicit killed $d6_64, implicit-def $d0_64
  ; CHECK:   ADJCALLSTACKUP 16, 0, implicit-def $sp, implicit $sp
  ; CHECK:   SDC164 killed $d0_64, %stack.0, 0 :: (store (s64) into %stack.0)
  ; CHECK: bb.1.bb1:
  ; CHECK:   $d0_64 = LDC164 %stack.0, 0 :: (load (s64) from %stack.0)
  ; CHECK:   RetRA implicit killed $d0_64
start:
  %0 = call double @bar(double %self)
  br label %bb1

bb1:
  ret double %0
}