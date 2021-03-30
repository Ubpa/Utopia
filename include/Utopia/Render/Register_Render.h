#pragma once
namespace Ubpa::UECS {
    class World;
}

namespace Ubpa::Utopia {
    void UDRefl_Register_Render();
    void World_Register_Render(UECS::World* world);
}
