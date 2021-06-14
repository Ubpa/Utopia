#pragma once

#include <UGM/transform.hpp>
#include "../RenderTargetTexture2D.h"
#include "../../Core/SharedVar.h"

namespace Ubpa::Utopia {
	struct Camera {
		float aspect{ 4.f / 3.f };
		[[interval(std::pair{1.f, 179.f})]]
		float fov{ 60.f };
		[[min(0.1f)]]
		float clippingPlaneMin{ 0.3f };
		[[min(0.1f)]]
		float clippingPlaneMax{ 1000.f };

		transformf prjectionMatrix;

		// if renderTarget is nullptr:
		//   the camera will render the scene to the default render target
		// else:
		//   the camera will render to it
		SharedVar<RenderTargetTexture2D> renderTarget;
	};
}
