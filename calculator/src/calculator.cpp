#include"../include/calculator.hpp"
#include"../include/data.hpp"
#include<mathlib/mathlib.hpp>

void calculate(Data *data){
    if(data == 0) return;

    if(data->error_type != ERROR_NONE) return;

    if(data->mode == HELP){
        return;
    }

    long long result = 0;

    int ok = 0;

    switch(data->operation){
        case ADD:{
            ok = mathlib::add(data->operand1, data->operand2, &result);
            break;
        }
        case SUB:{
            ok = mathlib::sub(data->operand1, data->operand2, &result);
            break;
        }
        case MUL:{
            ok = mathlib::mul(data->operand1, data->operand2, &result);
            break;
        }
        case DIV:{
            ok = mathlib::div(data->operand1, data->operand2, &result);
            break;
        }
        case POW:{
            ok = mathlib::pow(data->operand1, data->operand2, &result);
            break;
        }
        case FAC:{
            ok = mathlib::fact(data->operand1, &result);
            break;
        }
        default:{
            data->error_type = ERROR_UNKNOWN_ARGUMENT;
            return;
        }
    }

    if(!ok){
        data->error_type = ERROR_OVERFLOW;
        return;
    }

    data->result = result;
}