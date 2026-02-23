#include "render_path_3d_bind.h"
#include "texture_bind.h"



namespace Luaer {

	Luna<RenderPath3D_BindLua>::FunctionType RenderPath3D_BindLua::methods[] = {
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
		lunamethod(RenderPath2D_BindLua, GetHDRScaling),
		lunamethod(RenderPath2D_BindLua, SetHDRScaling),

		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),

		lunamethod(RenderPath3D_BindLua, SetResolutionScale),
		lunamethod(RenderPath3D_BindLua, SetAO),
		lunamethod(RenderPath3D_BindLua, SetAOPower),
		lunamethod(RenderPath3D_BindLua, SetAORange),
		lunamethod(RenderPath3D_BindLua, SetSSREnabled),
		lunamethod(RenderPath3D_BindLua, SetSSGIEnabled),
		lunamethod(RenderPath3D_BindLua, SetRaytracedDiffuseEnabled),
		lunamethod(RenderPath3D_BindLua, SetRaytracedReflectionsEnabled),
		lunamethod(RenderPath3D_BindLua, SetShadowsEnabled),
		lunamethod(RenderPath3D_BindLua, SetReflectionsEnabled),
		lunamethod(RenderPath3D_BindLua, SetFXAAEnabled),
		lunamethod(RenderPath3D_BindLua, SetBloomEnabled),
		lunamethod(RenderPath3D_BindLua, SetBloomThreshold),
		lunamethod(RenderPath3D_BindLua, SetColorGradingEnabled),
		lunamethod(RenderPath3D_BindLua, SetVolumeLightsEnabled),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsEnabled),
		lunamethod(RenderPath3D_BindLua, SetLensFlareEnabled),
		lunamethod(RenderPath3D_BindLua, SetMotionBlurEnabled),
		lunamethod(RenderPath3D_BindLua, SetDitherEnabled),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldEnabled),
		lunamethod(RenderPath3D_BindLua, SetEyeAdaptionEnabled),
		lunamethod(RenderPath3D_BindLua, SetMSAASampleCount),
		lunamethod(RenderPath3D_BindLua, SetCRTFilterEnabled),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterEnabled),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterAmount),
		lunamethod(RenderPath3D_BindLua, SetExposure),
		lunamethod(RenderPath3D_BindLua, SetHDRCalibration),
		lunamethod(RenderPath3D_BindLua, SetMotionBlurStrength),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldStrength),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsStrength),
		lunamethod(RenderPath3D_BindLua, SetOutlineEnabled),
		lunamethod(RenderPath3D_BindLua, SetOutlineThickness),
		lunamethod(RenderPath3D_BindLua, SetOutlineThreshold),
		lunamethod(RenderPath3D_BindLua, SetOutlineColor),
		lunamethod(RenderPath3D_BindLua, SetFSREnabled),
		lunamethod(RenderPath3D_BindLua, SetFSRSharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Enabled),
		lunamethod(RenderPath3D_BindLua, SetFSR2Sharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Preset),
		lunamethod(RenderPath3D_BindLua, SetTonemap),
		lunamethod(RenderPath3D_BindLua, SetVisibilityComputeShadingEnabled),

		lunamethod(RenderPath3D_BindLua, SetCropLeft),
		lunamethod(RenderPath3D_BindLua, SetCropTop),
		lunamethod(RenderPath3D_BindLua, SetCropRight),
		lunamethod(RenderPath3D_BindLua, SetCropBottom),

		lunamethod(RenderPath3D_BindLua, GetLastPostProcessRT),

		lunamethod(RenderPath3D_BindLua, SetDistortionOverlay),
		lunamethod(RenderPath3D_BindLua, SetChromaticAberrationEnabled),
		lunamethod(RenderPath3D_BindLua, SetChromaticAberrationAmount),

		lunamethod(RenderPath2D_BindLua, CopyFrom),
		{ NULL, NULL }
	};
	Luna<RenderPath3D_BindLua>::PropertyType RenderPath3D_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int RenderPath3D_BindLua::SetResolutionScale(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetResolutionScale(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			float value = Luaer::SGetFloat(L, 1);
			((RenderPath3D*)component)->resolutionScale = value;
		}
		else
			Luaer::SError(L, "SetResolutionScale(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAO(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetAO(AO value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			int value = Luaer::SGetInt(L, 1);
			RenderPath3D::AO ao = (RenderPath3D::AO)value;
			((RenderPath3D*)component)->setAO(ao);
		}
		else
			Luaer::SError(L, "SetAO(AO value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAOPower(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetAOPower(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			float value = Luaer::SGetFloat(L, 1);
			((RenderPath3D*)component)->setAOPower(value);
		}
		else
			Luaer::SError(L, "SetAOPower(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAORange(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetAORange(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			float value = Luaer::SGetFloat(L, 1);
			((RenderPath3D*)component)->setAORange(value);
		}
		else
			Luaer::SError(L, "SetAORange(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetSSREnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSREnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetSSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSGIEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetSSGIEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSGIEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetSSGIEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedDiffuseEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetRaytracedDiffuseEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedDiffuseEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetRaytracedDiffuseEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetRaytracedReflectionsEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedReflectionsEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetRaytracedReflectionsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetShadowsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetShadowsEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setShadowsEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetShadowsEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setReflectionsEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFXAAEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFXAAEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFXAAEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetFXAAEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetBloomEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setBloomEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetBloomEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetBloomThreshold(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setBloomThreshold(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetBloomThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetColorGradingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetColorGradingEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setColorGradingEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetColorGradingEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetVolumeLightsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetVolumeLightsEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setVolumeLightsEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetVolumeLightsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetLightShaftsEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLightShaftsEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetLightShaftsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLensFlareEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetLensFlareEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLensFlareEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetLensFlareEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetMotionBlurEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setMotionBlurEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetMotionBlurEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDitherEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetDitherEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setDitherEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetDitherEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetDepthOfFieldEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetDepthOfFieldEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetEyeAdaptionEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetEyeAdaptionEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setEyeAdaptionEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetEyeAdaptionEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMSAASampleCount(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetMSAASampleCount(int value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMSAASampleCount((uint32_t)Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetMSAASampleCount(int value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetCRTFilterEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetCRTFilterEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setCRTFilterEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetCRTFilterEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetSharpenFilterEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetSharpenFilterEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterAmount(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetSharpenFilterAmount(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterAmount(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetSharpenFilterAmount(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetExposure(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetExposure(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setExposure(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetExposure(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetHDRCalibration(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetHDRCalibration(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setHDRCalibration(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetHDRCalibration(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetMotionBlurStrength(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMotionBlurStrength(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetMotionBlurStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetDepthOfFieldStrength(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldStrength(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetDepthOfFieldStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetLightShaftsStrength(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setLightShaftsStrength(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetLightShaftsStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetOutlineEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetOutlineEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThickness(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetOutlineThickness(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThickness(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetOutlineThickness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetOutlineThreshold(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThreshold(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetOutlineThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineColor(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetOutlineColor(float r,g,b,a) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 3)
		{
		
			((RenderPath3D*)component)->setOutlineColor(Luaer::SGetFloat4(L, 1));
		}
		else
			Luaer::SError(L, "SetOutlineColor(float r,g,b,a) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFSREnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSREnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetFSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSRSharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFSRSharpness(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSRSharpness(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetFSRSharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Enabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFSR2Enabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSR2Enabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetFSR2Enabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Sharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFSR2Sharpness(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Sharpness(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetFSR2Sharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Preset(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetFSR2Preset(FSR2_Preset value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Preset((pf::RenderPath3D::FSR2_Preset)Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetFSR2Preset(FSR2_Preset value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetTonemap(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetTonemap(Tonemap value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setTonemap((pf::renderer::Tonemap)Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetTonemap(Tonemap value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetCropLeft(lua_State* L)
	{
		((RenderPath3D*)component)->crop_left = Luaer::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropTop(lua_State* L)
	{
		((RenderPath3D*)component)->crop_top = Luaer::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropRight(lua_State* L)
	{
		((RenderPath3D*)component)->crop_right = Luaer::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropBottom(lua_State* L)
	{
		((RenderPath3D*)component)->crop_bottom = Luaer::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetVisibilityComputeShadingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetVisibilityComputeShadingEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setVisibilityComputeShadingEnabled(Luaer::SGetBool(L, 1));
		else
			Luaer::SError(L, "SetVisibilityComputeShadingEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::GetLastPostProcessRT(lua_State* L)
	{
		const pf::graphics::Texture* tex = ((RenderPath3D*)component)->GetLastPostprocessRT();
		if (tex == nullptr)
			return 0;
		Luna<Texture_BindLua>::push(L, *tex);
		return 1;
	}

	int RenderPath3D_BindLua::SetDistortionOverlay(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc < 1)
		{
			Luaer::SError(L, "SetDistortionOverlay(Texture texture) not enough arguments!");
			return 0;
		}
		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
		if (tex == nullptr)
		{
			Luaer::SError(L, "SetDistortionOverlay(Texture texture) first argument is not a Texture!");
			return 0;
		}
		if (tex->resource.IsValid())
		{
			((RenderPath3D*)component)->distortion_overlay = tex->resource.GetTexture();
		}
		else
		{
			((RenderPath3D*)component)->distortion_overlay = {};
		}
		return 0;
	}

	int RenderPath3D_BindLua::SetChromaticAberrationEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetChromaticAberrationEnabled(bool value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setChromaticAberrationEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetChromaticAberrationEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetChromaticAberrationAmount(lua_State* L)
	{
		if (component == nullptr)
		{
			Luaer::SError(L, "SetExposure(float value) component is null!");
			return 0;
		}
		if (Luaer::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setChromaticAberrationAmount(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetChromaticAberrationAmount(float value) not enough arguments!");
		return 0;
	}

	static const std::string value_bindings = R"(
AO_DISABLED = 0
AO_SSAO = 1
AO_HBAO = 2
AO_MSAO = 3
AO_RTAO = 4

FSR2_Preset = {
	Quality = 0,
	Balanced = 1,
	Performance = 2,
	Ultra_Performance = 3,
}

Tonemap = {
	Reinhard = 0,
	ACES = 1,
}
)";

	void RenderPath3D_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<RenderPath3D_BindLua>::Register(Luaer::GetLuaState());

			Luaer::RunText(value_bindings);
		}
	}

}