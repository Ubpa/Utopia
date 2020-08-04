#include "LuaCmpt.h"

#include <UECS/World.h>

using namespace Ubpa::DustEngine;

void LuaCmpt::SetZero() {
	assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
	size_t size = UECS::RTDCmptTraits::Instance().Sizeof(ptr.Type());
	memset(ptr.Ptr(), 0, size);
}

void LuaCmpt::MemCpy(void* src) {
	assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
	size_t size = UECS::RTDCmptTraits::Instance().Sizeof(ptr.Type());
	memcpy(ptr.Ptr(), src, size);
}

void LuaCmpt::MemCpy(void* src, size_t offset, size_t size) {
	assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
	memcpy(Offset(offset), src, size);
}
