#pragma once
#include "Engine/Ecore/render_path3d.h"
#include "Engine/Runtime/runner.h"


namespace pf {
	class Luancher {
	public:

		Luancher(pf::Application& app): mApp(app) {
		}
		void launch();
		bool quit = false;
		pf::Application& mApp;

	private:
		int loop();
	};
}