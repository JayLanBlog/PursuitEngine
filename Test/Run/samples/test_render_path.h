#pragma once
#include "Engine/Ecore/render_path3d.h"
#include "Engine/Runtime/runner.h"
class TestsRenderer : public pf::RenderPath3D
{
	pf::gui::Label label;
	pf::gui::ComboBox testSelector;
	pf::ecs::Entity ik_entity = pf::ecs::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
	void ContainerTest();
};

class Tests : public pf::Application
{
	TestsRenderer renderer;
public:
	void Initialize() override;
};

