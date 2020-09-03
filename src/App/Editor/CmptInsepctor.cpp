#include "CmptInsepctor.h"

#include <UECS/CmptType.h>

#include <unordered_map>
#include <functional>

using namespace Ubpa::DustEngine;

struct CmptInspector::Impl {
	Visitor<void(void*, InspectContext)> inspector;
};

CmptInspector::CmptInspector() : pImpl{ new Impl } {}
CmptInspector::~CmptInspector() { delete pImpl; }

void CmptInspector::RegisterCmpt(UECS::CmptType type, std::function<void(void*, InspectContext)> cmptInspectFunc) {
	pImpl->inspector.Register(type.HashCode(), std::move(cmptInspectFunc));
}

bool CmptInspector::IsCmptRegistered(UECS::CmptType type) const {
	return pImpl->inspector.IsRegistered(type.HashCode());
}

void CmptInspector::Inspect(UECS::CmptPtr cmpt) {
	pImpl->inspector.Visit(cmpt.Type().HashCode(), cmpt.Ptr(), InspectContext{ pImpl->inspector });
}
