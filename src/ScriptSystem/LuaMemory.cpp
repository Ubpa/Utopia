#include "LuaMemory.h"

#include <cstring>
#include <cstdlib>

using namespace Ubpa::DustEngine;

void* LuaMemory::Malloc(size_t size) {
	return malloc(size);
}

void LuaMemory::Free(void* block) {
	free(block);
}

void* LuaMemory::Offset(void* ptr, size_t n) {
	return (unsigned char*)ptr + n;
}

void LuaMemory::Copy(void* dst, void* src, size_t size) {
	memcpy(dst, src, size);
}

void LuaMemory::Set(void* ptr, int value, size_t size) {
	memset(ptr, value, size);
}

void LuaMemory::StrCpy(char* dst, const char* src) {
	strcpy(dst, src);
}
