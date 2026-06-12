#pragma once

enum Operation{
	OP_NONE,
	ADD,
	SUB,
	MUL,
	DIV,
	POW,
	FAC
};

enum ErrorType{
	ERROR_NONE,
	ERROR_UNKNOWN_ARGUMENT,
	ERROR_INVALID_NUMBER,
	ERROR_INVALID_OPERAND_COUNT,
	ERROR_DIV_BY_ZERO,
	ERROR_OPERATION_NOT_SET,
	ERROR_OVERFLOW
};

enum Mode{
	CALC,
	HELP
};

struct Data{
	long long operand1;
	long long operand2;
	short opd_count;
	Operation operation;
	long long result;
	ErrorType error_type;
	Mode mode;
};

