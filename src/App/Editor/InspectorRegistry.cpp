#include <Utopia/App/Editor/InspectorRegistry.h>

#include <UECS/CmptType.h>

#include <unordered_map>
#include <functional>

using namespace Ubpa::Utopia;

struct InspectorRegistry::Impl {
	Visitor<void(void*, InspectContext)> inspector;
};

InspectorRegistry::InspectorRegistry() : pImpl{ new Impl } {}
InspectorRegistry::~InspectorRegistry() { delete pImpl; }

void InspectorRegistry::RegisterCmpt(UECS::CmptType type, std::function<void(void*, InspectContext)> cmptInspectFunc) {
	pImpl->inspector.Register(type.HashCode(), std::move(cmptInspectFunc));
}

void InspectorRegistry::RegisterAsset(const std::type_info& typeinfo, std::function<void(void*, InspectContext)> assetInspectFunc) {
	pImpl->inspector.Register(typeinfo.hash_code(), std::move(assetInspectFunc));
}

bool InspectorRegistry::IsRegisteredCmpt(UECS::CmptType type) const {
	return pImpl->inspector.IsRegistered(type.HashCode());
}

bool InspectorRegistry::IsRegisteredAsset(const std::type_info& typeinfo) const {
	return pImpl->inspector.IsRegistered(typeinfo.hash_code());
}

void InspectorRegistry::Inspect(const UECS::World* world, UECS::CmptPtr cmpt) {
	pImpl->inspector.Visit(cmpt.Type().HashCode(), cmpt.Ptr(), InspectContext{ world, pImpl->inspector });
}

void InspectorRegistry::Inspect(const std::type_info& typeinfo, void* asset) {
	pImpl->inspector.Visit(typeinfo.hash_code(), asset, InspectContext{ nullptr, pImpl->inspector });
}
