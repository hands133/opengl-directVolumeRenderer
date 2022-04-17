#pragma once

#include "vrTexture.h"

namespace tinyvr {

	enum class ShaderDataType : uint8_t
	{
		None = 0, Float, Float2, Float3, Float4,
		Mat3, Mat4, Int, Int2, Int3, Int4,
		Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case tinyvr::ShaderDataType::None:		return 0;
		case tinyvr::ShaderDataType::Float:		return 4;
		case tinyvr::ShaderDataType::Float2:	return 4 * 2;
		case tinyvr::ShaderDataType::Float3:	return 4 * 3;
		case tinyvr::ShaderDataType::Float4:	return 4 * 4;
		case tinyvr::ShaderDataType::Mat3:		return 4 * 3 * 3;
		case tinyvr::ShaderDataType::Mat4:		return 4 * 4 * 4;
		case tinyvr::ShaderDataType::Int:		return 4;
		case tinyvr::ShaderDataType::Int2:		return 4 * 2;
		case tinyvr::ShaderDataType::Int3:		return 4 * 3;
		case tinyvr::ShaderDataType::Int4:		return 4 * 4;
		case tinyvr::ShaderDataType::Bool:		return 1;
		}

		//HZ_CORE_ASSERT(false, "Unknown ShaderDataType!");
		TINYVR_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	struct vrBufferElement
	{
		std::string Name;
		ShaderDataType Type;
		uint32_t Offset;
		uint32_t Size;
		bool Normalized;

		vrBufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case tinyvr::ShaderDataType::None:		return 0;
			case tinyvr::ShaderDataType::Float:		return 1;
			case tinyvr::ShaderDataType::Float2:	return 2;
			case tinyvr::ShaderDataType::Float3:	return 3;
			case tinyvr::ShaderDataType::Float4:	return 4;
			case tinyvr::ShaderDataType::Mat3:		return 3 * 3;
			case tinyvr::ShaderDataType::Mat4:		return 4 * 4;
			case tinyvr::ShaderDataType::Int:		return 1;
			case tinyvr::ShaderDataType::Int2:		return 2;
			case tinyvr::ShaderDataType::Int3:		return 3;
			case tinyvr::ShaderDataType::Int4:		return 4;
			case tinyvr::ShaderDataType::Bool:		return 1;
			}

			//HZ_CORE_ASSERT(false, "Unknwon ShaderDataType!");
			TINYVR_ASSERT(false, "Unknwon ShaderDataType!");
			return 0;
		}
	};

	class vrBufferLayout
	{
	public:

		vrBufferLayout() = default;

		vrBufferLayout(const std::initializer_list<vrBufferElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		};

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<vrBufferElement>& GetElements() const { return m_Elements; }

		std::vector<vrBufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<vrBufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<vrBufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<vrBufferElement>::const_iterator end() const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride()
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<vrBufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	class vrVertexBuffer
	{
	public:
		virtual ~vrVertexBuffer() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const vrBufferLayout& GetLayout() const = 0;
		virtual void SetLayout(const vrBufferLayout& layout) = 0;

		static vrRef<vrVertexBuffer> Create(float* vertices, uint32_t size);
	};

	class vrIndexBuffer
	{
	public:
		virtual ~vrIndexBuffer() {}


		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32_t GetCount() const = 0;

		static vrRef<vrIndexBuffer> Create(uint32_t* indices, uint32_t size);
	};
}
