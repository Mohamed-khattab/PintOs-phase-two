#include <stdint.h>

typedef int fixed_point;

// Function to convert an integer to its fixed-point representation
fixed_point int_to_fixed(int x); 

// Function to convert a fixed-point number to its integer representation
int fixed_to_int_round(fixed_point x);

//round towards 0
int fixed_to_int_floor(fixed_point x);
 
 // Function to perform addition on two fixed-point numbers
fixed_point fixed_add(fixed_point x, fixed_point y) ;

// Function to perform subtraction on two fixed-point numbers
fixed_point fixed_subtract(fixed_point x, fixed_point y) ;

fixed_point int_fixed_add(fixed_point x, int n);

fixed_point int_fixed_sub(int n, fixed_point x);

// Function to perform multiplication on two fixed-point numbers
fixed_point fixed_multiply(fixed_point x, fixed_point y);

// Function to perform division on two fixed-point numbers
fixed_point fixed_divide(fixed_point x, fixed_point y);

fixed_point int_fixed_mul(fixed_point x, int n);

fixed_point int_fixed_div(fixed_point x, int n);
