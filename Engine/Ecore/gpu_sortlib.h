#pragma once
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"

namespace pf::gpusortlib
{
	// Perform bitonic sort on a GPU dataset
	//	maxCount				-	Maximum size of the dataset. GPU count can be smaller (see: counterBuffer_read param)
	//	comparisonBuffer_read	-	Buffer containing values to compare by (Read Only)
	//	counterBuffer_read		-	Buffer containing count of values to sort (Read Only)
	//	counterReadOffset		-	Byte offset into the counter buffer to read the count value (Read Only)
	//	indexBuffer_write		-	The index list which to sort. Contains index values which can index the sortBase_read buffer. This will be modified (Read + Write)
	void Sort(
		uint32_t maxCount,
		const pf::graphics::GPUBuffer& comparisonBuffer_read,
		const pf::graphics::GPUBuffer& counterBuffer_read,
		uint32_t counterReadOffset,
		const pf::graphics::GPUBuffer& indexBuffer_write,
		pf::graphics::CommandList cmd
	);

	void Initialize();
};
