!b32i = !p4hir.bit<32>
#int41_b32i = #p4hir.int<41> : !b32i
#int24_b32i = #p4hir.int<24> : !b32i
#int32_b32i = #p4hir.int<32> : !b32i
#int8_b32i = #p4hir.int<8> : !b32i
module {
  p4hir.func action @multiply_shift_hash_32_const(%arg0: !b32i {p4hir.dir = #p4hir<dir in>, p4hir.param_name = "x"}, %arg1: !p4hir.ref<!b32i> {p4hir.dir = #p4hir<dir out>, p4hir.param_name = "hash"}) {
    %a = p4hir.const ["a"] #int41_b32i
    %r = p4hir.const ["r"] #int8_b32i
    %w = p4hir.const ["w"] #int32_b32i
    %c24_b32i = p4hir.const #int24_b32i
    %shift_amount = p4hir.variable ["shift_amount", init] : <!b32i>
    p4hir.assign %c24_b32i, %shift_amount : <!b32i>
    %c41_b32i = p4hir.const #int41_b32i
    %mul = p4hir.binop(mul, %arg0, %c41_b32i) : !b32i
    %product = p4hir.variable ["product", init] : <!b32i>
    p4hir.assign %mul, %product : <!b32i>
    %val = p4hir.read %product : <!b32i>
    %val_0 = p4hir.read %shift_amount : <!b32i>
    %shr = p4hir.shr(%val, %val_0 : !b32i) : !b32i
    p4hir.assign %shr, %arg1 : <!b32i>
    p4hir.return
  }
}
