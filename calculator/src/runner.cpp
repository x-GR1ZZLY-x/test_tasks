#include"../include/runner.hpp"
#include"../include/data.hpp"
#include"../include/parser.hpp"
#include"../include/checker.hpp"
#include"../include/calculator.hpp"
#include"../include/printer.hpp"

static void initData(Data *data){
	data->operand1 = 0;
	data->operand2 = 0;
	data->opd_count = 0;
	data->operation = OP_NONE;
	data->result = 0;
	data->error_type = ERROR_NONE;
	data->mode = CALC;
}

void run(int argc, char** argv){
	Data data;

	initData(&data);

	parse(argc, argv, &data);

	if(data.mode == CALC){
		check(&data);
		calculate(&data);
	}

	print(&data);
}
