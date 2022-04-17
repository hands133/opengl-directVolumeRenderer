#pragma once

#include "tinyVR/renderer/vrBuffer.h"

namespace tinyvr {

	class vrOpenGLVertexBuffer : public vrVertexBuffer
	{
	public:
		vrOpenGLVertexBuffer(float* vertices, uint32_t size);
		virtual ~vrOpenGLVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const vrBufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const vrBufferLayout& layout) override { m_Layout = layout; }

	private:
		uint32_t m_RendererID;
		vrBufferLayout m_Layout;
	};

	class vrOpenGLIndexBuffer : public vrIndexBuffer
	{
	public:
		vrOpenGLIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~vrOpenGLIndexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual uint32_t GetCount() const override { return m_Count; }

	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};

}