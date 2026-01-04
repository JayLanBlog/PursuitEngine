#include "data_primitive.h"
#include <glad/glad.h>
namespace GRAPHT {

	static const GLenum glElementTypes[] =
	{
		GL_INT,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_UNSIGNED_BYTE,
		GL_UNSIGNED_BYTE
	};

	static const GLint glElementComponents[] =
	{
		1,
		1,
		2,
		3,
		4,
		4,
		4
	};

	Primitve::Primitve() {
		vbo = new GLVBO();
		ibo = new GLIBO();
	}
	Primitve::~Primitve() {

		if (vbo) {
			delete vbo;
		}
		if (ibo) {
			delete ibo;
		}
	}
	void Primitve::Draw() {
		if (vbo) {
			vbo->bind();
			iboDraw ?
				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(ibo->getCount()), GL_UNSIGNED_INT, 0) :
				glDrawArrays(GL_TRIANGLES, 0, vbo->data_count);
			
		}
	}

	void Primitve::pushElement(VertexElement element) {
		elements.push_back(element);
	}

	void Primitve::generateElements() {
		int idx = 0;
		
		for (VertexElement el : elements)
		{
			GLint components = glElementComponents[el.type];
			GLenum typeEnum = glElementTypes[el.type];
			int strade = vbo->getStradeSize();
			 el.itemCount ==-1 ? glVertexAttribPointer(idx, components, typeEnum, el.type == TYPE_UBYTE4_NORM ? GL_TRUE : GL_FALSE, strade, (void*)el.offset)
				: glVertexAttribIPointer(idx, el.itemCount, typeEnum, strade, (void*)el.offset);;
			
			glEnableVertexAttribArray(idx);
			idx++;
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
		// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
		glBindVertexArray(0);
	}
}