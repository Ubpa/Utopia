#include <Utopia/Render/Register_Render.h>

#include <Utopia/Render/Components/Components.h>
#include <Utopia/Render/Systems/Systems.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/RenderTargetTexture2D.h>

#include <Utopia/Render/DX12/StdPipeline.h>
#include <Utopia/Render/DX12/StdDXRPipeline.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::UDRefl_Register_Render() {
	// Texture
	UDRefl::Mngr.RegisterType<GPURsrc>();
	UDRefl::Mngr.RegisterType<Texture2D>();
	UDRefl::Mngr.AddBases<Texture2D, GPURsrc>();
	UDRefl::Mngr.RegisterType<TextureCube>();
	UDRefl::Mngr.AddBases<TextureCube, GPURsrc>();
	UDRefl::Mngr.RegisterType<RenderTargetTexture2D>();
	UDRefl::Mngr.AddBases<RenderTargetTexture2D, Texture2D>();

	// Camera
	// TODO: attrs

	Mngr.RegisterType<Camera::PipelineMode>();
	Mngr.AddField<Camera::PipelineMode::Custom>("Custom");
	Mngr.AddField<Camera::PipelineMode::Std>("Std");
	Mngr.AddField<Camera::PipelineMode::StdDXR>("StdDXR");

	Mngr.RegisterType<Camera>();
	Mngr.SimpleAddField<&Camera::aspect>("aspect");
	Mngr.SimpleAddField<&Camera::fov>("fov");
	Mngr.SimpleAddField<&Camera::clippingPlaneMin>("clippingPlaneMin");
	Mngr.SimpleAddField<&Camera::clippingPlaneMax>("clippingPlaneMax");
	Mngr.SimpleAddField<&Camera::prjectionMatrix>("prjectionMatrix");
	Mngr.AddField<&Camera::renderTarget>("renderTarget");
	Mngr.SimpleAddField<&Camera::order>("order");
	Mngr.SimpleAddField<&Camera::pipeline_mode>("pipeline_mode");
	Mngr.AddField<&Camera::custom_pipeline>("custom_pipeline");

	// Light
	Mngr.RegisterType<Light::Mode>();
	Mngr.SimpleAddField<Light::Mode::Directional>("Directional");
	Mngr.SimpleAddField<Light::Mode::Point>("Point");
	Mngr.SimpleAddField<Light::Mode::Spot>("Spot");
	Mngr.SimpleAddField<Light::Mode::Rect>("Rect");
	Mngr.SimpleAddField<Light::Mode::Disk>("Disk");

	Mngr.RegisterType<Light>();
	Mngr.SimpleAddField<&Light::mode>("mode");
	Mngr.SimpleAddField<&Light::color>("color");
	Mngr.SimpleAddField<&Light::intensity>("intensity");
	Mngr.SimpleAddField<&Light::range>("range");
	Mngr.SimpleAddField<&Light::width>("width");
	Mngr.SimpleAddField<&Light::height>("height");
	Mngr.SimpleAddField<&Light::innerSpotAngle>("innerSpotAngle");
	Mngr.SimpleAddField<&Light::outerSpotAngle>("outerSpotAngle");

	// MeshFilter
	Mngr.RegisterType<MeshFilter>();
	Mngr.AddField<&MeshFilter::mesh>("mesh");

	// MeshRenderer
	Mngr.RegisterType<MeshRenderer>();
	Mngr.AddField<&MeshRenderer::materials>("materials");

	// Skybox
	Mngr.RegisterType<Skybox>();
	Mngr.AddField<&Skybox::material>("materials");

	// Pipeline
	Mngr.RegisterType<IPipeline>();
	Mngr.RegisterType<StdPipeline>();
	Mngr.AddBases<StdPipeline, IPipeline>();
	Mngr.RegisterType<StdDXRPipeline>();
	Mngr.AddBases<StdDXRPipeline, IPipeline>();
}

void Ubpa::Utopia::World_Register_Render(UECS::World* world) {
	world->entityMngr.cmptTraits.Register<
		Camera,
		Light,
		MeshFilter,
		MeshRenderer,
		Skybox
	>();

	world->systemMngr.systemTraits.Register<
		CameraSystem
	>();
}
