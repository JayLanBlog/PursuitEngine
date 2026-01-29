#pragma once
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"
#include "scene_decl.h"


namespace pf
{
	struct GPUBVH
	{
		// Scene BVH intersection resources:
		pf::graphics::GPUBuffer bvhNodeBuffer;
		pf::graphics::GPUBuffer bvhParentBuffer;
		pf::graphics::GPUBuffer bvhFlagBuffer;
		pf::graphics::GPUBuffer primitiveCounterBuffer;
		pf::graphics::GPUBuffer primitiveIDBuffer;
		pf::graphics::GPUBuffer primitiveBuffer;
		pf::graphics::GPUBuffer primitiveMortonBuffer;
		uint32_t primitiveCapacity = 0;
		bool IsValid() const { return primitiveCounterBuffer.IsValid(); }

		void Update(const pf::scene::Scene& scene);
		void Build(const pf::scene::Scene& scene, pf::graphics::CommandList cmd) const;

		void Clear();

		static void Initialize();
	};
}
