#pragma once

namespace Ubpa::UECS {
    class World;
}

namespace Ubpa::Utopia {
    void UDRefl_Register_Core();
    void World_Register_Core(UECS::World* w);
}
