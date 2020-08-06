#include "InitUGraphviz.h"

#include "detail/UGraphviz_AutoRefl/UGraphviz_AutoRefl.h"

void Ubpa::DustEngine::detail::InitUGraphviz(lua_State* L) {
	ULuaPP::Register<UGraphviz::Graph>(L);
	ULuaPP::Register<UGraphviz::Registry>(L);
	ULuaPP::Register<UGraphviz::Subgraph>(L);
}
