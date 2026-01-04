#pragma once
#include <common/core.h>
#include <glad/glad.h>


namespace GRAPHT {





	enum DataType
	{
		BYTE =0 ,
		UNSIGNED_BYTE,
		SHORT,
		UNSIGNED_SHORT,
		INT,
		UNSIGNED_INT,
		FLOAT

	};
	static const GLenum DataTypes[] =
	{
		GL_BYTE,
		GL_UNSIGNED_BYTE,
		GL_SHORT,
		GL_UNSIGNED_SHORT,
		GL_INT,
		GL_UNSIGNED_INT,
		GL_FLOAT
	};
	enum Filter
	{
		NEARE = 0,
		LINEAR,
		MIPMAP_LINEAR
	};

	static const GLenum Filters[] =
	{
		GL_NEAREST,
		GL_LINEAR,
		GL_LINEAR_MIPMAP_LINEAR
	};


	enum Wrap
	{
		REPEAT = 0,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		BORDER
		
	};

	static const GLenum Wraps[] =
	{
		GL_REPEAT,
		GL_MIRRORED_REPEAT,
		GL_CLAMP_TO_EDGE,
		GL_CLAMP_TO_BORDER
	};


	enum ColorSpace
	{
		RGB = 0,
		RGBA,
		RED
	};

	static const GLenum COLOR_SPACE[] = {
		GL_RGB,
		GL_RGBA,
		GL_RED
	};

	const int ELEMENT_TYPE_SIZE[] = {
		  sizeof(int),
		sizeof(float),
		2 * sizeof(float),
		3 * sizeof(float),
		4 * sizeof(float),
		sizeof(unsigned),
		sizeof(unsigned)
	};

	enum VertexElementType
	{
		TYPE_INT = 0,
		TYPE_FLOAT,
		TYPE_VECTOR2,
		TYPE_VECTOR3,
		TYPE_VECTOR4,
		TYPE_UBYTE4,
		TYPE_UBYTE4_NORM,
		MAX_VERTEX_ELEMENT_TYPES
	};

	enum VertexElementSemantic
	{
		SEM_POSITION = 0,
		SEM_NORMAL,
		SEM_BINORMAL,
		SEM_TANGENT,
		SEM_TEXCOORD,
		SEM_COLOR,
		SEM_BLENDWEIGHTS,
		SEM_BLENDINDICES,
		SEM_OBJECTINDEX,
		SEM_BONE_ID,
		SEM_BONE_WEIGHT,
		SEM_BIGTANGENT,
		MAX_VERTEX_ELEMENT_SEMANTICS
	};

	/// Legacy vertex element bitmasks.
	enum class VertexElements : unsigned
	{
		None = 0,
		Position = 1 << 0,
		Normal = 1 << 1,
		Color = 1 << 2,
		TexCoord1 = 1 << 3,
		TexCoord2 = 1 << 4,
		CubeTexCoord1 = 1 << 5,
		CubeTexCoord2 = 1 << 6,
		Tangent = 1 << 7,
		BlendWeights = 1 << 8,
		BlendIndices = 1 << 9,
		InstanceMatrix1 = 1 << 10,
		InstanceMatrix2 = 1 << 11,
		InstanceMatrix3 = 1 << 12,
		ObjectIndex = 1 << 13
	};
}