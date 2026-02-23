#include "loading_screen_bind.h"
#include "scene_bind.h"
#include "texture_bind.h"
#include "application_bind.h"
#include "render_path_3d_bind.h"
#include <mutex>

using namespace Luaer::scene;
using namespace pf::scene;
using namespace pf::ecs;

namespace Luaer {
	Luna<LoadingScreen_BindLua>::FunctionType LoadingScreen_BindLua::methods[] = {
		lunamethod(RenderPath2D_BindLua, AddSprite),
		lunamethod(RenderPath2D_BindLua, AddFont),
		lunamethod(RenderPath2D_BindLua, RemoveSprite),
		lunamethod(RenderPath2D_BindLua, RemoveFont),
		lunamethod(RenderPath2D_BindLua, ClearSprites),
		lunamethod(RenderPath2D_BindLua, ClearFonts),
		lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, GetFontOrder),
		lunamethod(RenderPath2D_BindLua, AddLayer),
		lunamethod(RenderPath2D_BindLua, GetLayers),
		lunamethod(RenderPath2D_BindLua, SetLayerOrder),
		lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, SetFontOrder),

		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),

		lunamethod(LoadingScreen_BindLua, AddLoadModelTask),
		lunamethod(LoadingScreen_BindLua, AddRenderPathActivationTask),
		lunamethod(LoadingScreen_BindLua, IsFinished),
		lunamethod(LoadingScreen_BindLua, GetProgress),
		lunamethod(LoadingScreen_BindLua, SetBackgroundTexture),
		lunamethod(LoadingScreen_BindLua, GetBackgroundTexture),
		lunamethod(LoadingScreen_BindLua, SetBackgroundMode),
		lunamethod(LoadingScreen_BindLua, GetBackgroundMode),
		{ NULL, NULL }
	};
	Luna<LoadingScreen_BindLua>::PropertyType LoadingScreen_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int LoadingScreen_BindLua::AddLoadModelTask(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "AddLoadModelTask(Scene scene, string fileName, opt Matrix transform): loading screen is invalid!");
			return 0;
		}

		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 1);
			if (custom_scene)
			{
				// Overload 1: thread safe version
				if (argc > 1)
				{
					std::string fileName = Luaer::SGetString(L, 2);
					XMMATRIX transform = XMMatrixIdentity();
					if (argc > 2)
					{
						Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 3);
						if (matrix != nullptr)
						{
							transform = XMLoadFloat4x4(&matrix->data);
						}
						else
						{
							Luaer::SError(L, "AddLoadModelTask(Scene scene, string fileName, opt Matrix transform) argument is not a matrix!");
						}
					}
					Entity root = CreateEntity();
					loading->addLoadingFunction([=](pf::jobsystem::JobArgs args) {
						Scene scene;
						pf::scene::LoadModel2(scene, fileName, transform, root);

						// Note: we lock the  scene for merging from multiple loading screen tasks
						//	Because we don't have fine control over thread execution in lua, and this significantly
						//	simplifies loading into scene from loadingscreen
						std::scoped_lock lck(custom_scene->scene->locker);
						custom_scene->scene->Merge(scene);
						});
					Luaer::SSetLongLong(L, root);
					return 1;
				}
				else
				{
					Luaer::SError(L, "AddLoadModelTask(Scene scene, string fileName, opt Matrix transform) not enough arguments!");
					return 0;
				}
			}
			else
			{
				// Overload 2: global scene version
				std::string fileName = Luaer::SGetString(L, 1);
				XMMATRIX transform = XMMatrixIdentity();
				if (argc > 1)
				{
					Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 2);
					if (matrix != nullptr)
					{
						transform = XMLoadFloat4x4(&matrix->data);
					}
					else
					{
						Luaer::SError(L, "AddLoadModelTask(string fileName, opt Matrix transform) argument is not a matrix!");
					}
				}
				Entity root = CreateEntity();
				loading->addLoadingFunction([=](pf::jobsystem::JobArgs args) {
					Scene scene;
					pf::scene::LoadModel2(scene, fileName, transform, root);

					// Note: we lock the  scene for merging from multiple loading screen tasks
					//	Because we don't have fine control over thread execution in lua, and this significantly
					//	simplifies loading into scene from loadingscreen
					std::scoped_lock lck(GetGlobalScene()->locker);
					GetGlobalScene()->Merge(scene);
					});
				Luaer::SSetLongLong(L, root);
				return 1;
			}
		}
		else
		{
			Luaer::SError(L, "AddLoadModelTask(string fileName, opt Matrix transform) not enough arguments!");
		}
		return 0;
	}
	int LoadingScreen_BindLua::AddRenderPathActivationTask(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "AddRenderPathActivationTask(RenderPath path, opt float fadeSeconds = 0, opt int fadeR = 0,fadeG = 0,fadeB = 0, opt FadeType fadetype = FadeType.FadeToColor): loading screen is invalid!");
			return 0;
		}

		int argc = Luaer::SGetArgCount(L);
		if (argc < 2)
		{
			Luaer::SError(L, "AddRenderPathActivationTask(RenderPath path, Application app, opt float fadeSeconds = 0, opt int fadeR = 0,fadeG = 0,fadeB = 0, opt FadeType fadetype = FadeType.FadeToColor): not enough arguments!");
			return 0;
		}
		RenderPath* path = nullptr;

		RenderPath3D_BindLua* comp3D = Luna<RenderPath3D_BindLua>::lightcheck(L, 1);
		if (comp3D == nullptr)
		{
			RenderPath2D_BindLua* comp2D = Luna<RenderPath2D_BindLua>::lightcheck(L, 1);
			if (comp2D == nullptr)
			{
				LoadingScreen_BindLua* compLoading = Luna<LoadingScreen_BindLua>::lightcheck(L, 1);
				if (compLoading == nullptr)
				{
					RenderPath_BindLua* comp = Luna<RenderPath_BindLua>::lightcheck(L, 1);
					if (comp == nullptr)
					{
						Luaer::SError(L, "AddRenderPathActivationTask(RenderPath path, Application app, opt float fadeSeconds = 0, opt int fadeR = 0,fadeG = 0,fadeB = 0, opt FadeType fadetype = FadeType.FadeToColor): first argument is not a RenderPath!");
						return 0;
					}
					else
					{
						path = comp->component;
					}
				}
				else
				{
					path = compLoading->component;
				}
			}
			else
			{
				path = comp2D->component;
			}
		}
		else
		{
			path = comp3D->component;
		}

		Application_BindLua* app = Luna<Application_BindLua>::lightcheck(L, 2);
		if (app == nullptr)
		{
			Luaer::SError(L, "AddRenderPathActivationTask(RenderPath path, Application app, opt float fadeSeconds = 0, opt int fadeR = 0,fadeG = 0,fadeB = 0, opt FadeType fadetype = FadeType.FadeToColor): second argument is not an Application!");
			return 0;
		}

		float fadeSeconds = 0;
		pf::Color fadeColor = pf::Color::Black();
		pf::FadeManager::FadeType fadetype = pf::FadeManager::FadeType::FadeToColor;

		if (argc > 2)
		{
			fadeSeconds = Luaer::SGetFloat(L, 3);
			if (argc > 3)
			{
				fadeColor.setR((uint8_t)Luaer::SGetInt(L, 4));
				if (argc > 4)
				{
					fadeColor.setG((uint8_t)Luaer::SGetInt(L, 5));
					if (argc > 5)
					{
						fadeColor.setB((uint8_t)Luaer::SGetInt(L, 6));
						if (argc > 6)
						{
							fadetype = (pf::FadeManager::FadeType)Luaer::SGetInt(L, 7);
						}
					}
				}
			}
		}

		loading->addLoadingComponent(path, app->component, fadeSeconds, fadeColor, fadetype);
		return 0;
	}
	int LoadingScreen_BindLua::IsFinished(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading != nullptr)
		{
			Luaer::SSetBool(L, loading->isFinished());
			return 1;
		}
		Luaer::SError(L, "IsFinished(): loading screen is invalid!");
		return 0;
	}
	int LoadingScreen_BindLua::GetProgress(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading != nullptr)
		{
			Luaer::SSetInt(L, loading->getProgress());
			return 1;
		}
		Luaer::SError(L, "GetProgress(): loading screen is invalid!");
		return 0;
	}
	int LoadingScreen_BindLua::SetBackgroundTexture(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "SetBackgroundTexture(Texture tex): loading screen is not valid!");
			return 0;
		}
		int argc = Luaer::SGetArgCount(L);
		if (argc < 1)
		{
			Luaer::SError(L, "SetBackgroundTexture(Texture tex): not enough arguments!");
			return 0;
		}
		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
		if (tex == nullptr)
		{
			Luaer::SError(L, "SetBackgroundTexture(Texture tex): argument is not a Texture!");
			return 0;
		}
		loading->backgroundTexture = tex->resource;
		return 0;
	}
	int LoadingScreen_BindLua::GetBackgroundTexture(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "GetBackgroundTexture(): loading screen is not valid!");
			return 0;
		}
		Luna<Texture_BindLua>::push(L, loading->backgroundTexture);
		return 1;
	}
	int LoadingScreen_BindLua::SetBackgroundMode(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "SetBackgroundMode(int mode): loading screen is not valid!");
			return 0;
		}
		int argc = Luaer::SGetArgCount(L);
		if (argc < 1)
		{
			Luaer::SError(L, "SetBackgroundMode(int mode): not enough arguments!");
			return 0;
		}
		loading->background_mode = (LoadingScreen::BackgroundMode)Luaer::SGetInt(L, 1);
		return 0;
	}
	int LoadingScreen_BindLua::GetBackgroundMode(lua_State* L)
	{
		LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
		if (loading == nullptr)
		{
			Luaer::SError(L, "GetBackgroundMode(): loading screen is not valid!");
			return 0;
		}
		Luaer::SSetInt(L, (int)loading->background_mode);
		return 1;
	}

	void LoadingScreen_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<LoadingScreen_BindLua>::Register(Luaer::GetLuaState());

			Luaer::RunText(R"(
BackgroundMode = {
	Fill = 0,
	Fit = 1,
	Stretch = 2
}
)");
		}
	}
}