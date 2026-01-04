#pragma once
#include <graph/graph_def.h>

namespace GRAPHT {
	struct VertexElement
	{
		/// Default-construct.
		VertexElement() noexcept :
			type(TYPE_VECTOR3),
			semantic(SEM_POSITION),
			index(0),
			perInstance(false),
			offset(0),
			itemCount(-1)
		{
			
			
		}
		VertexElement(VertexElementType _type, VertexElementSemantic _semantic,int prop_count = -1, signed char _index = 0, bool _perInstance = false) noexcept :
			type(_type),
			semantic(_semantic),
			index(_index),
			perInstance(_perInstance),
			offset(0),
			itemCount(prop_count)
		{
			
		}
		VertexElementType type;
		VertexElementSemantic semantic;
		int itemCount;
		signed char index;
		bool perInstance;
		int offset;
		bool operator !=(const VertexElement& rhs) const { return !(*this == rhs); }
		/// Test for equality with another vertex element. Offset is intentionally not compared, as it's relevant only when an element exists within a vertex buffer.
		bool operator ==(const VertexElement& rhs) const
		{
			return type == rhs.type && semantic == rhs.semantic && index == rhs.index && perInstance == rhs.perInstance;
		}
	};

}
