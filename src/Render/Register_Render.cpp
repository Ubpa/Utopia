#include <Utopia/Render/Register_Render.h>

#include <Utopia/Render/Components/Components.h>
#include <Utopia/Render/Systems/Systems.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::UDRefl_Register_Render() {
	// Camera
	// TODO: attrs
	Mngr.RegisterType<Camera>();
	Mngr.SimpleAddField<&Camera::aspect>("aspect");
	Mngr.SimpleAddField<&Camera::fov>("fov");
	Mngr.SimpleAddField<&Camera::clippingPlaneMin>("clippingPlaneMin");
	Mngr.SimpleAddField<&Camera::clippingPlaneMax>("clippingPlaneMax");
	Mngr.SimpleAddField<&Camera::prjectionMatrix>("prjectionMatrix");

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
