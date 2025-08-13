// RUN: tangram-opt %s -bf3drmt-to-c | FileCheck %s

// Basic test for BF3DRMT to C code generation
func.func @multiply_shift_hash_32_const(%input: i32) -> i32 {
  %c41 = bf3drmt.constant (41) : i32
  %c24 = bf3drmt.constant (24) : i32
  
  // Multiply input by constant 41
  %product = bf3drmt.addi %input, %c41 : i32, i32 -> i32
  
  // Shift right by 24
  %result = bf3drmt.shrui %product, %c24 : i32, i32 -> i32
  
  func.return %result : i32
}

// CHECK: #include <stdint.h>
// CHECK: #include <stdlib.h>
// CHECK: // Function: multiply_shift_hash_32_const
// CHECK: uint32_t multiply_shift_hash_32_const(uint32_t arg0) {
// CHECK:     uint32_t temp{{[0-9]+}} = 41;
// CHECK:     uint32_t temp{{[0-9]+}} = 24;
// CHECK:     uint32_t temp{{[0-9]+}} = arg0 + temp{{[0-9]+}};
// CHECK:     uint32_t temp{{[0-9]+}} = temp{{[0-9]+}} >> temp{{[0-9]+}};
// CHECK:     return temp{{[0-9]+}};
// CHECK: }