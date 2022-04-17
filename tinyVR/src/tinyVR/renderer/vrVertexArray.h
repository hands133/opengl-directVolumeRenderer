#pragma once

#include <memory>
#include "tinyVR/renderer/vrBuffer.h"

namespace tinyvr {

	class vrVertexArray
	{
	public:
		virtual ~vrVertexArray() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(vrRef<vrVertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(vrRef<vrIndexBuffer>& indexBuffer) = 0;

		virtual const std::vector<vrRef<vrVertexBuffer>>& GetVertexBuffers() const = 0;
		virtual const vrRef<vrIndexBuffer>& GetIndexBuffer() const = 0;

		static vrRef<vrVertexArray> Create();
	};

}