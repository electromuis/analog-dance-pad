#include "Debug.h"
#include "Config/DancePadConfig.h"

#if defined(FEATURE_DEBUG_ENABLED)

#define DEBUG_BUFFER_SIZE 128

char debugBuffer [DEBUG_BUFFER_SIZE];
volatile uint16_t debugBufferAvailable = 0;

void Debug_MoveBuffer(uint16_t position) {
	debugBufferAvailable -= position;
	memcpy(debugBuffer, debugBuffer + position, debugBufferAvailable);
}

void Debug_Message(const char* message) {
	uint16_t freeBufferSpace = DEBUG_BUFFER_SIZE - debugBufferAvailable;
	uint16_t readLength = strlen(message);
	if(readLength > freeBufferSpace) {
		readLength = freeBufferSpace;
	}
	
	memcpy(debugBuffer + debugBufferAvailable, message, readLength);
	debugBufferAvailable += readLength;
}

void Debug_ReadBuffer(char* target, uint16_t length) {
	if(length > debugBufferAvailable) {
		length = debugBufferAvailable;
	}
	
	memcpy(target, debugBuffer, length);
	Debug_MoveBuffer(length);
}

uint16_t Debug_Available() {
	return debugBufferAvailable;
}

#else

void Debug_Init() {;}
void Debug_Message(const char* message) {;}
void Debug_ReadBuffer(char* target, uint16_t length) {;}
uint16_t Debug_Available() { return 0; }

#endif