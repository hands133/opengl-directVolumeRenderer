#include "vrpch.h"
#include "vrOpenGLVertexArray.h"

#include <glad/glad.h>

namespace tinyvr {

	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case tinyvr::ShaderDataType::None:		return 0;
		case tinyvr::ShaderDataType::Float:		return GL_FLOAT;
		case tinyvr::ShaderDataType::Float2:	return GL_FLOAT;
		case tinyvr::ShaderDataType::Float3:	return GL_FLOAT;
		case tinyvr::ShaderDataType::Float4:	return GL_FLOAT;
		case tinyvr::ShaderDataType::Mat3:		return GL_FLOAT;
		case tinyvr::ShaderDataType::Mat4:		return GL_FLOAT;
		case tinyvr::ShaderDataType::Int:		return GL_INT;
		case tinyvr::ShaderDataType::Int2:		return GL_INT;
		case tinyvr::ShaderDataType::Int3:		return GL_INT;
		case tinyvr::ShaderDataType::Int4:		return GL_INT;
		case tinyvr::ShaderDataType::Bool:		return GL_BOOL;
		}
		//TINYVR_CORE_ASSERT(false, "Unknown ShaderDataType!");
		TINYVR_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	vrOpenGLVertexArray::vrOpenGLVertexArray()
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	vrOpenGLVertexArray::~vrOpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_RendererID);
	}

	void vrOpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_RendererID);
	}

	void vrOpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void vrOpenGLVertexArray::AddVertexBuffer(vrRef<vrVertexBuffer>& vertexBuffer)
	{
		//TINYVR_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Veretx Buffer has no layout!");
		TINYVR_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Veretx Buffer has no layout!");

		glBindVertexArray(m_RendererID);
		vertexBuffer->Bind();

		uint32_t index = 0;
		const auto& layout = vertexBuffer->GetLayout();
		for (const auto& e : layout)
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, e.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(e.Type),
				e.Normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				reinterpret_cast<const void*>(e.Offset));
			++index;
		}

		m_VertexBuffers.push_back(vertexBuffer);
	}

	void vrOpenGLVertexArray::SetIndexBuffer(vrRef<vrIndexBuffer>& indexBuffer)
	{
		glBindVertexArray(m_RendererID);
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}
}