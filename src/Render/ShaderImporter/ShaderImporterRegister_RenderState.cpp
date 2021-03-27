#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_RenderState() {
	UDRefl::Mngr.RegisterType<RenderState>();
	UDRefl::Mngr.SimpleAddField<&RenderState::fillMode>("fillMode");
	UDRefl::Mngr.SimpleAddField<&RenderState::cullMode>("cullMode");
	UDRefl::Mngr.SimpleAddField<&RenderState::zTest>("zTest");
	UDRefl::Mngr.SimpleAddField<&RenderState::zWrite>("zWrite");
	UDRefl::Mngr.SimpleAddField<&RenderState::stencilState>("stencilState");
	UDRefl::Mngr.AddField<&RenderState::blendStates>("blendStates");
	UDRefl::Mngr.AddField<&RenderState::colorMask>("colorMask");
}
