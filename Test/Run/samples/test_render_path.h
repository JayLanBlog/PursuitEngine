#pragma once
#include "Engine/Ecore/render_path3d.h"
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