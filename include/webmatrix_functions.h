#ifndef __WEBMATRIX_FUNCTIONS_H__
#define __WEBMATRIX_FUNCTIONS_H__
#include <Arduino.h>

bool check_valid_input_text(String check_input_text); // Allow only ASCII
bool check_valid_rgb(String check_rgb); // Allow only integer numbers between 0 - 255
bool check_valid_speed(String check_speed); // Allow numbers and "." (decimals only), error on anything else 

#endif // __WEBMATRIX_FUNCTIONS_H__