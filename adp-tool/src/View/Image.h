#pragma once

#include <string>
#include <cstdint>

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
		Image(uint32_t width, uint32_t height, ImageFormat format, const void* data = nullptr);
		~Image();

		void SetData(const void* data);

		void* GetDescriptorSet() { return (void*)(intptr_t)image_texture; }
		void LoadImageData(const void* data);

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



