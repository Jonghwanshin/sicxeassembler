#ifndef LITTAB_H_
#define LITTAB_H_

#include "includes.h"

struct SIC_LITTAB_S{
	char* name;
	unsigned int value;
	unsigned int length;
	unsigned int address;
	struct SIC_LITTAB_S* next;
};

typedef struct SIC_LITTAB_S LITTAB_element;

extern LITTAB_element* LITTAB;

void DeployLiteral(unsigned int* PC);
LITTAB_element* NewLiteral(char* operand, char type);
unsigned int FindLiteral(char* operand);
unsigned int OperandIsLiteral(char* operand);

#endif