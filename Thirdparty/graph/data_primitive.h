#pragma once
#include "graph_def.h"
#include "data_element.h"
#include "data_buffer.h"

namespace GRAPHT {


	class Primitve {
	public:
		GLVBO* vbo;
		GLIBO* ibo;
		bool iboDraw =false;
		Primitve();
		~Primitve();
		std::vector<VertexElement> elements;
		void pushElement(VertexElement element);
		void generateElements();
		void Draw();
	};



}