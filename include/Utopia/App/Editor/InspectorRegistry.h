#pragma once

#include <UECS/UECS.hpp>

#include <functional>

#include <UVisitor/ncVisitor.h>
#include <_deps/crossguid/guid.hpp>

namespace UInspector {
	// Description: Makes a variable not show up in the inspector but be serialized.
	// Value      : void
	static constexpr char hide[] = "UInspector_hide";

	// Description: Add a header above some fields,
	// Value      : "..." / std::string_view
	static constexpr char header[] = "UInspector_header";

	// Description: change the display name of a field or a enumerator.
	// Value      : "..." / std::string_view
	static constexpr char name[] = "UInspector_name";

	// Description: Specify a tooltip for a field
	// Value      : "..." / std::string_view
	static constexpr char tooltip[] = "UInspector_tooltip";

	// Description: Make a float or int variable be restricted to a specific minimum value. (conflict with range)
	// Value      : float or int
	static constexpr char min_value[] = "UInspector_min_value";

	// Description: Make a float or int variable dragged by a specific step. (conflict with range)
	// Value      : float or int
	static constexpr char step[] = "UInspector_step";

	// Description: Make a float or int variable be restricted to a specific range.
	// Value      : std::pair<T, T> (T is float or int)
	static constexpr char range[] = "UInspector_range";
}

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

		void RegisterCmpt(TypeID, std::function<void(void*, InspectContext)> cmptInspectFunc);

		template<typename Func>
		void RegisterCmpt(Func&& func);

		void RegisterAsset(Type, std::function<void(void*, InspectContext)> assetInspectFunc);

		template<typename Func>
		void RegisterAsset(Func&& func);

		bool IsRegisteredCmpt(TypeID) const;
		bool IsRegisteredAsset(Type) const;

		void Inspect(const UECS::World*, UECS::CmptPtr);
		void Inspect(Type, void* asset);

	private:
		InspectorRegistry();
		~InspectorRegistry();
		struct Impl;
		Impl* pImpl;
	};
}

#include "details/InspectorRegistry.inl"
