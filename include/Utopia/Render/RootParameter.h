#pragma once

#include <UTemplate/Name.hpp>

#include <variant>
#include <vector>

namespace Ubpa::Utopia {
	enum class RootDescriptorType {
		SRV,
		UAV,
		CBV,
	};

	struct DescriptorRange {
		RootDescriptorType RangeType;
		unsigned int NumDescriptors;
		unsigned int BaseShaderRegister;
		unsigned int RegisterSpace;

		void Init(
			RootDescriptorType RangeType,
			unsigned int NumDescriptors,
			unsigned int BaseShaderRegister,
			unsigned int RegisterSpace = 0
		) {
			this->RangeType = RangeType;
			this->NumDescriptors = NumDescriptors;
			this->BaseShaderRegister = BaseShaderRegister;
			this->RegisterSpace = RegisterSpace;
		}
	};

	using RootDescriptorTable = std::vector<DescriptorRange>;

	struct RootConstants {
		unsigned int ShaderRegister;
		unsigned int RegisterSpace;
		unsigned int Num32BitValues;
	};

	struct RootDescriptor {
		RootDescriptorType DescriptorType{}; // ignore sampler
		unsigned int ShaderRegister{ 0 };
		unsigned int RegisterSpace{ 0 };

		void Init(
			RootDescriptorType DescriptorType,
			unsigned int ShaderRegister,
			unsigned int RegisterSpace = 0
		) {
			this->DescriptorType = DescriptorType;
			this->ShaderRegister = ShaderRegister;
			this->RegisterSpace = RegisterSpace;
		}
	};

	using RootParameter = std::variant<RootDescriptorTable, RootConstants, RootDescriptor>;
}

template<>
constexpr auto Ubpa::type_name<Ubpa::Utopia::RootParameter>() noexcept {
	return TSTR("Ubpa::Utopia::RootParameter");
}