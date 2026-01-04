
#ifndef __SCE__
// prevent linking problems when using ASAN
extern "C" {
#endif // __SCE__

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


#ifndef __SCE__
}
#endif // __SCE__
