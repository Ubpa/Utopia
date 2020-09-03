#pragma once

#include <UECS/CmptPtr.h>

#include <functional>

#include <UDP/Visitor/ncVisitor.h>

namespace Ubpa::DustEngine {
	class CmptInspector {
	public:
		static CmptInspector& Instance() noexcept {
			static CmptInspector instance;
			return instance;
		}

		struct InspectContext {
			const Visitor<void(void*, InspectContext)>& inspector;
		};

		void RegisterCmpt(UECS::CmptType type, std::function<void(void*, InspectContext)> cmptInspectFunc);

		template<typename Func>
		void RegisterCmpt(Func&& func);

		template<typename... Cmpts>
		void RegisterCmpts();

		bool IsCmptRegistered(UECS::CmptType type) const;

		void Inspect(UECS::CmptPtr cmpt);

	private:
		CmptInspector();
		~CmptInspector();
		struct Impl;
		Impl* pImpl;
	};
}

#include "details/CmptInsepctor.inl"
