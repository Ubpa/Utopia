#include <Utopia/Core/AssetMngr.h>

#include <iostream>

using namespace Ubpa::Utopia;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	AssetMngr::Instance().SetRootPath(LR"(..\src\test\x\00_AssetMngr\assets)");

	AssetMngr::Instance().Clear();
}
