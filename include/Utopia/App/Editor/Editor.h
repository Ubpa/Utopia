#include "../DX12App/DX12App.h"

namespace Ubpa::UECS { class World; }

namespace Ubpa::Utopia {
	class Editor : public Ubpa::Utopia::DX12App {
	public:
		Editor(HINSTANCE hInstance);
		~Editor();

		virtual bool Init() override;

		UECS::World* GetGameWorld();
		UECS::World* GetSceneWorld();
		UECS::World* GetEditorWorld();

		UECS::World* GetCurrentGameWorld();

	private:
		virtual void Update() override;
		virtual void Draw() override;
		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

		struct Impl;
		friend struct Impl;
		Impl* pImpl;
	};
}
