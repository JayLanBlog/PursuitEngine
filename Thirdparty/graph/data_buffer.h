#pragma once
#include <graph/data_attachment.h>

namespace GRAPHT {

	

	class VertexBuffer {
	public:
		/*
			float pos[]{x,y,z}  float2/float3
			float normal[]{x,y,z} float2/float 3
			float texture[]{x,y} float2
			flaot tangent[] {x,y,z} float3
		*/
		VertexBuffer();
		int data_size = 0;//size of f
		int data_count = 0;
		virtual void upload_data(void* data ,int size,int count)=0;
		int getBufferSize();
		int getStradeSize();
		virtual void bind() = 0;
		// address of data	
		void* data = nullptr;

	};

	class GLVBO:public VertexBuffer
	{
	public:
		GLVBO();
		~GLVBO();
		unsigned int VAO ,VBO; 
		virtual void upload_data(void* data, int size, int count);
		virtual void bind() ;
	};



	class IndexBuffer {
	public:
		IndexBuffer();
		virtual void upload_data(void* data, int size, int count)=0;
		virtual void bind() = 0;
		int getBufferSize();
		int getCount();
		// address of data	
		void* data = nullptr;
		int data_size = 0;//size of f
		int data_count = 0;
	};
	class GLIBO :public IndexBuffer {
	public:
		GLIBO();
		~GLIBO();
		unsigned int EBO = -1;
		virtual void upload_data(void* data, int size, int count);
		virtual void bind();

	};

	class FrameBuffer {
	public:
		FrameBuffer() {
			
			attach_count = 1;
			attachs = new BufferAttacthment[attach_count];
			create();
		}

		FrameBuffer(int attactCount) {
			attach_count = attactCount;
			attachs = new BufferAttacthment[attach_count];
			create();
		}

		void create();
		void use();
		//void generateAssmb();
		void makeAttachment();
		void drawFrameBuffer();
		//void makeDepthBuffer();
		void reset();
		unsigned int frame_buffer_id;
		BufferAttacthment* attachs;
		unsigned int* attachindexs;
		int attach_count;
	};

	class RenderBuffer {
	public:
		RenderBuffer() {
			create();
		}
		void create() {
			glGenRenderbuffers(1, &render_buffer_id);
		}
		void use() {
			glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id);
		}
		void renderToBuffer(unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT) {
			use();
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_id);
			// finally check if framebuffer is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		unsigned int render_buffer_id = -1;

	};
}