#include "data_buffer.h"
#include <glad/glad.h>
namespace GRAPHT {
	VertexBuffer::VertexBuffer() {
	
	}
	int VertexBuffer::getBufferSize() {
		return data_size * data_count;
	}

	int VertexBuffer::getStradeSize() {
		return data_size;
	}

	GLVBO::GLVBO()
	{
	}

	GLVBO::~GLVBO()
	{
		if (data) {
			delete data;
		}
		
	}

	void GLVBO::upload_data(void* data, int size, int count) {
		data_count = count;
		data_size = size;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER,getBufferSize(), data, GL_STATIC_DRAW);
		
	}

	void GLVBO::bind() {
		glBindVertexArray(VAO);
	}

	IndexBuffer::IndexBuffer() {
		
	}
	
	int IndexBuffer::getBufferSize() {
		return data_size * data_count;
	}
	int IndexBuffer::getCount() {
		return data_count;
	}

	void GLIBO::upload_data(void* data, int size, int count) {
		data_count = count;
		data_size = size;
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, getBufferSize(), data, GL_STATIC_DRAW);

	}

	GLIBO::GLIBO() {
	
	}
	GLIBO::~GLIBO() {
		if (data) {
			delete data;
		}
	}

	void GLIBO::bind() {
		//glBindVertexArray(EBO);
	}

	void  FrameBuffer::create() {
		//attacment = new Attachment(attach_count);
		glGenFramebuffers(1, &frame_buffer_id);
	}
	void  FrameBuffer::use() {
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id);
	}
	//void FrameBuffer::generateAssmb() {
	//	/*use();
	//	attacment->assembAttachmentForFrame();
	//	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	//		std::cout << " Framebuffer : <<"<<frame_buffer_id<<"<< ,not complete!" << std::endl;
	//	reset();*/
	//} 

	void FrameBuffer::makeAttachment() {
		reset();
		use();
		 attachindexs = new unsigned int[attach_count];
		for (int i = 0; i < attach_count; i++) {
			attachs[i] =* new BufferAttacthment(i);
			if (i==0) {
				attachs[i].GenerateTexture(NEARE, NEARE);
			}
			else {
				attachs[i].GenerateTexture(NEARE, NEARE,CLAMP_TO_EDGE, CLAMP_TO_EDGE);
			}
			attachindexs[i] = attachments[i];
		}
		//glDrawBuffers(3, attachindexs);
	}
	void FrameBuffer::drawFrameBuffer() {
		glDrawBuffers(attach_count, attachindexs);
	}
	void FrameBuffer::reset() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}