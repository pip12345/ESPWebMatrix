#include "webmatrix_functions.h"

// Allow only ASCII
bool check_valid_input_text(String check_input_text) 
{
    bool error = false;
    for(int i = 0; i < check_input_text.length(); i++) {
        if(!isAscii(check_input_text.c_str()[i])) {
            error = true;
        }
    }
    return error;
}

// Allow only integer numbers between 0 - 255
bool check_valid_rgb(String check_rgb) 
{
    bool error = false;
    int rgb_int = check_rgb.toInt();
    for(int i = 0; i < check_rgb.length(); i++) {
        if(!isDigit(check_rgb.c_str()[i])) {
            error = true;
        }
    }
    if(rgb_int < 0 || rgb_int > 255) {
        error = true;
    }
    return error;
}

// Allow numbers and "." (decimals only), error on anything else 
bool check_valid_speed(String check_speed) 
{
    bool error = false;
    int decimal_count = 0; // counts amount of decimals, max 1 per input 
    int digit_count = 0; // counts amount of digits, minimum 1 per input 
    for(int i = 0; i < check_speed.length(); i++) {
        // (isDigit) XNOR (input == '.')
        if( (isDigit(check_speed.c_str()[i]) && check_speed.c_str()[i] == '.') 
            || ((!(isDigit(check_speed.c_str()[i]))) && (!(check_speed.c_str()[i] == '.'))) ) {
                error = true;
        }
        if(check_speed.c_str()[i] == '.') {
            decimal_count++;
        }
        if(isDigit(check_speed.c_str()[i])) {
            digit_count++;
        }
        // Return error if there's too many decimals or no digits
        if(decimal_count > 1 || digit_count < 1) {
            error = true;
        }
    }
    // avoid cases like '04832'
    if(check_speed.c_str()[0] == '0' && check_speed.c_str()[1] != '.') {
        error = true;
    }

    return error;
}