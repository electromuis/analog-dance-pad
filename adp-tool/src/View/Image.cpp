#include "Image.h"
#include "Application.h"

//#include <windows.h>
extern "C" {
#include <GL/gl.h>
}

#include "imgui.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace Walnut {

	Image::Image(uint32_t width, uint32_t height, ImageFormat format, const void* data)
		: m_Width(width), m_Height(height), m_Format(format)
	{
		LoadImageData(data);
	}

	Image::~Image()
	{
		glDeleteTextures(1, &image_texture);
	}

	void Image::LoadImageData(const void* image_data)
	{
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef __EMSCRIPTEN__
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
#endif

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	}

}