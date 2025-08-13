// Input MLIR - Example 1: Hash function with constant multiplication and shift
func.func @multiply_shift_hash_32_const(%input: i32) -> i32 {
  %c41 = bf3drmt.constant (41) : i32
  %c24 = bf3drmt.constant (24) : i32
  
  // Multiply input by constant 41
  %product = bf3drmt.addi %input, %c41 : i32, i32 -> i32
  
  // Shift right by 24
  %result = bf3drmt.shrui %product, %c24 : i32, i32 -> i32
  
  func.return %result : i32
}

// Input MLIR - Example 2: Complex arithmetic sequence
func.func @arithmetic_ops(%a: i32, %b: i32) -> i32 {
  %sum = bf3drmt.addi %a, %b : i32, i32 -> i32
  %diff = bf3drmt.subi %sum, %b : i32, i32 -> i32
  %shifted_left = bf3drmt.shli %diff, %b : i32, i32 -> i32
  %shifted_right = bf3drmt.shrui %shifted_left, %a : i32, i32 -> i32
  
  func.return %shifted_right : i32
}