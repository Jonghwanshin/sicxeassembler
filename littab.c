#include "littab.h"

LITTAB_element* LITTAB = NULL;

void DeployLiteral(unsigned int* PC){
	LITTAB_element* header = LITTAB; 
	while(header != NULL){
		if(header->address == 0){
			header->address = *PC;//Then Deploy
			*PC += header->length;
		}
		header = header->next;
	}
}

LITTAB_element* NewLiteral(char* operand, char type ){
	LITTAB_element* header = LITTAB;
	LITTAB_element* newnode = NULL;
	newnode = (LITTAB_element*)malloc(sizeof(LITTAB_element));
	strcpy(newnode->name,operand);
	if(type == 'C'){
		newnode->value = 0;
		while(*operand){
			newnode->value = (newnode->value << 8) + *operand;	
			operand++;
		}
	}
	else if(type == 'X'){
		newnode->value = (unsigned int)strtoul(operand,NULL,16);
		newnode->length = 1;
	}
	else{
		newnode->value = 0;
		newnode->length = 0;
	}	
	newnode->address = 0;
	newnode->next == NULL;
	if(header != NULL){
		while(header->next != NULL){
			header= header->next;
		}
		header->next = newnode;
	}
	LITTAB = newnode;
}

unsigned int FindLiteral(char* operand){
	LITTAB_element* header = LITTAB;
	while(header!=NULL){
		if(strcmp(header->name,operand)==0){
			return header->address;
		}else{
			header = header->next;
		}
	}
	return 0;
}

unsigned int OperandIsLiteral(char* operand){
	char type;
	char* ptr = strtok(operand,"='");
	int isFound;
	type = *ptr;
	ptr = strtok(NULL,"'");
	isFound = FindLiteral(ptr);
	if(isFound){
		return isFound;
	}else{
		NewLiteral(ptr,type);
		return 0;
	}
}