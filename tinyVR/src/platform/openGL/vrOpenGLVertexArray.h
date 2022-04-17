#pragma once

#include "tinyVR/renderer/vrVertexArray.h"

namespace tinyvr {

	class vrOpenGLVertexArray : public vrVertexArray
	{
	public:
		vrOpenGLVertexArray();
		virtual ~vrOpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(vrRef<vrVertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(vrRef<vrIndexBuffer>& indexBuffer) override;

		virtual const std::vector<vrRef<vrVertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		virtual const vrRef<vrIndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		uint32_t m_RendererID;
		std::vector<vrRef<vrVertexBuffer>> m_VertexBuffers;
		vrRef<vrIndexBuffer> m_IndexBuffer;
	};
}
