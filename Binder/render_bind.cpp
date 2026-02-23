#include "render_bind.h"
#include "Engine/Ecore/renderer.h"
#include "Engine/Ecore/texture_helper.h"
#include "Engine/Ecore/scene.h"
#include "scene_bind.h"
#include "math_bind.h"
#include "texture_bind.h"
#include "Engine/Ecore/emitted_particle.h"
#include "Engine/Ecore/hair_particle.h"
#include "primitive_bind.h"
#include "Module/Util/event_handle.h"
#include "voxel_grid_bind.h"
#include "path_query_bind.h"
#include "trail_renderer_bind.h"

using namespace pf::ecs;
using namespace pf::graphics;
using namespace pf::scene;
using namespace Luaer::scene;
using namespace Luaer::primitive;

namespace Luaer::renderer
{
	int SetGamma(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Luaer::SSetString(L, "SetGamma() no longer supported!");
		}
		else
		{
			Luaer::SError(L, "SetGamma(float) not enough arguments!");
		}
		return 0;
	}
	int SetGameSpeed(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetGameSpeed(Luaer::SGetFloat(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetGameSpeed(float) not enough arguments!");
		}
		return 0;
	}
	int GetGameSpeed(lua_State* L)
	{
		Luaer::SSetFloat(L, pf::renderer::GetGameSpeed());
		return 1;
	}
	int IsRaytracingSupported(lua_State* L)
	{
		Luaer::SSetBool(L, pf::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::RAYTRACING));
		return 1;
	}

	int SetShadowProps2D(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetShadowProps2D(Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetShadowProps2D(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetShadowPropsCube(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetShadowPropsCube(Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetShadowPropsCube(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetDebugPartitionTreeEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugPartitionTree(Luaer::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugBoxesEnabled(lua_State* L)
	{
		Luaer::SError(L, "SetDebugBoxesEnabled is obsolete! Use SetDebugPartitionTreeEnabled(bool value) instead to draw a partition tree!");
		return 0;
	}
	int SetDebugBonesEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugBoneLines(Luaer::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEmittersEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugEmitters(Luaer::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEnvProbesEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugEnvProbes(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDebugEnvProbesEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugForceFieldsEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugForceFields(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDebugForceFieldsEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCamerasEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugCameras(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDebugCamerasEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCollidersEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawDebugColliders(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDebugCollidersEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetGridHelperEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetToDrawGridHelper(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetGridHelperEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDDGIDebugEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetDDGIDebugEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDDGIDebugEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDDGIEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetDDGIEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDDGIEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetVSyncEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::eventhandler::SetVSync(Luaer::SGetBool(L, 1));
		}
		return 0;
	}
	int SetResolution(lua_State* L)
	{
		Luaer::SError(L, "SetResolution() is deprecated, now it's handled by window events!");
		return 0;
	}
	int SetDebugLightCulling(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetDebugLightCulling(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetDebugLightCulling(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetOcclusionCullingEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetOcclusionCullingEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetOcclusionCullingEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetTemporalAAEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetTemporalAAEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetTemporalAAEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetRaytracedShadowsEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetRaytracedShadowsEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetRaytracedShadowsEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetMeshShaderAllowed(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetMeshShaderAllowed(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetMeshShaderAllowed(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetMeshletOcclusionCullingEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetMeshletOcclusionCullingEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetMeshletOcclusionCullingEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetCapsuleShadowEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetCapsuleShadowEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowFade(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetCapsuleShadowFade(Luaer::SGetFloat(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetCapsuleShadowFade(float value) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowAngle(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetCapsuleShadowAngle(Luaer::SGetFloat(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetCapsuleShadowAngle(float value) not enough arguments!");
		}
		return 0;
	}
	int SetShadowLODOverrideEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::renderer::SetShadowLODOverrideEnabled(Luaer::SGetBool(L, 1));
		}
		else
		{
			Luaer::SError(L, "SetShadowLODOverrideEnabled(bool value) not enough arguments!");
		}
		return 0;
	}

	int DrawLine(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (a && b)
			{
				pf::renderer::RenderableLine line;
				XMStoreFloat3(&line.start, XMLoadFloat4(&a->data));
				XMStoreFloat3(&line.end, XMLoadFloat4(&b->data));
				bool depth = false;
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
					{
						XMStoreFloat4(&line.color_start, XMLoadFloat4(&c->data));
						XMStoreFloat4(&line.color_end, XMLoadFloat4(&c->data));
					}
					else
						Luaer::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");

					if (argc > 3)
					{
						depth = Luaer::SGetBool(L, 4);
					}
				}
				pf::renderer::DrawLine(line, depth);
			}
			else
				Luaer::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");
		}
		else
			Luaer::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) not enough arguments!");

		return 0;
	}
	int DrawPoint(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (a)
			{
				pf::renderer::RenderablePoint point;
				XMStoreFloat3(&point.position, XMLoadFloat4(&a->data));
				bool depth = false;
				if (argc > 1)
				{
					point.size = Luaer::SGetFloat(L, 2);

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color)
						{
							point.color = color->data;
						}

						if (argc > 3)
						{
							depth = Luaer::SGetBool(L, 4);
						}
					}
				}
				pf::renderer::DrawPoint(point, depth);
			}
			else
				Luaer::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) first argument must be a Vector type!");
		}
		else
			Luaer::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) not enough arguments!");

		return 0;
	}
	int DrawBox(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Luaer::SGetBool(L, 3);
						}
						pf::renderer::DrawBox(m->data, color->data, depth);
						return 0;
					}
				}

				pf::renderer::DrawBox(m->data);
			}
			else
				Luaer::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Luaer::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawSphere(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Luaer::SGetBool(L, 3);
						}
						pf::renderer::DrawSphere(sphere->sphere, color->data, depth);
						return 0;
					}
				}

				pf::renderer::DrawSphere(sphere->sphere);
			}
			else
				Luaer::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Luaer::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawCapsule(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (capsule)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Luaer::SGetBool(L, 3);
						}
						pf::renderer::DrawCapsule(capsule->capsule, color->data, depth);
						return 0;
					}
				}

				pf::renderer::DrawCapsule(capsule->capsule);
			}
			else
				Luaer::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Luaer::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawDebugText(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			std::string text = Luaer::SGetString(L, 1);
			pf::renderer::DebugTextParams params;
			if (argc > 1)
			{
				Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 2);
				if (position != nullptr)
				{
					params.position.x = position->data.x;
					params.position.y = position->data.y;
					params.position.z = position->data.z;

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color != nullptr)
						{
							params.color = color->data;

							if (argc > 3)
							{
								params.scaling = Luaer::SGetFloat(L, 4);

								if (argc > 4)
								{
									params.flags = Luaer::SGetInt(L, 5);
								}
							}
						}
						else
							Luaer::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) third argument was not a Vector!");
					}
				}
				else
					Luaer::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) second argument was not a Vector!");

			}
			pf::renderer::DrawDebugText(text.c_str(), params);
		}
		else
			Luaer::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) not enough arguments!");

		return 0;
	}
	int DrawVoxelGrid(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			VoxelGrid_BindLua* a = Luna<VoxelGrid_BindLua>::lightcheck(L, 1);
			if (a)
			{
				pf::renderer::DrawVoxelGrid(a->voxelgrid);
			}
			else
				Luaer::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) first argument must be a VoxelGrid type!");
		}
		else
			Luaer::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) not enough arguments!");

		return 0;
	}
	int DrawPathQuery(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			PathQuery_BindLua* a = Luna<PathQuery_BindLua>::lightcheck(L, 1);
			if (a)
			{
				pf::renderer::DrawPathQuery(a->pathquery);
			}
			else
				Luaer::SError(L, "DrawPathQuery(PathQuery pathquery) first argument must be a PathQuery type!");
		}
		else
			Luaer::SError(L, "DrawPathQuery(PathQuery pathquery) not enough arguments!");

		return 0;
	}
	int DrawTrail(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			TrailRenderer_BindLua* a = Luna<TrailRenderer_BindLua>::lightcheck(L, 1);
			if (a)
			{
				pf::renderer::DrawTrail(&a->trail);
			}
			else
				Luaer::SError(L, "DrawTrail(TrailRenderer trail) first argument must be a TrailRenderer type!");
		}
		else
			Luaer::SError(L, "DrawTrail(TrailRenderer trail) not enough arguments!");

		return 0;
	}

	class PaintTextureParams_BindLua
	{
	public:
		pf::renderer::PaintTextureParams params;

		PaintTextureParams_BindLua(const pf::renderer::PaintTextureParams& params) : params(params) {}
		PaintTextureParams_BindLua(lua_State* L) {}

		int SetEditTexture(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetEditTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Luaer::SError(L, "SetEditTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.editTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetBrushTexture(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Luaer::SError(L, "SetBrushTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.brushTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetRevealTexture(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetRevealTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Luaer::SError(L, "SetRevealTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.revealTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetCenterPixel(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetCenterPixel(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetCenterPixel(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushCenter.x = uint32_t(vec->data.x);
			params.push.xPaintBrushCenter.y = uint32_t(vec->data.y);
			return 0;
		}
		int SetBrushColor(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushColor(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetBrushColor(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushColor = pf::Color::fromFloat4(vec->data);
			return 0;
		}
		int SetBrushRadius(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushRadius(int value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRadius = Luaer::SGetInt(L, 1);
			return 0;
		}
		int SetBrushAmount(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushAmount(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushAmount = Luaer::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushSmoothness(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushSmoothness(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushSmoothness = Luaer::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushRotation(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushRotation(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRotation = Luaer::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushShape(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetBrushShape(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushShape = Luaer::SGetInt(L, 1);
			return 0;
		}

		inline static constexpr char className[] = "PaintTextureParams";
		inline static constexpr Luna<PaintTextureParams_BindLua>::FunctionType methods[] = {
			lunamethod(PaintTextureParams_BindLua, SetEditTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushTexture),
			lunamethod(PaintTextureParams_BindLua, SetRevealTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushColor),
			lunamethod(PaintTextureParams_BindLua, SetCenterPixel),
			lunamethod(PaintTextureParams_BindLua, SetBrushRadius),
			lunamethod(PaintTextureParams_BindLua, SetBrushAmount),
			lunamethod(PaintTextureParams_BindLua, SetBrushSmoothness),
			lunamethod(PaintTextureParams_BindLua, SetBrushRotation),
			lunamethod(PaintTextureParams_BindLua, SetBrushShape),
			{ nullptr, nullptr }
		};
		inline static constexpr Luna<PaintTextureParams_BindLua>::PropertyType properties[] = {
			{ nullptr, nullptr }
		};
	};

	int PaintIntoTexture(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc < 1)
		{
			Luaer::SError(L, "PaintIntoTexture(PaintTextureParams params): not enough arguments!");
			return 0;
		}
		PaintTextureParams_BindLua* params = Luna<PaintTextureParams_BindLua>::lightcheck(L, 1);
		if (params == nullptr)
		{
			Luaer::SError(L, "PaintIntoTexture(PaintTextureParams params): argument is not a PaintTextureParams!");
			return 0;
		}
		pf::renderer::PaintIntoTexture(params->params);
		return 0;
	}
	int CreatePaintableTexture(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc < 2)
		{
			Luaer::SError(L, "CreatePaintableTexture(int width,height, opt int mips = 0, opt Vector initialColor = Vector()): not enough arguments!");
			return 0;
		}
		uint32_t width = (uint32_t)Luaer::SGetInt(L, 1);
		uint32_t height = (uint32_t)Luaer::SGetInt(L, 2);
		uint32_t mips = 0;
		pf::Color color = pf::Color::Transparent();
		if (argc > 2)
		{
			mips = (uint32_t)Luaer::SGetInt(L, 3);
			if (argc > 3)
			{
				Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (v != nullptr)
				{
					color = pf::Color::fromFloat4(v->data);
				}
			}
		}
		Luna<Texture_BindLua>::push(L, pf::renderer::CreatePaintableTexture(width, height, mips, color));
		return 1;
	}


	class PaintDecalParams_BindLua
	{
	public:
		pf::renderer::PaintDecalParams params;

		PaintDecalParams_BindLua(const pf::renderer::PaintDecalParams& params) : params(params) {}
		PaintDecalParams_BindLua(lua_State* L) {}

		int SetInTexture(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetInTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Luaer::SError(L, "SetInTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.in_texture = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetOutTexture(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetOutTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Luaer::SError(L, "SetOutTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.out_texture = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetMatrix(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetMatrix(Matrix mat): not enough arguments!");
				return 0;
			}
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (mat == nullptr)
			{
				Luaer::SError(L, "SetMatrix(Texture mat): argument is not a Texture!");
				return 0;
			}
			params.decalMatrix = mat->data;
			return 0;
		}
		int SetObject(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetObject(Entity entity): not enough arguments!");
				return 0;
			}
			params.objectEntity = (pf::ecs::Entity)Luaer::SGetLongLong(L, 1);
			return 0;
		}
		int SetSlopeBlendPower(lua_State* L)
		{
			int argc = Luaer::SGetArgCount(L);
			if (argc < 1)
			{
				Luaer::SError(L, "SetSlopeBlendPower(float power): not enough arguments!");
				return 0;
			}
			params.slopeBlendPower = Luaer::SGetFloat(L, 1);
			return 0;
		}

		inline static constexpr char className[] = "PaintDecalParams";
		inline static constexpr Luna<PaintDecalParams_BindLua>::FunctionType methods[] = {
			lunamethod(PaintDecalParams_BindLua, SetInTexture),
			lunamethod(PaintDecalParams_BindLua, SetOutTexture),
			lunamethod(PaintDecalParams_BindLua, SetMatrix),
			lunamethod(PaintDecalParams_BindLua, SetObject),
			lunamethod(PaintDecalParams_BindLua, SetSlopeBlendPower),
			{ nullptr, nullptr }
		};
		inline static constexpr Luna<PaintDecalParams_BindLua>::PropertyType properties[] = {
			{ nullptr, nullptr }
		};
	};
	int PaintDecalIntoObjectSpaceTexture(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc < 1)
		{
			Luaer::SError(L, "PaintDecalIntoObjectSpaceTexture(PaintDecalParams params): not enough arguments!");
			return 0;
		}
		PaintDecalParams_BindLua* params = Luna<PaintDecalParams_BindLua>::lightcheck(L, 1);
		if (params == nullptr)
		{
			Luaer::SError(L, "PaintDecalIntoObjectSpaceTexture(PaintDecalParams params): argument is not a PaintDecalParams!");
			return 0;
		}
		pf::renderer::PaintDecalIntoObjectSpaceTexture(params->params);
		return 0;
	}

	int PutWaterRipple(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			std::string name = Luaer::SGetString(L, 1);
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(name, v->GetFloat3());
			}
			else
				Luaer::SError(L, "PutWaterRipple(string imagename, Vector position) argument is not a Vector!");
		}
		else if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(v->GetFloat3());
			}
			else
				Luaer::SError(L, "PutWaterRipple(Vector position) argument is not a Vector!");
		}
		else
		{
			Luaer::SError(L, "PutWaterRipple(Vector position) not enough arguments!");
			Luaer::SError(L, "PutWaterRipple(string imagename, Vector position) not enough arguments!");
		}
		return 0;
	}

	int ClearWorld(lua_State* L)
	{
		Scene_BindLua* scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (scene == nullptr)
		{
			pf::renderer::ClearWorld(*GetGlobalScene());
		}
		else
		{
			pf::renderer::ClearWorld(*scene->scene);
		}
		return 0;
	}
	int ReloadShaders(lua_State* L)
	{
		pf::renderer::ReloadShaders();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			Luna<PaintTextureParams_BindLua>::Register(Luaer::GetLuaState());
			Luna<PaintDecalParams_BindLua>::Register(Luaer::GetLuaState());

			Luaer::RegisterFunc("SetGamma", SetGamma);
			Luaer::RegisterFunc("SetGameSpeed", SetGameSpeed);
			Luaer::RegisterFunc("GetGameSpeed", GetGameSpeed);
			Luaer::RegisterFunc("IsRaytracingSupported", IsRaytracingSupported);

			Luaer::RegisterFunc("SetShadowProps2D", SetShadowProps2D);
			Luaer::RegisterFunc("SetShadowPropsCube", SetShadowPropsCube);
			Luaer::RegisterFunc("SetDebugBoxesEnabled", SetDebugBoxesEnabled);
			Luaer::RegisterFunc("SetDebugPartitionTreeEnabled", SetDebugPartitionTreeEnabled);
			Luaer::RegisterFunc("SetDebugBonesEnabled", SetDebugBonesEnabled);
			Luaer::RegisterFunc("SetDebugEmittersEnabled", SetDebugEmittersEnabled);
			Luaer::RegisterFunc("SetDebugEnvProbesEnabled", SetDebugEnvProbesEnabled);
			Luaer::RegisterFunc("SetDebugForceFieldsEnabled", SetDebugForceFieldsEnabled);
			Luaer::RegisterFunc("SetDebugCamerasEnabled", SetDebugCamerasEnabled);
			Luaer::RegisterFunc("SetDebugCollidersEnabled", SetDebugCollidersEnabled);
			Luaer::RegisterFunc("SetGridHelperEnabled", SetGridHelperEnabled);
			Luaer::RegisterFunc("SetDDGIDebugEnabled", SetDDGIDebugEnabled);
			Luaer::RegisterFunc("SetDDGIEnabled", SetDDGIEnabled);
			Luaer::RegisterFunc("SetVSyncEnabled", SetVSyncEnabled);
			Luaer::RegisterFunc("SetResolution", SetResolution);
			Luaer::RegisterFunc("SetDebugLightCulling", SetDebugLightCulling);
			Luaer::RegisterFunc("SetOcclusionCullingEnabled", SetOcclusionCullingEnabled);
			Luaer::RegisterFunc("SetTemporalAAEnabled", SetTemporalAAEnabled);
			Luaer::RegisterFunc("SetRaytracedShadowsEnabled", SetRaytracedShadowsEnabled);
			Luaer::RegisterFunc("SetMeshShaderAllowed", SetMeshShaderAllowed);
			Luaer::RegisterFunc("SetMeshletOcclusionCullingEnabled", SetMeshletOcclusionCullingEnabled);
			Luaer::RegisterFunc("SetCapsuleShadowEnabled", SetCapsuleShadowEnabled);
			Luaer::RegisterFunc("SetCapsuleShadowFade", SetCapsuleShadowFade);
			Luaer::RegisterFunc("SetCapsuleShadowAngle", SetCapsuleShadowAngle);
			Luaer::RegisterFunc("SetShadowLODOverrideEnabled", SetShadowLODOverrideEnabled);

			Luaer::RegisterFunc("DrawLine", DrawLine);
			Luaer::RegisterFunc("DrawPoint", DrawPoint);
			Luaer::RegisterFunc("DrawBox", DrawBox);
			Luaer::RegisterFunc("DrawSphere", DrawSphere);
			Luaer::RegisterFunc("DrawCapsule", DrawCapsule);
			Luaer::RegisterFunc("DrawDebugText", DrawDebugText);
			Luaer::RegisterFunc("DrawVoxelGrid", DrawVoxelGrid);
			Luaer::RegisterFunc("DrawPathQuery", DrawPathQuery);
			Luaer::RegisterFunc("DrawTrail", DrawTrail);

			Luaer::RegisterFunc("PaintIntoTexture", PaintIntoTexture);
			Luaer::RegisterFunc("CreatePaintableTexture", CreatePaintableTexture);
			Luaer::RegisterFunc("PaintDecalIntoObjectSpaceTexture", PaintDecalIntoObjectSpaceTexture);

			Luaer::RegisterFunc("PutWaterRipple", PutWaterRipple);

			Luaer::RegisterFunc("ClearWorld", ClearWorld);
			Luaer::RegisterFunc("ReloadShaders", ReloadShaders);

			Luaer::RunText(R"(
GetScreenWidth = function() return main.GetCanvas().GetLogicalWidth() end
GetScreenHeight = function() return main.GetCanvas().GetLogicalHeight() end

DEBUG_TEXT_DEPTH_TEST = 1
DEBUG_TEXT_CAMERA_FACING = 2
DEBUG_TEXT_CAMERA_SCALING = 4
)");

		}
	}
};