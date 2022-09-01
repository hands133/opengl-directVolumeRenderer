#include "vrpch.h"
#include "vrOpenGLTexture.h"

#include <glad/glad.h>

namespace tinyvr {
    vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_1D> vrOpenGLTexture1D::s_GLImpl;
   
    vrOpenGLTexture1D::vrOpenGLTexture1D(uint32_t width, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
        : m_LVL(0), m_Width(width), m_Format(format), m_Type(type)
    {
        s_GLImpl.CreateTexture(m_TextureID);
        s_GLImpl.InitTextureProperty(m_TextureID, m_LVL);
    }

    vrRef<vrOpenGLTexture1D> vrOpenGLTexture1D::CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
    {
        return CreateRef<vrOpenGLTexture1D>(res.x, format, type);
    }

    void vrOpenGLTexture1D::SetData(uint32_t items, const void* data)
    {
        if(data)
            TINYVR_CORE_ASSERT(items >= m_Width, "Buffer size {0}, less than texture size {1}", items, m_Width);
        s_GLImpl.SetData({ m_Width, 1, 1 }, m_TextureID, m_Format, m_LVL, m_Type, 0, data);
    }

    void vrOpenGLTexture1D::BindImage(uint32_t idx, vrTexImageAccess access) const
    {
        s_GLImpl.BindImage(idx, m_TextureID, m_Format, m_Type, access);
    }

    glm::vec2 vrOpenGLTexture1D::GlobalMinMaxVal() const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, don't support other format!");
        return s_GLImpl.GlobalMinMax(m_TextureID, m_Format, m_Type, m_LVL, m_Width);
    }

    std::vector<float> vrOpenGLTexture1D::GetHistogram(uint32_t NumIntervals) const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, don't support other format!");
        return s_GLImpl.GlobalHistogram(m_TextureID, m_Format, m_Type, m_LVL, m_Width, NumIntervals);
    }

    bool vrOpenGLTexture1D::GetData(void* buffer) const
    {
        s_GLImpl.FetchTexture(m_TextureID, m_Format, m_Type, m_LVL, buffer);
        return true;
    }

    void vrOpenGLTexture1D::Resize(uint32_t width)
    {
        m_Width = width;

        if (m_TextureID)
        {
            s_GLImpl.DestroyTexture(m_TextureID);
            m_TextureID = 0;
        }
        s_GLImpl.CreateTexture(m_TextureID);
        s_GLImpl.InitTextureProperty(m_TextureID, m_LVL);
        s_GLImpl.SetData({ m_Width, 1, 1 }, m_TextureID, m_Format, m_LVL, m_Type, 0, nullptr);
    }
}

namespace tinyvr {

    vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_2D> vrOpenGLTexture2D::s_GLImpl;

    vrOpenGLTexture2D::vrOpenGLTexture2D(
        uint32_t width, uint32_t height, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
        : m_LVL(0), m_Width(width), m_Height(height), m_Format(format), m_Type(type)
    {
        s_GLImpl.CreateTexture(m_TextureID);
        s_GLImpl.InitTextureProperty(m_TextureID, m_LVL);
    }

    vrRef<vrOpenGLTexture2D> vrOpenGLTexture2D::CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
    {
        return CreateRef<vrOpenGLTexture2D>(res.x, res.y, format, type);
    }

    void vrOpenGLTexture2D::SetData(uint32_t items, const void* data)
    {
        if (data)
            TINYVR_CORE_ASSERT(items >= m_Width * m_Height, "Buffer size {0}, less than texture size {1}", items, m_Width * m_Height);
        s_GLImpl.SetData({ m_Width, m_Height, 1 }, m_TextureID, m_Format, m_LVL, m_Type, 0, data);
    }

    void vrOpenGLTexture2D::BindImage(uint32_t idx, vrTexImageAccess access) const
    {
        s_GLImpl.BindImage(idx, m_TextureID, m_Format, m_Type, access);
    }

    glm::vec2 vrOpenGLTexture2D::GlobalMinMaxVal() const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, don't support other format!");
        return s_GLImpl.GlobalMinMax(m_TextureID, m_Format, m_Type, m_LVL, m_Width * m_Height);
    }

    std::vector<float> vrOpenGLTexture2D::GetHistogram(uint32_t NumIntervals) const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, don't support other format!");
        return s_GLImpl.GlobalHistogram(m_TextureID, m_Format, m_Type, m_LVL, m_Width * m_Height, NumIntervals);
    }

    bool vrOpenGLTexture2D::GetData(void* buffer) const
    {
        s_GLImpl.FetchTexture(m_TextureID, m_Format, m_Type, m_LVL, buffer);
        return true;
    }

    void vrOpenGLTexture2D::Resize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        if (m_TextureID) {
            s_GLImpl.DestroyTexture(m_TextureID);
            m_TextureID = 0;
        }
        s_GLImpl.CreateTexture(m_TextureID);
        s_GLImpl.InitTextureProperty(m_TextureID, m_LVL);
        s_GLImpl.SetData({ m_Width, m_Height, 1 }, m_TextureID, m_Format, m_LVL, m_Type, 0, nullptr);
    }
}

namespace tinyvr {

    vrOpenGLTextureImpl<vrTextureDim::TEXTURE_DIM_3D> vrOpenGLTexture3D::s_GLImpl;
    
    vrOpenGLTexture3D::vrOpenGLTexture3D(
        uint32_t width, uint32_t height, uint32_t depth, 
        tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
        : m_LVL(0), m_Width(width), m_Height(height), m_Depth(depth), m_Format(format), m_Type(type)
    {
        s_GLImpl.CreateTexture(m_TextureID);
        s_GLImpl.InitTextureProperty(m_TextureID, m_LVL);
    }

    vrRef<vrOpenGLTexture3D> vrOpenGLTexture3D::CreateTexture(const glm::uvec3& res, tinyvr::vrTextureFormat format, tinyvr::vrTextureType type)
    {
        return CreateRef<vrOpenGLTexture3D>(res.x, res.y, res.z, format, type);
    }

    void vrOpenGLTexture3D::SetData(uint32_t items, const void* data)
    {
        if (data)
            TINYVR_CORE_ASSERT(items >= m_Width * m_Height * m_Depth, "Buffer size {0}, less than texture size {1}", items, m_Width * m_Height * m_Depth);
        s_GLImpl.SetData({ m_Width, m_Height, m_Depth }, m_TextureID, m_Format, m_LVL, m_Type, 0, data);
    }

    void vrOpenGLTexture3D::BindImage(uint32_t idx, vrTexImageAccess access) const
    {
        s_GLImpl.BindImage(idx, m_TextureID, m_Format, m_Type, access);
    }

    glm::vec2 vrOpenGLTexture3D::GlobalMinMaxVal() const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, support single channel texture only!");
        return s_GLImpl.GlobalMinMax(m_TextureID, m_Format, m_Type, m_LVL, m_Width * m_Height * m_Depth);
    }

    std::vector<float> vrOpenGLTexture3D::GetHistogram(uint32_t NumIntervals) const
    {
        TINYVR_ASSERT(m_Format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, don't support other format!");
        return s_GLImpl.GlobalHistogram(m_TextureID, m_Format, m_Type, m_LVL, m_Width * m_Height * m_Depth, NumIntervals);
    }

    bool vrOpenGLTexture3D::GetData(void* buffer) const
    {
        s_GLImpl.FetchTexture(m_TextureID, m_Format, m_Type, m_LVL, buffer);
        return true;
    }
}