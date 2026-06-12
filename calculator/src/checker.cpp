#include"checker.hpp"
#include"data.hpp"

void check(Data *data){
    if(data->mode == HELP) return;

    if(data->operation == OP_NONE){
        data->error_type = ERROR_OPERATION_NOT_SET;
        return;
    }

    if(data->operation == FAC){
        if(data->opd_count != 1){
            data->error_type = ERROR_INVALID_OPERAND_COUNT;
            return;
        }
        if(data->operand1 < 0){
            data->error_type = ERROR_INVALID_NUMBER;
            return;
        }
    }else{
        if(data->opd_count != 2){
            data->error_type = ERROR_INVALID_OPERAND_COUNT;
            return;
        }
        if(data->operation == DIV && data->operand2 == 0){
            data->error_type = ERROR_DIV_BY_ZERO;
            return;
        }
        if(data->operation == POW && data->operand2 < 0){
            data->error_type = ERROR_INVALID_NUMBER;
            return;
        }
    }

    data->error_type = ERROR_NONE;
}