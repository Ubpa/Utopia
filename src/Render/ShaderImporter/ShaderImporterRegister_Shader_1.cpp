#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

template<>
constexpr auto Ubpa::type_name<RootParameter>() noexcept {
	return TSTR("Ubpa::Utopia::RootParameter");
}

void Ubpa::Utopia::details::ShaderImporterRegister_Shader_1() {
	UDRefl::Mngr.AddField<&Shader::rootParameters>("rootParameters");
}
