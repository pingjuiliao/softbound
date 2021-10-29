; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instcombine -S | FileCheck %s

; If we have a masked merge, in the form of: (M is not constant)
;   ((x ^ y) & ~M) ^ y
; We can de-invert the M:
;   ((x ^ y) & M) ^ x

define i4 @scalar (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @scalar(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %y
  ret i4 %r
}

; ============================================================================ ;
; Various cases with %x and/or %y being a constant
; ============================================================================ ;

define i4 @in_constant_varx_mone_invmask(i4 %x, i4 %mask) {
; CHECK-LABEL: @in_constant_varx_mone_invmask(
; CHECK-NEXT:    [[N1_DEMORGAN:%.*]] = or i4 [[X:%.*]], [[MASK:%.*]]
; CHECK-NEXT:    ret i4 [[N1_DEMORGAN]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 %x, -1 ; %x
  %n1 = and i4 %n0, %notmask
  %r = xor i4 %n1, -1
  ret i4 %r
}

define i4 @in_constant_varx_6_invmask(i4 %x, i4 %mask) {
; CHECK-LABEL: @in_constant_varx_6_invmask(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], 6
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[MASK:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 %x, 6 ; %x
  %n1 = and i4 %n0, %notmask
  %r = xor i4 %n1, 6
  ret i4 %r
}

define i4 @in_constant_mone_vary_invmask(i4 %y, i4 %mask) {
; CHECK-LABEL: @in_constant_mone_vary_invmask(
; CHECK-NEXT:    [[MASK_NOT:%.*]] = xor i4 [[MASK:%.*]], -1
; CHECK-NEXT:    [[R:%.*]] = or i4 [[MASK_NOT]], [[Y:%.*]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 -1, %y ; %x
  %n1 = and i4 %n0, %notmask
  %r = xor i4 %n1, %y
  ret i4 %r
}

define i4 @in_constant_6_vary_invmask(i4 %y, i4 %mask) {
; CHECK-LABEL: @in_constant_6_vary_invmask(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y:%.*]], 6
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[MASK:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], 6
; CHECK-NEXT:    ret i4 [[R]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 %y, 6 ; %x
  %n1 = and i4 %n0, %notmask
  %r = xor i4 %n1, %y
  ret i4 %r
}

; ============================================================================ ;
; Commutativity
; ============================================================================ ;

; Used to make sure that the IR complexity sorting does not interfere.
declare i4 @gen4()

; FIXME: should the  %n1 = and i4 %im, %n0  swapped order pattern be tested?

define i4 @c_1_0_0 (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @c_1_0_0(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %y, %x ; swapped order
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %y
  ret i4 %r
}

define i4 @c_0_1_0 (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @c_0_1_0(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[Y]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %x ; %x instead of %y
  ret i4 %r
}

define i4 @c_0_0_1 (i4 %m) {
; CHECK-LABEL: @c_0_0_1(
; CHECK-NEXT:    [[X:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[Y:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %x  = call i4 @gen4()
  %y  = call i4 @gen4()
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %y, %n1 ; swapped order
  ret i4 %r
}

define i4 @c_1_1_0 (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @c_1_1_0(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[Y]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %y, %x ; swapped order
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %x ; %x instead of %y
  ret i4 %r
}

define i4 @c_1_0_1 (i4 %x, i4 %m) {
; CHECK-LABEL: @c_1_0_1(
; CHECK-NEXT:    [[Y:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y]], [[X:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %y  = call i4 @gen4()
  %n0 = xor i4 %y, %x ; swapped order
  %n1 = and i4 %n0, %im
  %r  = xor i4 %y, %n1 ; swapped order
  ret i4 %r
}

define i4 @c_0_1_1 (i4 %y, i4 %m) {
; CHECK-LABEL: @c_0_1_1(
; CHECK-NEXT:    [[X:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X]], [[Y:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[Y]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %x  = call i4 @gen4()
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %x, %n1 ; swapped order, %x instead of %y
  ret i4 %r
}

define i4 @c_1_1_1 (i4 %m) {
; CHECK-LABEL: @c_1_1_1(
; CHECK-NEXT:    [[X:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[Y:%.*]] = call i4 @gen4()
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y]], [[X]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[Y]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %x  = call i4 @gen4()
  %y  = call i4 @gen4()
  %n0 = xor i4 %y, %x ; swapped order
  %n1 = and i4 %n0, %im
  %r  = xor i4 %x, %n1 ; swapped order, %x instead of %y
  ret i4 %r
}

define i4 @commutativity_constant_varx_6_invmask(i4 %x, i4 %mask) {
; CHECK-LABEL: @commutativity_constant_varx_6_invmask(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], 6
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[MASK:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 %x, 6 ; %x
  %n1 = and i4 %notmask, %n0 ; swapped
  %r = xor i4 %n1, 6
  ret i4 %r
}

define i4 @commutativity_constant_6_vary_invmask(i4 %y, i4 %mask) {
; CHECK-LABEL: @commutativity_constant_6_vary_invmask(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[Y:%.*]], 6
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[MASK:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], 6
; CHECK-NEXT:    ret i4 [[R]]
;
  %notmask = xor i4 %mask, -1
  %n0 = xor i4 %y, 6 ; %x
  %n1 = and i4 %notmask, %n0 ; swapped
  %r = xor i4 %n1, %y
  ret i4 %r
}

; ============================================================================ ;
; Negative tests. Should not be folded.
; ============================================================================ ;

; One use only.

declare void @use4(i4)

define i4 @n_oneuse_D_is_ok (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @n_oneuse_D_is_ok(
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = and i4 [[N0]], [[M:%.*]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[TMP1]], [[X]]
; CHECK-NEXT:    call void @use4(i4 [[N0]])
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y ; two uses of %n0, THIS IS OK!
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %y
  call void @use4(i4 %n0)
  ret i4 %r
}

define i4 @n_oneuse_A (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @n_oneuse_A(
; CHECK-NEXT:    [[IM:%.*]] = xor i4 [[M:%.*]], -1
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[N1:%.*]] = and i4 [[N0]], [[IM]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[N1]], [[Y]]
; CHECK-NEXT:    call void @use4(i4 [[N1]])
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im ; two uses of %n1, which is going to be replaced
  %r  = xor i4 %n1, %y
  call void @use4(i4 %n1)
  ret i4 %r
}

define i4 @n_oneuse_AD (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @n_oneuse_AD(
; CHECK-NEXT:    [[IM:%.*]] = xor i4 [[M:%.*]], -1
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[N1:%.*]] = and i4 [[N0]], [[IM]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[N1]], [[Y]]
; CHECK-NEXT:    call void @use4(i4 [[N0]])
; CHECK-NEXT:    call void @use4(i4 [[N1]])
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im ; two uses of %n1, which is going to be replaced
  %r  = xor i4 %n1, %y
  call void @use4(i4 %n0)
  call void @use4(i4 %n1)
  ret i4 %r
}

; Some third variable is used

define i4 @n_third_var (i4 %x, i4 %y, i4 %z, i4 %m) {
; CHECK-LABEL: @n_third_var(
; CHECK-NEXT:    [[IM:%.*]] = xor i4 [[M:%.*]], -1
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[N1:%.*]] = and i4 [[N0]], [[IM]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[N1]], [[Z:%.*]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %z ; not %x or %y
  ret i4 %r
}

define i4 @n_badxor (i4 %x, i4 %y, i4 %m) {
; CHECK-LABEL: @n_badxor(
; CHECK-NEXT:    [[IM:%.*]] = xor i4 [[M:%.*]], 1
; CHECK-NEXT:    [[N0:%.*]] = xor i4 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[N1:%.*]] = and i4 [[N0]], [[IM]]
; CHECK-NEXT:    [[R:%.*]] = xor i4 [[N1]], [[Y]]
; CHECK-NEXT:    ret i4 [[R]]
;
  %im = xor i4 %m, 1 ; not -1
  %n0 = xor i4 %x, %y
  %n1 = and i4 %n0, %im
  %r  = xor i4 %n1, %y
  ret i4 %r
}