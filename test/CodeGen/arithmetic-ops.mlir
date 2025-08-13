// RUN: tangram-opt %s -bf3drmt-to-c | FileCheck %s

// Test for arithmetic operations
func.func @arithmetic_ops(%a: i32, %b: i32) -> i32 {
  %sum = bf3drmt.addi %a, %b : i32, i32 -> i32
  %diff = bf3drmt.subi %sum, %b : i32, i32 -> i32
  %shifted_left = bf3drmt.shli %diff, %b : i32, i32 -> i32
  %shifted_right = bf3drmt.shrui %shifted_left, %a : i32, i32 -> i32
  
  func.return %shifted_right : i32
}

// CHECK: #include <stdint.h>
// CHECK: #include <stdlib.h>
// CHECK: // Function: arithmetic_ops
// CHECK: uint32_t arithmetic_ops(uint32_t arg0, uint32_t arg1) {
// CHECK:     uint32_t temp{{[0-9]+}} = arg0 + arg1;
// CHECK:     uint32_t temp{{[0-9]+}} = temp{{[0-9]+}} - arg1;
// CHECK:     uint32_t temp{{[0-9]+}} = temp{{[0-9]+}} << arg1;
// CHECK:     uint32_t temp{{[0-9]+}} = temp{{[0-9]+}} >> arg0;
// CHECK:     return temp{{[0-9]+}};
// CHECK: }