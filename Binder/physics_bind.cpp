#include "physics_bind.h" 
#include "scene_bind.h"
#include "math_bind.h"
#include "primitive_bind.h"
namespace Luaer
{
	Luna<Physics_BindLua>::FunctionType Physics_BindLua::methods[] = {
		lunamethod(Physics_BindLua, SetEnabled),
		lunamethod(Physics_BindLua, IsEnabled),
		lunamethod(Physics_BindLua, SetSimulationEnabled),
		lunamethod(Physics_BindLua, IsSimulationEnabled),
		lunamethod(Physics_BindLua, SetInterpolationEnabled),
		lunamethod(Physics_BindLua, IsInterpolationEnabled),
		lunamethod(Physics_BindLua, SetDebugDrawEnabled),
		lunamethod(Physics_BindLua, IsDebugDrawEnabled),
		lunamethod(Physics_BindLua, SetAccuracy),
		lunamethod(Physics_BindLua, GetAccuracy),
		lunamethod(Physics_BindLua, SetFrameRate),
		lunamethod(Physics_BindLua, GetFrameRate),
		lunamethod(Physics_BindLua, SetPosition),
		lunamethod(Physics_BindLua, SetPositionAndRotation),
		lunamethod(Physics_BindLua, SetLinearVelocity),
		lunamethod(Physics_BindLua, SetAngularVelocity),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyForce),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyImpulse),
		lunamethod(Physics_BindLua, ApplyImpulseAt),
		lunamethod(Physics_BindLua, ApplyTorque),
		lunamethod(Physics_BindLua, SetActivationState),
		lunamethod(Physics_BindLua, ActivateAllRigidBodies),
		lunamethod(Physics_BindLua, ResetPhysicsObjects),
		lunamethod(Physics_BindLua, GetVelocity),
		lunamethod(Physics_BindLua, GetPosition),
		lunamethod(Physics_BindLua, GetRotation),
		lunamethod(Physics_BindLua, GetCharacterGroundPosition),
		lunamethod(Physics_BindLua, GetCharacterGroundNormal),
		lunamethod(Physics_BindLua, GetCharacterGroundVelocity),
		lunamethod(Physics_BindLua, IsCharacterGroundSupported),
		lunamethod(Physics_BindLua, GetCharacterGroundState),
		lunamethod(Physics_BindLua, ChangeCharacterShape),
		lunamethod(Physics_BindLua, MoveCharacter),
		lunamethod(Physics_BindLua, SetGhostMode),
		lunamethod(Physics_BindLua, SetRagdollGhostMode),
		lunamethod(Physics_BindLua, Intersects),
		lunamethod(Physics_BindLua, PickDrag),
		lunamethod(Physics_BindLua, DriveVehicle),
		lunamethod(Physics_BindLua, GetVehicleForwardVelocity),
		{ NULL, NULL }
	};
	Luna<Physics_BindLua>::PropertyType Physics_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int Physics_BindLua::SetEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsEnabled(lua_State* L)
	{
		Luaer::SSetBool(L, pf::physics::IsEnabled());
		return 1;
	}
	int Physics_BindLua::SetSimulationEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetSimulationEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetSimulationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsSimulationEnabled(lua_State* L)
	{
		Luaer::SSetBool(L, pf::physics::IsSimulationEnabled());
		return 1;
	}
	int Physics_BindLua::SetInterpolationEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetInterpolationEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetInterpolationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsInterpolationEnabled(lua_State* L)
	{
		Luaer::SSetBool(L, pf::physics::IsInterpolationEnabled());
		return 1;
	}
	int Physics_BindLua::SetDebugDrawEnabled(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetDebugDrawEnabled(Luaer::SGetBool(L, 1));
		}
		else
			Luaer::SError(L, "SetDebugDrawEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsDebugDrawEnabled(lua_State* L)
	{
		Luaer::SSetBool(L, pf::physics::IsDebugDrawEnabled());
		return 1;
	}
	int Physics_BindLua::SetAccuracy(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetAccuracy(Luaer::SGetInt(L, 1));
		}
		else
			Luaer::SError(L, "SetAccuracy(int value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetAccuracy(lua_State* L)
	{
		Luaer::SSetInt(L, pf::physics::GetAccuracy());
		return 1;
	}
	int Physics_BindLua::SetFrameRate(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::physics::SetFrameRate(Luaer::SGetFloat(L, 1));
		}
		else
			Luaer::SError(L, "SetFrameRate(float value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetFrameRate(lua_State* L)
	{
		Luaer::SSetFloat(L, pf::physics::GetFrameRate());
		return 1;
	}

	int Physics_BindLua::SetPosition(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) second argument is not a Vector!");
				return 0;
			}
			pf::physics::SetPosition(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetPositionAndRotation(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec2 == nullptr)
			{
				Luaer::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) third argument is not a Vector!");
				return 0;
			}
			pf::physics::SetPositionAndRotation(
				*component->component,
				vec->GetFloat3(),
				vec2->data
			);
		}
		else
			Luaer::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetLinearVelocity(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			pf::physics::SetLinearVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetAngularVelocity(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			pf::physics::SetAngularVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForce(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) second argument is not a Vector!");
				return 0;
			}
			pf::physics::ApplyForce(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForceAt(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = Luaer::SGetBool(L, 4);

			pf::physics::ApplyForceAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			Luaer::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulse(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				scene::HumanoidComponent_BindLua* humanoid = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					Luaer::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) first argument is not a RigidBodyPhysicsComponent!");
					Luaer::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) first argument is not a HumanoidComponent!");
					return 0;
				}
				pf::scene::HumanoidComponent::HumanoidBone bone = (pf::scene::HumanoidComponent::HumanoidBone)Luaer::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					Luaer::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) third argument is not a Vector!");
					return 0;
				}
				pf::physics::ApplyImpulse(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) second argument is not a Vector!");
				return 0;
			}
			pf::physics::ApplyImpulse(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulseAt(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				scene::HumanoidComponent_BindLua* humanoid = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					Luaer::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) first argument is not a RigidBodyPhysicsComponent!");
					Luaer::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) first argument is not a HumanoidComponent!");
					return 0;
				}
				pf::scene::HumanoidComponent::HumanoidBone bone = (pf::scene::HumanoidComponent::HumanoidBone)Luaer::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					Luaer::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) third argument is not a Vector!");
					return 0;
				}
				Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (vec2 == nullptr)
				{
					Luaer::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) fourth argument is not a Vector!");
					return 0;
				}
				bool at_local = true;
				if (argc > 4)
					at_local = Luaer::SGetBool(L, 5);

				pf::physics::ApplyImpulseAt(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec,
					*(XMFLOAT3*)vec2,
					at_local
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = Luaer::SGetBool(L, 4);

			pf::physics::ApplyImpulseAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			Luaer::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyTorque(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) second argument is not a Vector!");
				return 0;
			}
			pf::physics::ApplyTorque(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Luaer::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetActivationState(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rigid = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rigid == nullptr)
			{
				scene::SoftBodyPhysicsComponent_BindLua* soft = Luna<scene::SoftBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
				if (soft == nullptr)
				{
					Luaer::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) first argument is not a RigidBodyPhysicsComponent or SoftBodyPhysicsComponent!");
					return 0;
				}
				pf::physics::SetActivationState(
					*soft->component,
					(pf::physics::ActivationState)Luaer::SGetInt(L, 2)
				);
				return 0;
			}
			pf::physics::SetActivationState(
				*rigid->component,
				(pf::physics::ActivationState)Luaer::SGetInt(L, 2)
			);
		}
		else
			Luaer::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ActivateAllRigidBodies(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Luaer::SError(L, "ActivateAllRigidBodies(Scene scene) first argument is not a Scene!");
				return 0;
			}
			pf::physics::ActivateAllRigidBodies(*scene->scene);
		}
		else
			Luaer::SError(L, "ActivateAllRigidBodies(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ResetPhysicsObjects(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Luaer::SError(L, "ResetPhysicsObjects(Scene scene) first argument is not a Scene!");
				return 0;
			}
			pf::physics::ResetPhysicsObjects(*scene->scene);
		}
		else
			Luaer::SError(L, "ResetPhysicsObjects(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVelocity(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetVelocity(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetPosition(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetPosition(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetRotation(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetRotation(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetRotation(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetRotation(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundPosition(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetCharacterGroundPosition(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundNormal(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetCharacterGroundNormal(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundVelocity(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, pf::physics::GetCharacterGroundVelocity(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsCharacterGroundSupported(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luaer::SSetBool(L, pf::physics::IsCharacterGroundSupported(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundState(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luaer::SSetInt(L, (int)pf::physics::GetCharacterGroundState(*component->component));
			return 1;
		}
		else
			Luaer::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ChangeCharacterShape(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			pf::scene::RigidBodyPhysicsComponent::CapsuleParams capsule;
			if (argc > 1)
			{
				capsule.height = Luaer::SGetFloat(L, 2);
				if (argc > 2)
				{
					capsule.radius = Luaer::SGetFloat(L, 3);
				}
			}
			Luaer::SSetBool(L, pf::physics::ChangeCharacterShape(*component->component, capsule));
			return 1;
		}
		else
			Luaer::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::MoveCharacter(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Luaer::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) second argument is not a Vector!");
				return 0;
			}

			float movement_speed = 6.0f;
			float jump = 0;
			bool controlMovementDuringJump = false;

			if (argc > 2)
			{
				movement_speed = Luaer::SGetFloat(L, 3);
				if (argc > 3)
				{
					jump = Luaer::SGetFloat(L, 4);
					if (argc > 4)
					{
						controlMovementDuringJump = Luaer::SGetBool(L, 5);
					}
				}
			}

			pf::physics::MoveCharacter(
				*component->component,
				vec->GetFloat3(),
				movement_speed,
				jump,
				controlMovementDuringJump
			);
		}
		else
			Luaer::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetGhostMode(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component != nullptr)
			{
				bool value = Luaer::SGetBool(L, 2);
				pf::physics::SetGhostMode(*component->component, value);
				return 0;
			}
			scene::HumanoidComponent_BindLua* humanoidcomponent = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (humanoidcomponent != nullptr)
			{
				bool value = Luaer::SGetBool(L, 2);
				pf::physics::SetGhostMode(*humanoidcomponent->component, value);
				return 0;
			}
		}
		else
			Luaer::SError(L, "SetGhostMode(RigidBodyPhysicsComponent|HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetRagdollGhostMode(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::HumanoidComponent_BindLua* component = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Luaer::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			bool value = Luaer::SGetBool(L, 2);

			pf::physics::SetRagdollGhostMode(*component->component, value);
			return 0;
		}
		else
			Luaer::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::Intersects(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 1)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Luaer::SError(L, "Intersects(Scene, Ray) first argument is not a Scene!");
				return 0;
			}
			primitive::Ray_BindLua* ray = Luna<primitive::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				Luaer::SError(L, "Intersects(Scene, Ray) second argument is not a Ray!");
				return 0;
			}

			pf::physics::RayIntersectionResult result = pf::physics::Intersects(*scene->scene, ray->ray);
			Luaer::SSetLongLong(L, result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			Luaer::SSetLongLong(L, result.humanoid_ragdoll_entity);
			Luaer::SSetInt(L, (int)result.humanoid_bone);
			Luna<Vector_BindLua>::push(L, result.position_local);
			return 6;
		}
		Luaer::SError(L, "Intersects(Scene, Ray) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::PickDrag(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 2)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Luaer::SError(L, "PickDrag(Scene, Ray, PickDragOperation) first argument is not a Scene!");
				return 0;
			}
			primitive::Ray_BindLua* ray = Luna<primitive::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				Luaer::SError(L, "PickDrag(Scene, Ray, PickDragOperation) second argument is not a Ray!");
				return 0;
			}
			PickDragOperation_BindLua* op = Luna<PickDragOperation_BindLua>::lightcheck(L, 3);
			if (op == nullptr)
			{
				Luaer::SError(L, "PickDrag(Scene, Ray, PickDragOperation) third argument is not a PickDragOperation!");
				return 0;
			}

			pf::physics::ConstraintType type = pf::physics::ConstraintType::Fixed;
			float break_distance = FLT_MAX;

			if (argc > 3)
			{
				type = (pf::physics::ConstraintType)Luaer::SGetInt(L, 4);

				if (argc > 4)
				{
					break_distance = Luaer::SGetFloat(L, 5);
				}
			}

			pf::physics::PickDrag(*scene->scene, ray->ray, op->op, type, break_distance);
			return 0;
		}
		Luaer::SError(L, "PickDrag(Scene, Ray, PickDragOperation) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::DriveVehicle(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rb = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				Luaer::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				Luaer::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			float forward = 0;
			float right = 0;
			float brake = 0;
			float handbrake = 0;

			if (argc > 1)
			{
				forward = Luaer::SGetFloat(L, 2);

				if (argc > 2)
				{
					right = Luaer::SGetFloat(L, 3);

					if (argc > 3)
					{
						brake = Luaer::SGetFloat(L, 4);

						if (argc > 4)
						{
							handbrake = Luaer::SGetFloat(L, 5);
						}
					}
				}
			}

			pf::physics::DriveVehicle(*rb->component, forward, right, brake, handbrake);
			return 0;
		}
		Luaer::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVehicleForwardVelocity(lua_State* L)
	{
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rb = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				Luaer::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				Luaer::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			Luaer::SSetFloat(L, pf::physics::GetVehicleForwardVelocity(*rb->component));
			return 1;
		}
		Luaer::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) not enough arguments!");
		return 0;
	}

	void Physics_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<PickDragOperation_BindLua>::Register(Luaer::GetLuaState());
			Luna<Physics_BindLua>::Register(Luaer::GetLuaState());
			Luna<Physics_BindLua>::push_global(Luaer::GetLuaState(), "physics");

			Luaer::RunText(R"(
ACTIVATION_STATE_ACTIVE = 0
ACTIVATION_STATE_INACTIVE = 1

ConstraintType = {
	Fixed = 0,
	Point = 1
}

CharacterGroundStates = {
	OnGround = 0,
	OnSteepGround = 1,
	NotSupported = 2,
	InAir = 3,
}
)");
		}
	}




	Luna<PickDragOperation_BindLua>::FunctionType PickDragOperation_BindLua::methods[] = {
		lunamethod(PickDragOperation_BindLua, Finish),
		{ NULL, NULL }
	};
	Luna<PickDragOperation_BindLua>::PropertyType PickDragOperation_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int PickDragOperation_BindLua::Finish(lua_State* L)
	{
		op = {};
		return 0;
	}

}