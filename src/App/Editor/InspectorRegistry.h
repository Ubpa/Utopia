#pragma once

#include <UECS/World.h>

#include <functional>

#include <UDP/Visitor/ncVisitor.h>
#include <_deps/crossguid/guid.hpp>

namespace Ubpa::Utopia {
	class InspectorRegistry {
	public:
		static InspectorRegistry& Instance() noexcept {
			static InspectorRegistry instance;
			return instance;
		}

		struct InspectContext {
			const UECS::World* world;
			const Visitor<void(void*, InspectContext)>& inspector;
		};

		void RegisterCmpt(UECS::CmptType, std::function<void(void*, InspectContext)> cmptInspectFunc);

		template<typename Func>
		void RegisterCmpt(Func&& func);

		template<typename... Cmpts>
		void RegisterCmpts();

		void RegisterAsset(const std::type_info&, std::function<void(void*, InspectContext)> assetInspectFunc);

		template<typename Func>
		void RegisterAsset(Func&& func);

		template<typename... Assets>
		void RegisterAssets();

		bool IsRegisteredCmpt(UECS::CmptType) const;
		bool IsRegisteredAsset(const std::type_info&) const;

		void Inspect(const UECS::World*, UECS::CmptPtr);
		void Inspect(const std::type_info&, void* asset);

	private:
		InspectorRegistry();
		~InspectorRegistry();
		struct Impl;
		Impl* pImpl;
	};
}

#include "details/InspectorRegistry.inl"
