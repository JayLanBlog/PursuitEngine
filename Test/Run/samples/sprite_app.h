#pragma once
#include "../app_run.h"
#include "Engine/Ecore/sprite.h"

namespace pf::arun {
	using namespace pf;
	using namespace pf::graphics;

	class SpriteLauncher : public TLauncher {
	public:
		Sprite sprite;
		void spriteinit();

		virtual void intialize();
		virtual void render();
	};
}