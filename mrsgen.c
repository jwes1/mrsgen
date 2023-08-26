#include<stdio.h>
#include<windows.h>
#include "resource.h"

#define OPERATION_ADD 0
#define OPERATION_SUB 1
#define OPERATION_XOR 2
#define OPERATION_ROL 3
#define OPERATION_ROR 4

#define MRSEXE_CODE_OFFSET 0xD375

#define OPCODE_ROR 0xC8C0
#define OPCODE_ROL 0xC0C0
#define OPCODE_ADD 0x04
#define OPCODE_SUB 0x2C
#define OPCODE_XOR 0x34

typedef struct _operation{
	BYTE type;
	BYTE value;
} operation;

typedef struct _operationlist{
	operation* oplist;
	WORD       size;
} operation_list;

void operation_list_start(operation_list* l){
	l->oplist = NULL;
	l->size   = 0;
}
void operation_list_add(operation_list* l, BYTE t, BYTE v){
	int i = l->size;
	l->oplist = (operation*)realloc(l->oplist, (i+1)*sizeof(operation));
	l->oplist[i].type  = t;
	l->oplist[i].value = v;
	l->size++;
}
void operation_list_free(operation_list* l){
	if(l->oplist)
		free(l->oplist);
	l->oplist=NULL;
	l->size=0;
}

char* operations[]={
	"ADD",
	"SUB",
	"XOR",
	"ROL",
	"ROR"
};

HANDLE wstdout;
HANDLE wstdin;
CONSOLE_SCREEN_BUFFER_INFO csbi = {0};
CONSOLE_CURSOR_INFO ccInfo;
COORD cInit;
COORD cTemp, cTemp2;
COORD cOperationList;
COORD cSetValue;

void GetCursorCurrentPosition(COORD* c){
	CONSOLE_SCREEN_BUFFER_INFO temp_csbi={0};
	GetConsoleScreenBufferInfo(wstdout, &temp_csbi);
	(*c)=temp_csbi.dwCursorPosition;
}

#define STATE_SELECT		0
#define STATE_EXPECT_HEX	1
#define STATE_LEAVE			2

int main(){
	INPUT_RECORD ir;
	DWORD nTemp;
	DWORD nIndex = 0;
	BYTE  state;
	BYTE  hexLen=0;
	BYTE  hexVal=0;
	BYTE  lastHex = 0;
	operation_list oplist;
	int i;
	
	HRSRC mrsExe;
	HGLOBAL mrsExeHandle;
	char* mrsExeBuffer;
	
	FILE* mrsExeOut = NULL;
	
	operation_list_start(&oplist);
	
	wstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	wstdin  = GetStdHandle(STD_INPUT_HANDLE);
	
	GetConsoleScreenBufferInfo(wstdout, &csbi);
	
	GetCursorCurrentPosition(&cOperationList);
	cOperationList.X += 50;
	printf("Select an operation below:\n\n\n");
	GetCursorCurrentPosition(&cSetValue);
	cSetValue.X+=3;
	printf("   Use arrows LEFT and RIGHT and press ENTER\n   to select an operation, ESC to exit\n");
	SetConsoleCursorPosition(wstdout, cOperationList);
	printf("Operations added:\n");
	
	GetCursorCurrentPosition(&cInit);
	cInit.X += 3;
	SetConsoleCursorPosition(wstdout, cInit);
	
	GetConsoleCursorInfo(wstdout, &ccInfo);
	ccInfo.bVisible = FALSE;
	SetConsoleCursorInfo(wstdout, &ccInfo);
	SetConsoleTextAttribute(wstdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
	printf("%-5s %-5s %-5s %-5s %-s\n", "ADD", "SUB", "XOR", "ROL", "ROR");
	SetConsoleTextAttribute(wstdout, csbi.wAttributes);
	
	cTemp = cInit;
	cTemp.X--;
	FillConsoleOutputAttribute(wstdout, BACKGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, strlen(operations[nIndex])+2, cTemp, &nTemp);
	
	state = STATE_SELECT;
	
	while(1){
		ReadConsoleInput(wstdin, &ir, 1, &nTemp);
		if(ir.EventType == KEY_EVENT){
			if(ir.Event.KeyEvent.bKeyDown){
				nTemp = ir.Event.KeyEvent.wVirtualKeyCode;
				// printf("[%02X]\n", ir.Event.KeyEvent.wVirtualKeyCode);
				switch(nTemp){
					case VK_ESCAPE:
						if(state==STATE_SELECT){
							state = STATE_LEAVE;
						}
						break;
					case VK_LEFT:
						if(state == STATE_SELECT){
							FillConsoleOutputAttribute(wstdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, strlen(operations[nIndex])+2, cTemp, &nTemp);
							nIndex--;
							if((int)nIndex<0)
								nIndex=(sizeof(operations)/sizeof(char*))-1;
							cTemp = cInit;
							cTemp.X += 6*nIndex-1;
							// SetConsoleCursorPosition(wstdout, cTemp);
							FillConsoleOutputAttribute(wstdout, BACKGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, strlen(operations[nIndex])+2, cTemp, &nTemp);
							// printf("%d\n", nIndex);
						}
						break;
					case VK_RIGHT:
						if(state == STATE_SELECT){
							FillConsoleOutputAttribute(wstdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, strlen(operations[nIndex])+2, cTemp, &nTemp);
							// printf("<right>\n");
							nIndex++;
							nIndex%=sizeof(operations)/sizeof(char*);
							cTemp = cInit;
							cTemp.X += 6*nIndex-1;
							// SetConsoleCursorPosition(wstdout, cTemp);
							FillConsoleOutputAttribute(wstdout, BACKGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, strlen(operations[nIndex])+2, cTemp, &nTemp);
							// printf("%d\n", nIndex);
						}
						break;
					case VK_RETURN:
						if(state == STATE_SELECT){
							SetConsoleCursorPosition(wstdout, cSetValue);
							FillConsoleOutputCharacter(wstdout, ' ', 44, cSetValue, &nTemp);
							cSetValue.Y++;
							FillConsoleOutputCharacter(wstdout, ' ', 44, cSetValue, &nTemp);
							cSetValue.Y--;
							printf("\n     Type an hexadecimal value\n");
							SetConsoleCursorPosition(wstdout, cSetValue);
							printf("%s AL, ", operations[nIndex]);
							state = STATE_EXPECT_HEX;
							hexVal = 0;
							hexLen = 0;
							lastHex = 0;
							ccInfo.bVisible=TRUE;
							SetConsoleCursorInfo(wstdout, &ccInfo);
							SetConsoleTextAttribute(wstdout, FOREGROUND_GREEN|FOREGROUND_INTENSITY);
						}else if(state == STATE_EXPECT_HEX){
							if(hexLen){
								operation_list_add(&oplist, nIndex, hexVal);
								FillConsoleOutputCharacter(wstdout, ' ', 44, cSetValue, &nTemp);
								FillConsoleOutputAttribute(wstdout, csbi.wAttributes, 44, cSetValue, &nTemp);
								cSetValue.Y++;
								FillConsoleOutputCharacter(wstdout, ' ', 44, cSetValue, &nTemp);
								FillConsoleOutputAttribute(wstdout, csbi.wAttributes, 44, cSetValue, &nTemp);
								cSetValue.Y--;
								cOperationList.Y++;
								SetConsoleCursorPosition(wstdout, cOperationList);
								SetConsoleTextAttribute(wstdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
								printf("  %s AL, %02Xh", operations[nIndex], hexVal);
								ccInfo.bVisible = FALSE;
								SetConsoleCursorInfo(wstdout, &ccInfo);
								SetConsoleCursorPosition(wstdout, cSetValue);
								SetConsoleTextAttribute(wstdout, csbi.wAttributes);
								printf("Use arrows LEFT and RIGHT and press ENTER\n   to select an operation, ESC to exit\n");
								state = STATE_SELECT;
							}
							// printf("\n%02X\n", hexVal);
						}
						break;
					case VK_BACK:
						if(state == STATE_EXPECT_HEX){
							if(hexLen){
								hexVal-=lastHex;
								hexVal/=16;
								GetCursorCurrentPosition(&cTemp2);
								cTemp2.X-=1;
								FillConsoleOutputCharacter(wstdout, ' ', 1, cTemp2, &nTemp);
								SetConsoleCursorPosition(wstdout, cTemp2);
								hexLen--;
							}
						}
						break; 
					default:
						if(state == STATE_EXPECT_HEX){
							nTemp = ir.Event.KeyEvent.uChar.AsciiChar;
							if(hexLen<2 && ((toupper(nTemp)>='A' && toupper(nTemp)<='F') || isdigit(nTemp))){
								putchar(nTemp);
								hexVal *= 16;
								lastHex = (toupper(nTemp)>='A'&&toupper(nTemp)<='F')?10+toupper(nTemp)-'A':nTemp-'0';
								hexVal += lastHex;
								hexLen++;
							}
						}
				}
			}
		}
		if(state==STATE_LEAVE)
			break;
	}
	printf("\n\n\n");
	if(oplist.size){
		mrsExe = FindResource(NULL, MAKEINTRESOURCE(IDB_MRSEXE), RT_RCDATA);
		if(!mrsExe){
			fprintf(stderr, "Cannot create encrypted mrs.exe\n");
			operation_list_free(&oplist);
			Sleep(2000);
			return 1;
		}
		mrsExeHandle = LoadResource(NULL, mrsExe);
		if(!mrsExeHandle){
			fprintf(stderr, "Cannot create encrypted mrs.exe\n");
			operation_list_free(&oplist);
			Sleep(2000);
			return 1;
		}
		mrsExeBuffer = LockResource(mrsExeHandle);
		if(!mrsExeBuffer){
			fprintf(stderr, "Cannot create encrypted mrs.exe\n");
			operation_list_free(&oplist);
			Sleep(2000);
			return 1;
		}

		nIndex = 0;
		
		for(i=0; i<oplist.size; i++){
			switch(oplist.oplist[i].type){
				case OPERATION_ADD:
					nTemp = OPCODE_ADD;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					nTemp = oplist.oplist[i].value;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					break;
				case OPERATION_SUB:
					nTemp = OPCODE_SUB;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					nTemp = oplist.oplist[i].value;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					break;
				case OPERATION_XOR:
					nTemp = OPCODE_XOR;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					nTemp = oplist.oplist[i].value;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					break;
				case OPERATION_ROL:
					nTemp = OPCODE_ROL;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 2);
					nIndex+=2;
					nTemp = oplist.oplist[i].value;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					break;
				case OPERATION_ROR:
					nTemp = OPCODE_ROR;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 2);
					nIndex+=2;
					nTemp = oplist.oplist[i].value;
					memcpy(mrsExeBuffer+MRSEXE_CODE_OFFSET+nIndex, &nTemp, 1);
					nIndex++;
					break;
			}
		}
		
		mrsExeOut = fopen("mrs_enc.exe", "wb");
		if(!mrsExeOut){
			fprintf(stderr, "Cannot create encrypted mrs.exe\n");
			operation_list_free(&oplist);
			Sleep(2000);
			return 1;
		}
		
		nTemp = SizeofResource(NULL, mrsExe);
		if(!nTemp){
			fprintf(stderr, "Cannot create encrypted mrs.exe\n");
			fclose(mrsExeOut);
			operation_list_free(&oplist);
			Sleep(2000);
			return 1;
		}
		fwrite(mrsExeBuffer, nTemp, 1, mrsExeOut);
		fclose(mrsExeOut);
		
		operation_list_free(&oplist);
		
		printf("Saved to mrs_enc.exe\n");
	}else{
		fprintf(stderr, "No operations :C\n");
	}
	
	ccInfo.bVisible = TRUE;
	SetConsoleCursorInfo(wstdout, &ccInfo);
	
	Sleep(2000);
	
	return 0;
}
