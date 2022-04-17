#include "vrpch.h"
#include "vrOpenGLBuffer.h"

#include <glad/glad.h>

namespace tinyvr {

	// =========================================================
	// ===================== VertexBuffer ======================
	// =========================================================

	vrOpenGLVertexBuffer::vrOpenGLVertexBuffer(float* vertices, uint32_t size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	vrOpenGLVertexBuffer::~vrOpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void vrOpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	}

	void vrOpenGLVertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// =========================================================
	// ====================== IndexBuffer ======================
	// =========================================================

	vrOpenGLIndexBuffer::vrOpenGLIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	vrOpenGLIndexBuffer::~vrOpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void vrOpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	}

	void vrOpenGLIndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

}