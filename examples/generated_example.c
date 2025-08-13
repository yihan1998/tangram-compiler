#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Function: multiply_shift_hash_32_const
uint32_t multiply_shift_hash_32_const(uint32_t arg0) {
    uint32_t temp0 = 41;
    uint32_t temp1 = 24;
    uint32_t temp2 = arg0 + temp0;
    uint32_t temp3 = temp2 >> temp1;
    return temp3;
}

// Function: arithmetic_ops  
uint32_t arithmetic_ops(uint32_t arg0, uint32_t arg1) {
    uint32_t temp0 = arg0 + arg1;
    uint32_t temp1 = temp0 - arg1;
    uint32_t temp2 = temp1 << arg1;
    uint32_t temp3 = temp2 >> arg0;
    return temp3;
}

// Simple test to verify the generated code compiles and works
int main() {
    uint32_t result1 = multiply_shift_hash_32_const(100);
    uint32_t result2 = arithmetic_ops(5, 3);
    
    printf("multiply_shift_hash_32_const(100) = %u\n", result1);
    printf("arithmetic_ops(5, 3) = %u\n", result2);
    
    return 0;
}