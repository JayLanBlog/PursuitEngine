#pragma once
#include "Core/core_include.h"
#include "Module/Container/container_include.h"
#include "Engine/Device/graph_driver.h"
#include "Module/Math/pf_math.h"
#include "enums.h"
#include "ecs.h"
#include "primitive.h"
#include "scene_decl.h"
#include "scene_component.h"



namespace pf
{
	class Archive;
}

namespace pf
{
	class HairParticleSystem
	{
	public:
		pf::graphics::GPUBuffer constantBuffer;
		pf::graphics::GPUBuffer generalBuffer;
		pf::scene::MeshComponent::BufferView simulation_view;
		pf::scene::MeshComponent::BufferView vb_pos[2];
		pf::scene::MeshComponent::BufferView vb_nor;
		pf::scene::MeshComponent::BufferView vb_pos_raytracing;
		pf::scene::MeshComponent::BufferView vb_uvs;
		pf::scene::MeshComponent::BufferView wetmap;
		pf::scene::MeshComponent::BufferView ib_culled;
		pf::scene::MeshComponent::BufferView indirect_view;
		pf::scene::MeshComponent::BufferView prim_view;

		pf::graphics::GPUBuffer indexBuffer;
		pf::graphics::GPUBuffer vertexBuffer_length;

		pf::graphics::RaytracingAccelerationStructure BLAS;

		void CreateFromMesh(const pf::scene::MeshComponent& mesh);
		void CreateRenderData();
		void CreateRaytracingRenderData();

		void UpdateCPU(
			const pf::scene::TransformComponent& transform,
			const pf::scene::MeshComponent& mesh,
			float dt
		);

		struct UpdateGPUItem
		{
			const HairParticleSystem* hair = nullptr;
			uint32_t instanceIndex = 0;
			const pf::scene::MeshComponent* mesh = nullptr;
			const pf::scene::MaterialComponent* material = nullptr;
		};
		// Update a batch of hair particles by GPU
		static void UpdateGPU(
			const UpdateGPUItem* items,
			uint32_t itemCount,
			pf::graphics::CommandList cmd
		);

		void Draw(
			const pf::scene::MaterialComponent& material,
			pf::enums::RENDERPASS renderPass,
			pf::graphics::CommandList cmd
		) const;

		pf::ecs::Entity meshID = pf::ecs::INVALID_ENTITY;

		enum FLAGS
		{
			EMPTY = 0,
			_DEPRECATED_REGENERATE_FRAME = 1 << 0,
			REBUILD_BUFFERS = 1 << 1,
			DIRTY = 1 << 2,
			CAMERA_BEND_ENABLED = 1 << 3,
		};
		uint32_t _flags = CAMERA_BEND_ENABLED;

		uint32_t strandCount = 0;
		uint32_t segmentCount = 1;
		uint32_t billboardCount = 1;
		uint32_t randomSeed = 1;
		float length = 1.0f;
		float stiffness = 0.5f;
		float drag = 0.1f;
		float gravityPower = 0;
		float randomness = 0.2f;
		float viewDistance = 200;
		pf::vector<float> vertex_lengths;
		float width = 1;
		float uniformity = 1;

		struct AtlasRect
		{
			XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
			float size = 1;
		};
		pf::vector<AtlasRect> atlas_rects;

		// Non-serialized attributes:
		XMFLOAT4X4 world;
		pf::primitive::AABB aabb;
		pf::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths
		uint32_t layerMask = ~0u;
		mutable bool regenerate_frame = true;
		pf::graphics::Format position_format = pf::graphics::Format::R16G16B16A16_UNORM;
		mutable bool must_rebuild_blas = true;
		mutable bool gpu_initialized = false;

		void Serialize(pf::Archive& archive, pf::ecs::EntitySerializer& seri);

		static void Initialize();

		constexpr uint32_t GetParticleCount() const { return strandCount * segmentCount; }
		constexpr uint32_t GetVertexCount() const { return strandCount * (segmentCount * 2 + 2) * billboardCount; }
		constexpr uint32_t GetIndexCount() const { return 6 * GetParticleCount() * billboardCount; }
		uint64_t GetMemorySizeInBytes() const;

		constexpr bool IsDirty() const { return _flags & DIRTY; }
		constexpr bool IsCameraBendEnabled() const { return _flags & CAMERA_BEND_ENABLED; }

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr void SetCameraBendEnabled(bool value = true) { if (value) { _flags |= CAMERA_BEND_ENABLED; } else { _flags &= ~CAMERA_BEND_ENABLED; } }

		void ConvertFromOLDSpriteSheet(uint32_t framesX, uint32_t framesY, uint32_t frameCount, uint32_t frameStart);
	};
}
