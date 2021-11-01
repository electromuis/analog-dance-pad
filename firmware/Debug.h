#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdint.h>

void Debug_Message(const char* message);
void Debug_ReadBuffer(char* target, uint16_t length);
uint16_t Debug_Available();

#endif