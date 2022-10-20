#pragma once

#include <string>

typedef unsigned int GLuint;

namespace Walnut {

	enum class ImageFormat
	{
		None = 0,
		RGBA,
		RGBA32F
	};

	class Image
	{
	public:
		Image(std::string_view path);
		~Image();

		void SetData(const void* data);

		GLuint* GetDescriptorSet() { return &image_texture; }
		void LoadImageData(uint8_t* data);

		void Resize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
	private:
		void AllocateMemory(uint64_t size);
		void Release();
	private:
		uint32_t m_Width = 0, m_Height = 0;

		GLuint image_texture;

		ImageFormat m_Format = ImageFormat::None;
		size_t m_AlignedSize = 0;
		std::string m_Filepath;
	};

}



