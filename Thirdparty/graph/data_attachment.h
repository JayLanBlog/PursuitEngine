#pragma once
#include "graph/graph_def.h"
#include "data/texture/Texture.h"
namespace GRAPHT {
	static const GLenum attachments[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4
	};

	class Attachment {
	public:
		int attchment_count = 0;
	
		vector<GLTexture> outputTextures;

		int SCR_WIDTH = 800;
		int SCR_HEIGHT = 600;
		Attachment() {
			attchment_count = 1;
			createOutputTextures();
		}

		Attachment(int count) {
			attchment_count = count;
			createOutputTextures();
		}
		void createOutputTextures() {
			for (int i = 0; i < attchment_count; i++) {
				GLTexture texture;
				texture.createTexture();
				outputTextures.push_back(texture);
			}
		}

		void assembAttachmentForFrame();
	

	};


	class  BufferAttacthment{
	public:
		unsigned int width = 800, height = 600;
		int attachmentIndex = 0;
		GLTexture outputTexture;
		BufferAttacthment() = default;
		BufferAttacthment(int attachement) :attachmentIndex(attachement) {
			outputTexture.createTexture();
		}
		void GenerateTexture(Filter min, Filter mag,Wrap wrap_S = REPEAT, Wrap wrap_T= REPEAT) {
			outputTexture.wrap_S = wrap_S;
			outputTexture.wrap_T = wrap_T;
			outputTexture.min = min;
			outputTexture.mag = mag;
			if (attachmentIndex <2) {
				outputTexture.dataType = FLOAT;
			}
			else
			{
				outputTexture.dataType = UNSIGNED_BYTE;
			}
			outputTexture.srcWidth = width;
			outputTexture.srcHeight = height;
			outputTexture.generateTexture(nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[attachmentIndex], GL_TEXTURE_2D, outputTexture.id, 0);
		}
	};

}