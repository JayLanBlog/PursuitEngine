#include "test_render_path.h"
#include <Module/Util/input.h>

using namespace pf::ecs;
using namespace pf::scene;

#define CONTENT_DIR "Res/"

enum TEST_TYPE
{
	HELLOWORLD,
	MODEL,
	EMITTEDPARTICLE1,
	EMITTEDPARTICLE2,
	HAIRPARTICLE,
	LUASCRIPT,
	WATERTEST,
	SHADOWSTEST,
	PHYSICSTEST,
	CLOTHPHYSICSTEST,
	JOBSYSTEMTEST,
	FONTTEST,
	VOLUMETRICTEST,
	SPRITETEST,
	LIGHTMAPBAKETEST,
	NETWORKTEST,
	CONTROLLERTEST,
	INVERSEKINEMATICSTEST,
	INSTANCESTEST,
	CONTAINERPERF,
};

pf::SpriteFont
b_a, b_b, b_x, b_y, // Action Buttons: A, B, X, Y 
b_du, b_dd, b_dl, b_dr, // Directional Buttons: Up, Down, Left, Right
b_sta, b_bck, // Menu Buttons: Start, Back 
b_lb, b_rb, // Shoulder Buttons: LB, RB
b_ls, b_rs, // Stick Press Buttons: Left Stick, Right Stick
t_l, t_r, // Trigger Inputs: LT, RT
s_l, s_r, // Stick Axes: Left Stick, Right Stick
i_led; // Controller LED status


void Tests::Initialize() {
	pf::Application::Initialize();
	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	infoDisplay.heap_allocation_counter = true;
	renderer.init(canvas);
	renderer.Load();
	ActivatePath(&renderer);
}

void TestsRenderer::Load()  {

	setSSREnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);
	pf::renderer::SetOcclusionCullingEnabled(false);
	pf::gui::GUI& gui = GetGUI();

	label.Create("Label1");
	label.SetText("Wicked Engine Test Framework");
	label.font.params.h_align = pf::font::WIFALIGN_CENTER;
	label.SetSize(XMFLOAT2(240, 20));
	gui.AddWidget(&label);


	static pf::audio::Sound sound;
	static pf::audio::SoundInstance soundinstance;
	static pf::gui::Button audioTest;
	static pf::gui::Slider volume;
	static pf::gui::Slider direction;


	audioTest.Create("AudioTest");
	audioTest.SetText("Play Test Audio");
	audioTest.SetSize(XMFLOAT2(200, 20));
	audioTest.SetPos(XMFLOAT2(80, 110));
	audioTest.OnClick([&](pf::gui::EventArgs args) {
		static bool playing = false;

		if (!sound.IsValid())
		{
			//TO DO : pf::audio::CreateSound(CONTENT_DIR "models/water.wav", &sound);
			//pf::audio::CreateSoundInstance(&sound, &soundinstance);
			//pf::audio::SetVolume(volume.GetValue() / 100.0f, &soundinstance);
		}

		if (playing)
		{
			//pf::audio::Stop(&soundinstance);
			audioTest.SetText("Play Test Audio");
		}
		else
		{
			//pf::audio::Play(&soundinstance);
			audioTest.SetText("Stop Test Audio");
		}

		playing = !playing;
		});

	gui.AddWidget(&audioTest);

	volume.Create(0, 100, 50, 100, "Volume");
	volume.SetText("Volume: ");
	volume.SetSize(XMFLOAT2(200, 20));
	volume.SetPos(XMFLOAT2(80, 140));
	volume.OnSlide([](pf::gui::EventArgs args) {
	//	pf::audio::SetVolume(args.fValue / 100.0f, &soundinstance);
		});

	gui.AddWidget(&volume);

	direction.Create(-1, 1, 0, 10000, "Direction");
	direction.SetText("Direction: ");
	direction.SetSize(XMFLOAT2(200, 20));
	direction.SetPos(XMFLOAT2(80, 170));
	direction.OnSlide([](pf::gui::EventArgs args) {
		pf::audio::SoundInstance3D instance3D;
		instance3D.emitterPos = XMFLOAT3(args.fValue, 0, 0);
		instance3D.listenerPos = XMFLOAT3(0, 0, -0.1f);
		pf::audio::Update3D(&soundinstance, instance3D);
		});
	gui.AddWidget(&direction);

	testSelector.Create("TestSelector");
	testSelector.SetText("Demo: ");
	testSelector.SetSize(XMFLOAT2(200, 20));
	testSelector.SetPos(XMFLOAT2(80, 220));
	testSelector.AddItem("HelloWorld", HELLOWORLD);
	testSelector.AddItem("Model", MODEL);
	testSelector.AddItem("EmittedParticle 1", EMITTEDPARTICLE1);
	testSelector.AddItem("EmittedParticle 2", EMITTEDPARTICLE2);
	testSelector.AddItem("HairParticle", HAIRPARTICLE);
	testSelector.AddItem("Lua Script", LUASCRIPT);
	testSelector.AddItem("Water Test", WATERTEST);
	testSelector.AddItem("Shadows Test", SHADOWSTEST);
	testSelector.AddItem("Physics Test", PHYSICSTEST);
	testSelector.AddItem("Cloth Physics Test", CLOTHPHYSICSTEST);
	testSelector.AddItem("Job System Test", JOBSYSTEMTEST);
	testSelector.AddItem("Font Test", FONTTEST);
	testSelector.AddItem("Volumetric Test", VOLUMETRICTEST);
	testSelector.AddItem("Sprite Test", SPRITETEST);
	testSelector.AddItem("Lightmap Bake Test", LIGHTMAPBAKETEST);
	testSelector.AddItem("Network Test", NETWORKTEST);
	testSelector.AddItem("Controller Test", CONTROLLERTEST);
	testSelector.AddItem("Inverse Kinematics", INVERSEKINEMATICSTEST);
	testSelector.AddItem("65k Instances", INSTANCESTEST);
	testSelector.AddItem("Container perf", CONTAINERPERF);
	testSelector.SetMaxVisibleItemCount(10);

	testSelector.OnSelect([=](pf::gui::EventArgs args) {
		// Reset all state that tests might have modified:
		pf::eventhandler::SetVSync(true);
		pf::profiler::SetEnabled(false);
		pf::renderer::SetToDrawGridHelper(false);
		pf::renderer::SetTemporalAAEnabled(false);
		pf::renderer::ClearWorld(pf::scene::GetScene());
		pf::scene::GetScene().weather = WeatherComponent();
		this->ClearSprites();
		this->ClearFonts();
	/*	if (pf::lua::GetLuaState() != nullptr) {
			pf::lua::KillProcesses();
		}*/

		// Reset camera position:
		TransformComponent transform;
		transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
		transform.UpdateTransform();
		pf::scene::GetCamera().TransformCamera(transform);

		float screenW = GetLogicalWidth();
		float screenH = GetLogicalHeight();


		// Based on combobox selection, start the appropriate test:
		switch (args.userdata) {	
		case HELLOWORLD:
		{
			// This will spawn a sprite with two textures. The first texture is a color texture and it will be animated.
			//	The second texture is a static image of "hello world" written on it
			//	Then add some animations to the sprite to get a nice wobbly and color changing effect.
			//	You can learn more in the Sprite test in RunSpriteTest() function
			static pf::Sprite sprite;
			sprite = pf::Sprite(CONTENT_DIR "images/movingtex.png", CONTENT_DIR "images/HelloWorld.png");
			sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite.params.siz = XMFLOAT2(200, 100);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.rot = XM_PI / 4.0f;
			sprite.anim.wobbleAnim.amount = XMFLOAT2(0.16f, 0.16f);
			sprite.anim.movingTexAnim.speedX = 0;
			sprite.anim.movingTexAnim.speedY = 3;
			AddSprite(&sprite);
		}
			break;
		case MODEL:
			pf::renderer::SetTemporalAAEnabled(true);
			pf::scene::LoadModel(CONTENT_DIR "models/teapot.wiscene");
			break;
		}

		});
	testSelector.SetSelected(0);
	gui.AddWidget(&testSelector);
	

	// This will spawn a sprite with two textures. The first texture is a color texture and it will be animated.
	//	The second texture is a static image of "hello world" written on it
	//	Then add some animations to the sprite to get a nice wobbly and color changing effect.
	//	You can learn more in the Sprite test in RunSpriteTest() function
	//static pf::Sprite sprite;
	//sprite = pf::Sprite("images/movingtex.png", "images/HelloWorld.png");
	//sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
	//sprite.params.siz = XMFLOAT2(200, 100);
	//sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
	//sprite.anim.rot = XM_PI / 4.0f;
	//sprite.anim.wobbleAnim.amount = XMFLOAT2(0.16f, 0.16f);
	//sprite.anim.movingTexAnim.speedX = 0;
	//sprite.anim.movingTexAnim.speedY = 3;
	//AddSprite(&sprite);

	// Set a theme globally:
	{
		pf::Color theme_color_idle = pf::Color(100, 130, 150, 150);
		pf::Color theme_color_focus = pf::Color(100, 180, 200, 200);
		pf::Color dark_point = pf::Color(0, 0, 20, 200); // darker elements will lerp towards this
		pf::gui::Theme theme;
		theme.image.background = true;
		theme.image.blendFlag = pf::enums::BLENDMODE_OPAQUE;
		theme.font.color = pf::Color(160, 240, 250, 255);
		theme.shadow_color = pf::Color(100, 180, 200, 100);
		
		theme.tooltipImage = theme.image;
		theme.tooltipImage.color = theme_color_idle;
		theme.tooltipFont = theme.font;
		theme.tooltip_shadow_color = theme.shadow_color;



		pf::Color theme_color_active = pf::Color::White();
		pf::Color theme_color_deactivating = pf::Color::lerp(theme_color_focus, pf::Color::White(), 0.5f);

		gui.SetTheme(theme); // set basic params to all states


		// customize colors for specific states:
		gui.SetColor(theme_color_idle, pf::gui::IDLE);
		gui.SetColor(theme_color_focus, pf::gui::FOCUS);
		gui.SetColor(theme_color_active, pf::gui::ACTIVE);
		gui.SetColor(theme_color_deactivating, pf::gui::DEACTIVATING);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.7f), pf::gui::WIDGET_ID_WINDOW_BASE);


		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.75f), pf::gui::WIDGET_ID_SLIDER_BASE_IDLE);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.8f), pf::gui::WIDGET_ID_SLIDER_BASE_FOCUS);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.85f), pf::gui::WIDGET_ID_SLIDER_BASE_ACTIVE);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.8f), pf::gui::WIDGET_ID_SLIDER_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, pf::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
		gui.SetColor(theme_color_focus, pf::gui::WIDGET_ID_SLIDER_KNOB_FOCUS);
		gui.SetColor(theme_color_active, pf::gui::WIDGET_ID_SLIDER_KNOB_ACTIVE);
		gui.SetColor(theme_color_deactivating, pf::gui::WIDGET_ID_SLIDER_KNOB_DEACTIVATING);


		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.75f), pf::gui::WIDGET_ID_SCROLLBAR_BASE_IDLE);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.8f), pf::gui::WIDGET_ID_SCROLLBAR_BASE_FOCUS);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.85f), pf::gui::WIDGET_ID_SCROLLBAR_BASE_ACTIVE);
		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.8f), pf::gui::WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, pf::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		gui.SetColor(theme_color_focus, pf::gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
		gui.SetColor(theme_color_active, pf::gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);

		gui.SetColor(pf::Color::lerp(theme_color_idle, dark_point, 0.8f), pf::gui::WIDGET_ID_COMBO_DROPDOWN);

	}
	RenderPath3D::Load();

}
void TestsRenderer::Update(float dt)  {
	int selected = testSelector.GetSelected();
	uint64_t userdata = testSelector.GetItemUserData(selected);


	switch (userdata)
	{
	case MODEL:
	{
		Scene& scene = pf::scene::GetScene();
		// teapot_material Base Base_mesh Top Top_mesh editorLight
		pf::ecs::Entity e_teapot_base = scene.Entity_FindByName("Base");
		pf::ecs::Entity e_teapot_top = scene.Entity_FindByName("Top");
		assert(e_teapot_base != pf::ecs::INVALID_ENTITY);
		assert(e_teapot_top != pf::ecs::INVALID_ENTITY);
		TransformComponent* transform_base = scene.transforms.GetComponent(e_teapot_base);
		TransformComponent* transform_top = scene.transforms.GetComponent(e_teapot_top);
		assert(transform_base != nullptr);
		assert(transform_top != nullptr);
		float rotation = dt;
		if (pf::input::Down(pf::input::KEYBOARD_BUTTON_LEFT))
		{
			transform_base->Rotate(XMVectorSet(0, rotation, 0, 1));
			transform_top->Rotate(XMVectorSet(0, rotation, 0, 1));
		}
		else if (pf::input::Down(pf::input::KEYBOARD_BUTTON_RIGHT))
		{
			transform_base->Rotate(XMVectorSet(0, -rotation, 0, 1));
			transform_top->Rotate(XMVectorSet(0, -rotation, 0, 1));
		}
	}
	break;
	case CONTROLLERTEST:
	{
		// Handle displays of all buttons here, the text will turn to white when the associated button are pressed
		b_a.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_2)) ? pf::Color::White() : pf::Color::Green();
		b_b.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_3)) ? pf::Color::White() : pf::Color::Red();
		b_x.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_1)) ? pf::Color::White() : pf::Color::Cyan();
		b_y.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_4)) ? pf::Color::White() : pf::Color::Yellow();
		b_sta.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_10)) ? pf::Color::White() : pf::Color::Gray();
		b_bck.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_9)) ? pf::Color::White() : pf::Color::Gray();
		b_ls.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_7)) ? pf::Color::White() : pf::Color::Gray();
		b_rs.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_8)) ? pf::Color::White() : pf::Color::Gray();
		b_du.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_UP)) ? pf::Color::White() : pf::Color::Gray();
		b_dd.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_DOWN)) ? pf::Color::White() : pf::Color::Gray();
		b_dl.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_LEFT)) ? pf::Color::White() : pf::Color::Gray();
		b_dr.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_RIGHT)) ? pf::Color::White() : pf::Color::Gray();
		b_lb.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_5)) ? pf::Color::White() : pf::Color::Gray();
		b_rb.params.color = (pf::input::Down(pf::input::GAMEPAD_BUTTON_6)) ? pf::Color::White() : pf::Color::Gray();

		// Handle displays of triggers and axes, will update the text according to the provided data of the controller
		static std::wstring text_head_axes[] = { L"L Trigger (PS: L2): ", L"R Trigger (PS: R2): ", L"L Stick: ", L"R Stick: " };
		t_l.text = text_head_axes[0] + std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_TRIGGER_L).x);
		t_r.text = text_head_axes[1] + std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_TRIGGER_R).x);
		s_l.text = text_head_axes[2] +
			std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_THUMBSTICK_L).x) + L", " +
			std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_THUMBSTICK_L).y);
		s_r.text = text_head_axes[3] +
			std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_THUMBSTICK_R).x) + L", " +
			std::to_wstring(pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_THUMBSTICK_R).y);

		// Handling controller's vibrations and LED color
		static float vibration_multipllier = 0.f;
		static int color_index_set = 0;
		static uint32_t color_sets[] = { pf::Color::Black(), pf::Color::Cyan(), pf::Color::Red(), pf::Color::Green(), pf::Color::Yellow() };
		static std::wstring color_sets_desc[] = { L"Controller LED: None", L"Controller LED: Cyan", L"Controller LED: Red", L"Controller LED: Green", L"Controller LED: Yellow" };

		if (pf::input::Down(pf::input::GAMEPAD_BUTTON_2)) vibration_multipllier = 1.f;
		else vibration_multipllier = 0.f;
		if (pf::input::Press(pf::input::GAMEPAD_BUTTON_3)) {
			color_index_set++;
			if (color_index_set > 4) color_index_set = 0;
		}

		pf::input::ControllerFeedback feedback;
		feedback.led_color.rgba = color_sets[color_index_set];
		feedback.vibration_left = pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_TRIGGER_L).x * vibration_multipllier;
		feedback.vibration_right = pf::input::GetAnalog(pf::input::GAMEPAD_ANALOG_TRIGGER_R).x * vibration_multipllier;
		pf::input::SetControllerFeedback(feedback, 0);

		i_led.text = color_sets_desc[color_index_set];
		i_led.params.color = color_sets[color_index_set];
	}
	break;
	case INVERSEKINEMATICSTEST:
	{
		if (ik_entity != INVALID_ENTITY)
		{
			// Inverse kinematics test:
			Scene& scene = pf::scene::GetScene();
			InverseKinematicsComponent& ik = *scene.inverse_kinematics.GetComponent(ik_entity);
			TransformComponent& target = *scene.transforms.GetComponent(ik.target);

			// place ik target on a plane intersected by mouse ray:
			pf::primitive::Ray ray = pf::renderer::GetPickRay((long)pf::input::GetPointer().x, (long)pf::input::GetPointer().y, *this);
			XMVECTOR plane = XMVectorSet(0, 0, 1, 0.2f);
			XMVECTOR I = XMPlaneIntersectLine(plane, XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction) * 10000);
			target.ClearTransform();
			XMFLOAT3 _I;
			XMStoreFloat3(&_I, I);
			target.Translate(_I);
			target.UpdateTransform();

			// draw debug ik target position:
			pf::renderer::RenderablePoint pp;
			pp.position = target.GetPosition();
			pp.color = XMFLOAT4(0, 1, 1, 1);
			pp.size = 0.2f;
			pf::renderer::DrawPoint(pp);

			pp.position = scene.transforms.GetComponent(ik_entity)->GetPosition();
			pp.color = XMFLOAT4(1, 0, 0, 1);
			pp.size = 0.1f;
			pf::renderer::DrawPoint(pp);
		}
	}
	break;

	case INSTANCESTEST:
	{
		static pf::Timer timer;
		float sec = (float)timer.elapsed_seconds();
		pf::jobsystem::context ctx;
		pf::jobsystem::Dispatch(ctx, (uint32_t)scene->transforms.GetCount(), 1024, [&](pf::jobsystem::JobArgs args) {
			TransformComponent& transform = scene->transforms[args.jobIndex];
			XMStoreFloat4x4(
				&transform.world,
				transform.GetLocalMatrix() * XMMatrixTranslation(0, std::sin(sec + 20 * (float)args.jobIndex / (float)scene->transforms.GetCount()) * 0.1f, 0)
			);
			});
		scene->materials[0].SetEmissiveColor(XMFLOAT4(1, 1, 1, 1));
		pf::jobsystem::Dispatch(ctx, (uint32_t)scene->objects.GetCount(), 1024, [&](pf::jobsystem::JobArgs args) {
			ObjectComponent& object = scene->objects[args.jobIndex];
			float f = std::pow(std::sin(-sec * 2 + 4 * (float)args.jobIndex / (float)scene->objects.GetCount()) * 0.5f + 0.5f, 32.0f);
			object.emissiveColor = XMFLOAT4(0, 0.25f, 1, f * 3);
			});
		pf::jobsystem::Wait(ctx);
	}
	break;

	}

	RenderPath3D::Update(dt);
}
void TestsRenderer::ResizeLayout()  {
	RenderPath3D::ResizeLayout();
	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	label.SetPos(XMFLOAT2(screenW / 2.f - label.scale.x / 2.f, screenH * 0.95f));
}

//
void TestsRenderer::RunJobSystemTest() {

}
void TestsRenderer::RunFontTest() {

}
void TestsRenderer::RunSpriteTest() {

}
void TestsRenderer::RunNetworkTest() {

}

void TestsRenderer::ContainerTest() {

}

