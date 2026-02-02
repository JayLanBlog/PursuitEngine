#pragma once
#include "Run/app_run.h"
#include "Engine/Component/pgui.h"

namespace pf::arun {
	using namespace pf;
	using namespace pf::graphics;
	using namespace pf::gui;

	class GUISample : public TLauncher {
	public:
		GUI gui;
		Label label;
		Button audioTest;
		Canvas canvas_gui;
		virtual void intialize();
		virtual void render();
	};

}