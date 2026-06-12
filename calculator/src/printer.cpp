#include"../include/printer.hpp"
#include"../include/data.hpp"
#include<stdio.h>

static void printHelp(){
    printf("Usage:\n"
        "  calculator -n <num> -n <num> -o <op>\n\n"
        "Operations:\n"
        "  +  addition\n"
        "  -  subtraction\n"
        "  *  multiplication\n"
        "  /  division\n"
        "  ^  power\n"
        "  !  factorial\n\n"
        "Examples:\n"
        "  calculator -n 10 -n 20 -o +\n"
        "  calculator -n 10 -o !\n");
}

static void printError(ErrorType error){
    switch(error){
        case ERROR_UNKNOWN_ARGUMENT:
            fprintf(stderr, "error: unknown argument\n");
            break;
	    case ERROR_INVALID_NUMBER:
            fprintf(stderr, "error: invalid number\n");
            break;
	    case ERROR_INVALID_OPERAND_COUNT:
            fprintf(stderr, "error: invalid operand count\n");
            break;
	    case ERROR_DIV_BY_ZERO:
            fprintf(stderr, "error: div by zero\n");
            break;
	    case ERROR_OPERATION_NOT_SET:
            fprintf(stderr, "error: operation not set\n");
            break;
	    case ERROR_OVERFLOW:
            fprintf(stderr, "error: overflow\n");
            break;
        default:
            break;
    }
}

void print(const Data *data){
    if(data == 0){
        return;
    }
    if(data->mode == HELP){
        printHelp();
        return;
    }
    if(data->error_type != ERROR_NONE){
        printError(data->error_type);
        return;
    }

    printf("%lld\n", data->result);
}