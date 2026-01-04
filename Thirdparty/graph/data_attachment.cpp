#include "data_attachment.h"

namespace GRAPHT {


	void Attachment::assembAttachmentForFrame() {
		unsigned int* attachment_datas = new unsigned int[attchment_count];
		for (int i = 0; i < attchment_count; i++) {
			outputTextures[i].use();
			outputTextures[i].wrap_S = i == 0 ? CLAMP_TO_EDGE:REPEAT;
			outputTextures[i].wrap_T = i == 0 ? CLAMP_TO_EDGE:REPEAT;
			outputTextures[i].mag = NEARE;
			outputTextures[i].min = NEARE;
			outputTextures[i].srcWidth = SCR_WIDTH;
			outputTextures[i].srcHeight = SCR_HEIGHT;
			outputTextures[i].colorSpace = RGBA;
			outputTextures[i].generateTexture(NULL);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[i], GL_TEXTURE_2D, outputTextures[i].id, 0);
			attachment_datas[i] = attachments[i];
		}
		glDrawBuffers(attchment_count, attachment_datas);
	}



}