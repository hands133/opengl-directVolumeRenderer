#pragma once

#include <functional>
#include <map>

#include <glm/glm.hpp>

#include "tinyVR/renderer/vrTexture.h"
#include "vrOpenGLTextureImpl.hpp"

namespace tinyvr {

    class vrOpenGLTexture1D : public vrTexture1D
    {
    public:
        vrOpenGLTexture1D(uint32_t width, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        static vrRef<vrOpenGLTexture1D> CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetTextureID() const override { return m_TextureID; }
        uint32_t GetLevel() const override { return m_LVL; }

        uint32_t GetTexLayout() const { return s_GLImpl.GetLayout(m_Format, m_Type).LAYOUT; }

        void SetLayout(vrTextureFormat format, vrTextureType type) override
        {
            m_Format = format;
            m_Type = type;
        }
        void SetData(uint32_t items = 0, const void* data = nullptr) override;

        void Bind() const override { s_GLImpl.Bind(m_TextureID); }
        void BindUnit(uint32_t idx) const override { s_GLImpl.BindUnit(idx, m_TextureID); }
        void BindImage(uint32_t idx, vrTexImageAccess access) const override;

        glm::vec2 GlobalMinMaxVal() const override;
        std::vector<float> GetHistogram(uint32_t NumIntervals) const override;
        bool GetData(void* buffer) const override;

        void Resize(uint32_t width) override;

    private:
        GLuint m_TextureID;

        // property
        GLuint m_LVL;
        uint32_t m_Width;

        vrTextureFormat m_Format;
        vrTextureType m_Type;

        static vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_1D> s_GLImpl;
    };

    class vrOpenGLTexture2D : public vrTexture2D
    {
    public:
        vrOpenGLTexture2D(uint32_t width, uint32_t height, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        static vrRef<vrOpenGLTexture2D> CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetLevel() const override { return m_LVL; }
        uint32_t GetTextureID() const override { return m_TextureID; }

        uint32_t GetTexLayout() const { return s_GLImpl.GetLayout(m_Format, m_Type).LAYOUT; }

        void SetLayout(vrTextureFormat format, vrTextureType type) override
        {
            m_Format = format;
            m_Type = type;
        }
        void SetData(uint32_t items = 0, const void* data = nullptr) override;

        void Bind() const override { s_GLImpl.Bind(m_TextureID); }
        void BindUnit(uint32_t idx) const override { s_GLImpl.BindUnit(idx, m_TextureID); }
        void BindImage(uint32_t idx, vrTexImageAccess access) const override;

        glm::vec2 GlobalMinMaxVal() const override;
        std::vector<float> GetHistogram(uint32_t NumIntervals) const override;
        bool GetData(void* buffer) const override;

        void Resize(uint32_t width, uint32_t height) override;

    private:
        GLuint m_TextureID;

        // property
        GLuint m_LVL;
        uint32_t m_Width, m_Height;

        vrTextureFormat m_Format;
        vrTextureType m_Type;

        static vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_2D> s_GLImpl;
    };

    class vrOpenGLTexture3D : public vrTexture3D
    {
    public:
        vrOpenGLTexture3D(uint32_t width, uint32_t height, uint32_t depth, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        static vrRef<vrOpenGLTexture3D> CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type);

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetDepth() const override { return m_Depth; }
        uint32_t GetLevel() const override { return m_LVL; }
        uint32_t GetTextureID() const override { return m_TextureID; }

        uint32_t GetTexLayout() const { return s_GLImpl.GetLayout(m_Format, m_Type).LAYOUT; }

        void SetLayout(vrTextureFormat format, vrTextureType type) override
        {
            m_Format = format;
            m_Type = type;
        }
        void SetData(uint32_t items = 0, const void* data = nullptr) override;

        void Bind() const override { s_GLImpl.Bind(m_TextureID); }
        void BindUnit(uint32_t idx) const override { s_GLImpl.BindUnit(idx, m_TextureID); }
        void BindImage(uint32_t idx, vrTexImageAccess access) const override;

        glm::vec2 GlobalMinMaxVal() const override;
        std::vector<float> GetHistogram(uint32_t NumIntervals) const override;
        bool GetData(void* buffer) const override;

    private:
        GLuint m_TextureID;

        // property
        GLuint m_LVL;
        uint32_t m_Width, m_Height, m_Depth;

        vrTextureFormat m_Format;
        vrTextureType m_Type;

        static vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_3D> s_GLImpl;
    };
}