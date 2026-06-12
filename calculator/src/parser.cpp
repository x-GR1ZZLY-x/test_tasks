#include"../include/parser.hpp"
#include"../include/data.hpp"
#include<getopt.h>
#include<stdlib.h>
#include<errno.h>

static int parseNumber(char *text, long long *value){
	char *end = 0;
	errno = 0;
	long long result = strtoll(text, &end, 10);

	if(errno != 0) return 0;

	if(*end != '\0') return 0;

	*value = result;

	return 1;
}

static int parseOperation(char *text, Operation *operation){
	if(text == 0) return 0;

	if(text[0] == '+' && text[1] == '\0'){
		*operation = ADD;
		return 1;
	}

	if(text[0] == '-' && text[1] == '\0'){
		*operation = SUB;
		return 1;
	}

	if(text[0] == '*' && text[1] == '\0'){
		*operation = MUL;
		return 1;
	}

	if(text[0] == '/' && text[1] == '\0'){
		*operation = DIV;
		return 1;
	}

	if(text[0] == '^' && text[1] == '\0'){
		*operation = POW;
		return 1;
	}

	if(text[0] == '!' && text[1] == '\0'){
		*operation = FAC;
		return 1;
	}
	return 0;
}

void parse(int argc, char** argv, Data *data){
	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	int option = 0;

	while((option = getopt_long(argc, argv, "n:o:",
			long_options, 0)) != -1){
		switch(option){
			case 'h':{
				data->mode = HELP;
				return;
			}
			case 'n':{
				long long value = 0;
				if(!parseNumber(optarg, &value)){
					data->error_type = ERROR_INVALID_NUMBER;
					return;
				}
				if(data->opd_count == 0){
					data->operand1 = value;
				}else if(data->opd_count == 1){
					data->operand2 = value;
				}
				data->opd_count++;
				break;
			}
			case 'o':{
				if(!parseOperation(optarg, &data->operation)){
					data->error_type = ERROR_UNKNOWN_ARGUMENT;
					return;
				}
				break;
			}
			default:{
				data->error_type = ERROR_UNKNOWN_ARGUMENT;
				return;
			}
		}
	}


}
