#include<stdio.h>
#include<malloc.h>
#include<string.h>

#include"hashtable.h"

#define ULONG_MAX 4294967295
#define BUFSIZE 70

typedef struct{
	char name[7];
	int opcode;
	char format;
} SIC_OPTAB;

//SYMTAB DEFINITION
hashtable_t* SYMTAB;

//OPTAB DEFINITION
static SIC_OPTAB OPTAB[] = {
  {	"ADD", 0x18, 3 },       { "ADDR", 0x90, 2 },    { "AND", 0x40, 3 },
  {	"CLEAR", 0xB4, 2 },     { "COMP", 0x28, 3 },    { "COMPR", 0xA0, 2 },
  {	"DIV", 0x24, 3 },       { "DIVR", 0x9C, 2 },    { "HIO", 0xF4, 1 },
  {	"J", 0x3C, 3 },         { "JEQ", 0x30, 3 },     { "JGT", 0x34, 3 },
  {	"JLT", 0x38, 3 },       { "JSUB", 0x48, 3 },    { "LDA", 0x0, 3 },
  {	"LDB", 0x68, 3 },       { "LDCH", 0x50, 3 },    { "LDL", 0x8, 3 },
  {	"LDS", 0x6C, 3 },       { "LDT", 0x74, 3 },     { "LDX", 0x4, 3 },
  { "LPS", 0xD0, 3 },       { "MUL", 0x20, 3 },     { "MULR", 0x98, 2 },
  { "OR", 0x44, 3 },        { "RD", 0xD8, 3 },      { "RMO", 0xAC, 2 },
  { "RSUB", 0x4C, 3 },      { "SHIFTL", 0xA4, 2 },  { "SHIFTR", 0xA8, 2 },
  { "SIO", 0xF0, 1 },       { "SSK", 0xEC, 3 },     { "STA", 0x0C, 3 },
  { "STB", 0x78, 3 },       { "STCH", 0x54, 3 },    { "STI", 0xD4, 3 },
  { "STL", 0x14, 3 },       { "STS", 0x7C, 3 },     { "STSW", 0xE8, 3 },
  { "STT", 0x84, 3 },       { "STX", 0x10, 3 },     { "SUB", 0x1C, 3 },
  { "SUBR", 0x94, 2 },      { "SVC", 0xB0, 2 },     { "TD", 0xE0, 3 },
  { "TIO", 0xF8, 1 },       { "TIX", 0x2C, 3 },     { "TIXR", 0xB8, 2 },
  { "WD", 0xDC, 3 },
  //oh, what the hell, let's put in floating point instructions, too
  { "ADDF", 0x58, 3 },      { "COMPF", 0x88, 3 },   { "DIVF", 0x64, 3 },
  { "FIX", 0xC4, 1 },       { "FLOAT", 0xC0, 1 },   { "LDF", 0x70, 3 },
  { "MULF", 0x60, 3 },      { "NORM", 0xC8, 1 },    { "STF", 0x80, 3 },
  { "SUBF", 0x5C, 3 }
};
#define NUM_OPCODES         (sizeof(OPTAB) / sizeof(SIC_OPTAB))	//

//initialize register table
//source : http://dmitrybrant.com/2003/11/02/assembler-and-linker-for-sicxe
static SIC_OPTAB REGTAB  [] = {
  { "A", 0x0, 0 },   { "X", 0x1, 0 },   { "L", 0x2, 0 },   { "PC", 0x8, 0 },
  { "SW", 0x9, 0 },   { "B", 0x3, 0 },   { "S", 0x4, 0 },   { "T", 0x5, 0 },   { "F", 0x6, 0 },
};
#define NUM_REGS         (sizeof(REGTAB) / sizeof(SIC_OPTAB))

//LITTAB Definition
hashtable_t* LITTAB;

//START(0), END(1), RESW(2), RESB(3), BYTE(4), WORD(5), EQU(6), ORG(7), LTORG(8)
static SIC_OPTAB ASMTAB[] = {
	{"START",0xF0,0}, {"END",0xF1,1}, {"RESW",0xF2,2}, 
	{"RESB", 0xF3,3}, {"BYTE",0xF4,4}, {"WORD",0xF5,5},
	{"EQU",0xF6,6}, {"ORG",0xF7,7},	 {"LTORG",0xF8,8}, {"BASE",0xF9,9},};
#define NUM_ASMDIRS			(sizeof(ASMTAB)/sizeof(SIC_OPTAB))

//LOCCTR(GLOBAL, DEFAULT, CDATA, CBLKS
unsigned int* presentLOCCTR = NULL;
unsigned int LOCCTR[4] = {0,};

//Register Definition
int A = 0;
int X = 0;
int L = 0;
int BASE = 0;
int PC = 0;

//CONTROL SIGNAL
int letThisLinePass = 0;
int isDone = 0;
int statusBit = 0;
int whereIsOnOPTAB = 0;
int whereIsOnASMTAB = 0;
int whichPass = 1;
int previousLOCCTR = 0;
#define isExtended		1 << 12
#define isPCRelative	1 << 13
#define isBaseRelative	1 << 14
#define isIndexed		1 << 15
#define isImmediate		1 << 16
#define isIndirect		1 << 17

//Inforamtion
int startAddress = 0;
int endAddress = 0;

//INPUT FILE POINTER AND OUTPUT STREAM
FILE *fp;
char stream[BUFSIZE];
char outputHeaderBuffer[BUFSIZE];
char outputBuffer[BUFSIZE+1];
char* outputBufferPointer = NULL;
int ptrOutputBuffer = 0;

int FindTAB(SIC_OPTAB* table,char* opcode_mnemonic);
char StartsWith(char* string);
void PrintOpCode(unsigned int operandToNumber, unsigned int format);
void PrintBuffer(unsigned int flush);

int TokenParser(char* symboldef, char* opcode_mnemonic, char* operand);
int Decoder(char* symboldef, char* opcode_mnemonic, char* operand, unsigned int* opcode);
int Translator(char* symboldef, char* opcode_mnemonic, char* operand, unsigned int* opcode);

void SinglePass(char* filename,char* symboldef, char* opcode_mnemonic, char* operand, unsigned int* opcode);

int main(){
	char* filename = (char *)malloc(20*sizeof(char));
	char* symboldef = (char *)malloc(5*sizeof(char));
	char* opcode_mnemonic = (char *)malloc(4*sizeof(char));
	char* operand = (char *)malloc(10*sizeof(char));
	unsigned int* opcode = (unsigned int *)malloc(sizeof(int));
	outputBufferPointer = &outputBuffer;

	SYMTAB = ht_create(100);
	LITTAB = ht_create(100);
	printf("_______________SIC/XE Assembler_______________\n");
	printf("Assemble을 원하는 File의 이름을 입력해주세요.\n Filename >");
	scanf("%s",filename);

	SinglePass(filename,symboldef,opcode_mnemonic,operand,opcode);
	whichPass = 2;
	SinglePass(filename,symboldef,opcode_mnemonic,operand,opcode);
	
	filename = NULL;
	symboldef = NULL;
	opcode_mnemonic = NULL;
	operand = NULL;
	opcode = NULL;

	free(filename);
	free(symboldef);
	free(opcode_mnemonic);
	free(operand);
	free(opcode);
	fclose(fp);
	return 0;
}

void SinglePass(char* filename,char* symboldef, char* opcode_mnemonic, char* operand, unsigned int* opcode){

	fp=fopen(filename,"r");

	if(fp == NULL){
		printf("해당 파일을 읽을 수 없습니다.\n");
		return;
	}

	while(!feof(fp) && !isDone){
		TokenParser(symboldef,opcode_mnemonic,operand);
		if(letThisLinePass == 0){
			Decoder(symboldef,opcode_mnemonic,operand,opcode);
			Translator(symboldef,opcode_mnemonic,operand,opcode);
		}
		if(feof(fp)){
			//return 0;
		}
		//letThisLinePass=0;
		//Initalize control signals before next line
		statusBit = 0;
		whereIsOnOPTAB = 0;
		whereIsOnASMTAB = 0;
	}

	if(whichPass == 2){
		//PrintBuffer(1);		//Print Last Line
		//
	}

	fclose(fp);
	letThisLinePass=0;
	isDone = 0;
	whereIsOnOPTAB =0;
	whereIsOnASMTAB =0;

}

//Parser
int TokenParser(char* symboldef, char* opcode_mnemonic, char* operand){
	//READ A LINE
	char token[3][10] = {'\0',};
	char* token_pointer=NULL;
	int i = 0, temp = 0;

	if(fgets(stream,BUFSIZE,fp) != NULL){
		token_pointer = strtok(stream," \n\t");
		while(token_pointer != NULL && i <= 2)
		{
			//IF TOKEN STARTS WITH ., THE TOKEN IS COMMMENT
			if(strchr(token_pointer,'.') - token_pointer == 0){
				letThisLinePass=1;
				i=4; //ESCAPE
			}
			else{
				//printf("%s ",token_pointer); //FOR DEBUG
				letThisLinePass=0;
				strcpy(token[i], token_pointer);	//COPY TO EACH TOKEN ARRAY ELEMENT
				token_pointer = strtok(NULL,". \n\t"); //NULL
				i++;
			}
		}
	}
	//printf("\n"); FOR DEBUG
	switch(i)
	{
		case 1:
			*symboldef='\0';
			strcpy(opcode_mnemonic,token[0]);
			*operand='\0';
			break;
		case 2:
			//opcode_mnemonic / OPERAND
			*symboldef='\0';
			strcpy(opcode_mnemonic,token[0]);
			strcpy(operand,token[1]);
			//printf("%s %s\n",opcode_mnemonic,operand); //FOR DEBUG
			break;
		case 3:
			//SYMDEF/opcode_mnemonic/OPERAND
			strcpy(symboldef,token[0]);
			strcpy(opcode_mnemonic,token[1]);
			strcpy(operand,token[2]);
			//printf("%s %s %s\n",symboldef,opcode_mnemonic,operand); //FOR DEBUG
			break;
		default:
			//THIS LINE HAS COMMENT LINE OR CANNOT BE RECOGNIZED, DON'T DO ANYTHING.
			break;
	}
}

int Decoder(char* symboldef, char* opcode_mnemonic, char* operand, unsigned int* opcode){
	char mode = StartsWith(operand);
	char* ptr = NULL;
	//Assembler Directive/Operation Code구분(opcode_mnemonic 처리)
	if(StartsWith(opcode_mnemonic) == '+') //Extended Address
	{
		opcode_mnemonic = strtok(opcode_mnemonic,"+");	//remove Extended Sign
		statusBit |= isExtended;	//set extended_flag++;
	}

	//find OP_TAB
	whereIsOnOPTAB = FindTAB(OPTAB,opcode_mnemonic);
	if(whereIsOnOPTAB>=0){ //if found
		*opcode = OPTAB[whereIsOnOPTAB].opcode;			//change mnemonic to opcode_mnemonic
		//printf("%X\n",*opcode);						//FOR DEBUG
		LOCCTR[0] = PC;									//increase locctr by format of instruction
		PC += OPTAB[whereIsOnOPTAB].format;
		if(statusBit & isExtended){
			PC += 4-OPTAB[whereIsOnOPTAB].format;		//In the Case of Extended, the Opcode Format is 4.
		}
		//printf("%04X %04X",LOCCTR[0],PC);				//FOR DEBUG : PRINT CURRENT LOCCTR AND PC
	}
	else{
		//if assembler directive : START(0), RESW(1), RESB(2), BYTE(3), EQU(4), ORG(5)
		LOCCTR[0] = PC;
		whereIsOnASMTAB=FindTAB(ASMTAB,opcode_mnemonic);
		*opcode = ASMTAB[whereIsOnASMTAB].opcode;
	}

	//ADD SYMBOL DEFINITION AND ADDRESS TO SYMTAB(symboldef 처리)
	if(*symboldef != NULL){
		ht_set(SYMTAB,symboldef, LOCCTR[0],LOCCTR[0]);
	}

	//symbol reference and identify addressing mode (operand 처리)
	//identifying addressing mode
	ptr = strtok(operand,"#@");
	if(ptr!=NULL){
		strcpy(operand,ptr);
	}
	//operand = ptr;
	if(mode == '@') {
		statusBit |= isIndirect;
	} else if (mode == '#') {
		statusBit |= isImmediate;
	} else if (OPTAB[whereIsOnOPTAB].format > 2){
		statusBit |= isIndirect | isImmediate;
	}
	if(EndsWith(operand,",X") == 1){
		strtok(operand,",");
		statusBit |= isIndexed;
		//printf("%s",operand); //FOR DEBUG
	}
}

// Translator
int Translator(char* symboldef, char* opcode_mnemonic, char *operand, unsigned int* opcode){
	//Address Calculation
	entry_t* searchResultSYMTAB;
	int operandToNumber;
	unsigned int operationCode;
	char mode = StartsWith(operand);
	//char* headerbuffer = &outputHeaderBuffer;
	/* For the record, ASMTAB looks like this
	{"START",0,0xF0}, {"END",1,0xF1}, {"RESW",2,0xF2}, {"RESB",3,0xF3}, 
	{"BYTE",4,0xF4}, {"WORD",5,0xF5}, {"EQU",6,0xF6},  {"ORG",7,0xF7},	{"LTORG",8,0xF8},*/
	if(whereIsOnOPTAB < 0){	//Assembler Directive
		switch(*opcode & 0xF)
		{
			case 0:
				startAddress = (int)strtoul(operand,NULL,16);
				PC = startAddress;
				LOCCTR[0] = PC-3;
				if(whichPass==2){
					printf("H%-6s%06X%06X\n",symboldef,startAddress,endAddress-startAddress);
				}
				break;
			case 1:
				isDone = 1;
				endAddress = LOCCTR[0];
				PC = (int)strtoul(operand,NULL,16);
				//sprintf(headerbuffer+13,"%06X\n",endAddress);
				//printf("%s",headerbuffer);
				break;
			case 2:
				operandToNumber = atoi(operand);
				//printf("%04X\n", LOCCTR[0]);
				//LOCCTR[0]+=3*operandToNumber;
				PC+=3*operandToNumber;
				LOCCTR[0] = PC;
				break;
			case 3:
				operandToNumber = atoi(operand);
				//printf("%04X\n", LOCCTR[0]);
				//LOCCTR[0]+=1*operandToNumber;
				PC+=1*operandToNumber;
				LOCCTR[0] = PC;
				break;
			case 4:
				//print the value of operand to buffer
				//LOCCTR[0] = PC;
				//printf("%04X ", LOCCTR[0]);
				operand=strtok(operand,"'CX");
				if(mode == 'C'){//'Character'
					while(*operand != '\0')
					{
						//printf("%2X",*operand);
						PrintOpCode(*operand,1);
						//LOCCTR[0]+=1;
						PC+=1;
						LOCCTR[0] = PC;
						operand++;
					}
					//printf("\n");
				} else if(mode == 'X'){//'Hex Value'
					//printf("%s\n",operand);
					PrintOpCode(*operand,1);
					//LOCCTR[0]+=1;
					PC+=1;
					//LOCCTR[0] = PC;
				}
				LOCCTR[0]=PC;
				break;
			case 5:
				//print the value of operand to buffer
				//LOCCTR[0]+=3;
				PC+=3;
				LOCCTR[0] = PC;
				break;
			case 6:
				//replace symbol to operand, find symbol and set
				ht_set(SYMTAB,symboldef,atoi(operand),LOCCTR[0]);
				break;
			case 7:
				//set location value to operand
				//ht_set
				break;
			case 8:
				//print all the contents in the LITTAB which are not located somewhere.
				break;
			case 9:
				//Set Base to operand value
				if(mode == '*'){ //current LOCCTR
					BASE = PC;//LOCCTR[0];
				} else if(mode >= '0' && mode <= '9'){ //#number
					BASE = atoi(operand);
				} else{ //symbol
					searchResultSYMTAB = ht_get(SYMTAB,operand);
					if(operand == NULL){
					} else if(searchResultSYMTAB == NULL || searchResultSYMTAB->value == 0){
						ht_set(SYMTAB,operand,0,LOCCTR[0]);	//Setting a SYMBOL without address and add unresolved address
					} else {
						if(!(statusBit & isExtended)){			//if not extended, we need to calculate address relatively
							BASE = searchResultSYMTAB->value; //First, calculate address relative to PC
						}
					}
				}
				break;
		}
	} else if(OPTAB[whereIsOnOPTAB].format == 1){
		operandToNumber = *opcode;
		//printf("%02X\n", operandToNumber);
		PrintOpCode(operandToNumber,2);
	} else if(OPTAB[whereIsOnOPTAB].format == 2){
		char* ptr;
		int whereIsOnREGTAB1 = 0;
		int whereIsOnREGTAB2 = 0;
		ptr = strpbrk(operand,",");
		if(ptr == NULL){//only one register
			whereIsOnREGTAB1 = FindTAB(REGTAB,operand);	
			operandToNumber = ((*opcode) << 8) | (REGTAB[whereIsOnREGTAB1].opcode <<4);
			//printf("%04X\n", operandToNumber);
			PrintOpCode(operandToNumber,2);
		}else{
			strtok(operand,",");
			whereIsOnREGTAB1 = FindTAB(REGTAB,operand);
			whereIsOnREGTAB2 = FindTAB(REGTAB,(ptr+1));	
			operandToNumber = ((*opcode) << 8) | (REGTAB[whereIsOnREGTAB1].opcode <<4) | (REGTAB[whereIsOnREGTAB2].opcode);
			//printf("%04X\n", operandToNumber);
			PrintOpCode(operandToNumber,2);
		}		
	} else { //Operation
		if(mode == '='){	//This is Literal
			mode = StartsWith(strtok(operand,"='"));
			operand++;									// Remove Delimiter
			ht_get(LITTAB,operand);						// Find this from LITTAB
			ht_set(LITTAB,operand,0,LOCCTR[0]);			// Insert this to LITTAB
		} else if(mode == '*'){
			operandToNumber = ((*opcode) << 16) | LOCCTR[0]&0xFFFF;
			printf("%06X\n",operandToNumber);
		} else if(mode >= '0' && mode <= '9'){ //This is number
			int disp = atoi(operand);
			if(disp >= 4096){
				operandToNumber = ((*opcode) << 24 | statusBit<<8 | (disp & 0x0FFFFF));
				//printf("%08X\n", operandToNumber);
				PrintOpCode(operandToNumber,4);
			}
			else{
				operandToNumber = ((*opcode) << 16 | statusBit | (disp & 0x0FFF));
				//printf("%06X\n", operandToNumber);
				PrintOpCode(operandToNumber,3);
			}
		} else{
			searchResultSYMTAB = ht_get(SYMTAB,operand);
			if(operand == NULL){
				operandToNumber = ((*opcode) << 16);
			} else if(searchResultSYMTAB == NULL || searchResultSYMTAB->value == 0){
				//Couldn't Find Definition or Address in SYMTAB 
				ht_set(SYMTAB,operand,0,LOCCTR[0]);	//Setting a SYMBOL without address and add unresolved address
				if(!(statusBit & isExtended)){
					operandToNumber = ((*opcode) << 16 | statusBit);
					//printf("%06X\n", operandToNumber);
					PrintOpCode(operandToNumber,3);
				} else {
					operandToNumber = (((*opcode) << 16 | statusBit) << 8);
					//printf("%08X\n", operandToNumber);
					PrintOpCode(operandToNumber,4);
				}
			} else{
				//already defined and founded		
				if(!(statusBit & isExtended)){	//if not extended, we need to calculate address relatively
					//we need to calculate address with respect to PC
					int diff = 0;
					diff = searchResultSYMTAB->value - PC;
					if (diff >= -2047 && diff <= 2048){
						//This is PC Relative
						statusBit |= isPCRelative;
					} else {
						diff = searchResultSYMTAB->value - BASE;
						if( diff >= 0 && diff <= 4095){
							//this is Base Relative
							statusBit |= isBaseRelative;
						} else{
							printf("\nERROR: OVERFLOW, THE ADDRESS CANNOT BE CALCULATED RELATIVELY\n");
							return 0;
						}
					}
					operandToNumber = ((*opcode) << 16 | statusBit | (diff & 0x0FFF));
					//printf("%06X\n", operandToNumber);
					PrintOpCode(operandToNumber,3);
				} else{
					//Extended Situation
					operandToNumber = ((*opcode) << 24 | statusBit<<8 | (searchResultSYMTAB->value & 0x0FFFFF));
					//printf("%08X\n", operandToNumber);
					PrintOpCode(operandToNumber,4);
				}
			}
		}
	}
}

void PrintOpCode(unsigned int operandToNumber, unsigned int format){
	if(whichPass == 2){
		if(LOCCTR[0] != previousLOCCTR){	//Address got changed, new line
			PrintBuffer(1);
		}
		if(ptrOutputBuffer == 0){	//If empty buffer, mark 'T' and start address
			sprintf(outputBufferPointer,"T%06X00", LOCCTR[0]);
			ptrOutputBuffer+=9;
		}
		if((ptrOutputBuffer+format) >= BUFSIZE - 2){	//Buffer is about to overflow
			PrintBuffer(1);
			sprintf(outputBufferPointer,"T%06X00", LOCCTR[0]);
			ptrOutputBuffer+=9;
		}
		switch(format){	//Print machine code to buffer
			case 1:
				sprintf(outputBufferPointer+ptrOutputBuffer,"%02X",operandToNumber);
				ptrOutputBuffer+=2;
				break;
			case 2:
				sprintf(outputBufferPointer+ptrOutputBuffer,"%04X",operandToNumber);
				ptrOutputBuffer+=4;
				break;
			case 3:
				sprintf(outputBufferPointer+ptrOutputBuffer,"%06X",operandToNumber);
				ptrOutputBuffer+=6;
				break;
			case 4:	
				sprintf(outputBufferPointer+ptrOutputBuffer,"%08X",operandToNumber);
				ptrOutputBuffer+=8;
				break;
			}
			previousLOCCTR = LOCCTR[0]+format;
		if(LOCCTR[0]+format == endAddress){
			PrintBuffer(1);
		}
	} else {
		return;
	}
}

void PrintBuffer(unsigned int flush){
	if(whichPass == 2){
		if(flush){	//flush current buffer
			char temp = outputBuffer[9];
			sprintf(outputBufferPointer+7,"%02X",(ptrOutputBuffer-9)/2);	//Insert Length to outputBufferPointer 
			outputBuffer[9] = temp;
			printf("%s\n",outputBuffer);
			outputBufferPointer[0] = '\0';
			ptrOutputBuffer = 0;
		}
		
	}
}

char StartsWith(char* string){ 
	//return the character which is the start of input string
	return string[0];
}

int EndsWith(char* string, char* suffix){ 
	//source : http://stackoverflow.com/questions/744766/how-to-compare-ends-of-strings-in-c
	size_t lenstr = strlen(string);
    size_t lensuffix = strlen(suffix);
	if(!string||!suffix)
		return 0;
    if (lensuffix > lenstr)
        return 0;
    return strncmp(string + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int FindTAB(SIC_OPTAB* table,char* opcode_mnemonic){
	char start = StartsWith(opcode_mnemonic);
	int index_start = 0, index_end=0;
	int i = 0;
	if(table == OPTAB){
		index_end = NUM_OPCODES;
	}else if(table == ASMTAB){
		index_end = NUM_ASMDIRS;
	}else{
		index_end = NUM_REGS;
	}

	for(i=index_start; i<index_end; i++){
		if(strcmp(opcode_mnemonic,table[i].name) == 0){
			return i;
		}
	}
	
	return -1;

}