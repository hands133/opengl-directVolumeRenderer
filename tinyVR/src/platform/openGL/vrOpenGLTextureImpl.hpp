#pragma once

#include "tinyVR/core/vrBase.h"
#include "tinyVR/renderer/vrTexture.h"

#include "tinyVR/core/vrUtil.hpp"
#include "vrOpenGLErrorHandle.h"
#include "vrOpenGLTextureImplUtil.h"

#include <glad/glad.h>
#include <map>

namespace tinyvr {

    template <vrTextureDim dim>
    class vrOpenGLTextureImpl
    {
    public:

        struct GL_PAIRU3
        {
            uint32_t FMT;
            uint32_t TYPE;
            uint32_t LAYOUT;
        };

        void CreateTexture(GLuint& texID);
        void DestroyTexture(GLuint& texID);

        static GL_PAIRU3 GetLayout(vrTextureFormat format, vrTextureType type);

        static GLuint GetDimFromEnum();
        static GLuint GetBorderFromEnum(vrTextureBorder border);
        static GLuint GetInterpFromEnum(vrTextureInterp interp);

        static void Bind(GLuint texID);

        static void BindUnit(uint32_t idx, GLuint texID);
        static void BindImage(uint32_t idx, GLuint texID,
            vrTextureFormat format, vrTextureType type, vrTexImageAccess access);

        static void InitTextureProperty(
            GLuint texID, GLuint lvl = 0,
            vrTextureBorder border = vrTextureBorder::TEXTURE_BORDER_CLAMP_EDGE,
            vrTextureInterp interp = vrTextureInterp::TEXTURE_INTERP_LINEAR, bool genMipMap = false);

        static void SetData(
            glm::uvec3 res, GLuint texID, vrTextureFormat format, GLuint lvl,
            vrTextureType type, GLuint border = 0, const void* data = nullptr);

        static void FetchTexture(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, void* bufferPtr);
        static glm::vec2 GlobalMinMax(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, uint32_t items);
        static std::vector<float> GlobalHistogram(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, uint32_t items, uint32_t NumIntervals);

    private:
        static std::map<std::pair<vrTextureFormat, vrTextureType>, GL_PAIRU3> m_TexLayoutMap;

        // for select texture data
        template <vrTextureDim dim>
        struct __TexDimOPSelector {};

        template <>
        struct __TexDimOPSelector<vrTextureDim::TEXTURE_DIM_1D>
        {
            void SetTextureData(
                GLuint dimParam, GLuint lvl, glm::uvec3 res, GLuint borderParam,
                vrOpenGLTextureImpl::GL_PAIRU3 layoutParam, const void* data)
            {   glTexImage1D(dimParam, lvl, layoutParam.LAYOUT, res.x, borderParam, layoutParam.FMT, layoutParam.TYPE, data);    }

            void SetBorderParam(GLuint dimParam, GLuint borderParam)
            {   glTexParameteri(dimParam, GL_TEXTURE_WRAP_S, borderParam);  }
        };

        template<>
        struct __TexDimOPSelector<vrTextureDim::TEXTURE_DIM_2D>
        {
            void SetTextureData(
                GLuint dimParam, GLuint lvl, glm::uvec3 res, GLuint borderParam,
                vrOpenGLTextureImpl::GL_PAIRU3 layoutParam, const void* data)
            {   glTexImage2D(dimParam, lvl, layoutParam.LAYOUT, res.x, res.y, borderParam, layoutParam.FMT, layoutParam.TYPE, data);     }

            void SetBorderParam(GLuint dimParam, GLuint borderParam)
            {
                glTexParameteri(dimParam, GL_TEXTURE_WRAP_S, borderParam);
                glTexParameteri(dimParam, GL_TEXTURE_WRAP_T, borderParam);
            }
        };

        template<>
        struct __TexDimOPSelector<vrTextureDim::TEXTURE_DIM_3D>
        {
            void SetTextureData(
                GLuint dimParam, GLuint lvl, glm::uvec3 res, GLuint borderParam,
                vrOpenGLTextureImpl::GL_PAIRU3 layoutParam, const void* data)
            {   glTexImage3D(dimParam, lvl, layoutParam.LAYOUT, res.x, res.y, res.z, borderParam, layoutParam.FMT, layoutParam.TYPE, data);  }

            void SetBorderParam(GLuint dimParam, GLuint borderParam)
            {
                glTexParameteri(dimParam, GL_TEXTURE_WRAP_S, borderParam);
                glTexParameteri(dimParam, GL_TEXTURE_WRAP_T, borderParam);
                glTexParameteri(dimParam, GL_TEXTURE_WRAP_R, borderParam);
            }
        };
	};

    template <vrTextureDim dim>
    inline void vrOpenGLTextureImpl<dim>::SetData(
        glm::uvec3 res, GLuint texID, vrTextureFormat format, GLuint lvl,
        vrTextureType type, GLuint border, const void* data)
    {
        Bind(texID);

        auto dimParam = GetDimFromEnum();
        auto layoutParam = m_TexLayoutMap.find({ format, type })->second;

        __TexDimOPSelector<dim> __selector;
        __selector.SetTextureData(dimParam, lvl, res, border, layoutParam, data);
    }

    template<vrTextureDim dim>
    inline void vrOpenGLTextureImpl<dim>::FetchTexture(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, void* bufferPtr)
    {
        Bind(texID);
        auto layoutParam = m_TexLayoutMap.find({ format, type })->second;
        glGetTexImage(GetDimFromEnum(), lvl, layoutParam.FMT, layoutParam.TYPE, bufferPtr);
    }

    template<vrTextureDim dim>
    inline glm::vec2 vrOpenGLTextureImpl<dim>::GlobalMinMax(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, uint32_t items)
    {
        glm::vec2 minmax(0.0f, 0.0f);
        int formatScales = 1;
        int typeBytes = 1;

        switch (format)
        {
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RED:      formatScales = 1;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RG:       formatScales = 2;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RGB:      formatScales = 3;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA:     formatScales = 4;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_DEPTH:    formatScales = 1;   break;
        }

        switch (type)
        {
        case tinyvr::vrTextureType::TEXTURE_TYPE_U8I:       typeBytes = 1;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_U16I:      typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_U32I:      typeBytes = 4;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_8I:        typeBytes = 1;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_16I:       typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_32I:       typeBytes = 4;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_FLT16:     typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_FLT32:     typeBytes = 4;      break;
        }

        // NOTE: ASSUME the element is all r
        uint32_t N = items * formatScales * typeBytes;
        void* buffer = new uint8_t[N];
        memset(buffer, 0, N);

        FetchTexture(texID, format, type, lvl, buffer);

        minmax = getMinMax(type, format, buffer, items);

        delete[] buffer;
        return minmax;
    }

    template<vrTextureDim dim>
    inline std::vector<float> vrOpenGLTextureImpl<dim>::GlobalHistogram(GLuint texID, vrTextureFormat format, vrTextureType type, GLuint lvl, uint32_t items, uint32_t NumIntervals)
    {
        std::vector<float> hist;

        int formatScales = 1;
        int typeBytes = 1;

        switch (format)
        {
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RED:      formatScales = 1;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RG:       formatScales = 2;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RGB:      formatScales = 3;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA:     formatScales = 4;   break;
        case tinyvr::vrTextureFormat::TEXTURE_FMT_DEPTH:    formatScales = 1;   break;
        }

        switch (type)
        {
        case tinyvr::vrTextureType::TEXTURE_TYPE_U8I:       typeBytes = 1;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_U16I:      typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_U32I:      typeBytes = 4;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_8I:        typeBytes = 1;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_16I:       typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_32I:       typeBytes = 4;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_FLT16:     typeBytes = 2;      break;
        case tinyvr::vrTextureType::TEXTURE_TYPE_FLT32:     typeBytes = 4;      break;
        }

        // NOTE: ASSUME the element is all r
        uint32_t N = items * formatScales * typeBytes;
        void* buffer = new uint8_t[N];
        memset(buffer, 0, N);

        FetchTexture(texID, format, type, lvl, buffer);
        hist = getHistogram(type, format, buffer, items, NumIntervals);

        delete[] buffer;

        return hist;
    }

    // { FMT; TYPE; LAYOUT }
    template <vrTextureDim dim>
    std::map<std::pair<vrTextureFormat, vrTextureType>, typename vrOpenGLTextureImpl<dim>::GL_PAIRU3> vrOpenGLTextureImpl<dim>::m_TexLayoutMap =
    {
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_U8I },      { GL_RED_INTEGER,       GL_UNSIGNED_BYTE,   GL_R8UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_U16I },     { GL_RED_INTEGER,       GL_UNSIGNED_SHORT,  GL_R16UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_U32I },     { GL_RED_INTEGER,       GL_UNSIGNED_INT,    GL_R32UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_8I },       { GL_RED_INTEGER,       GL_BYTE,            GL_R8I }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_16I },      { GL_RED_INTEGER,       GL_SHORT,           GL_R16I }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_32I },      { GL_RED_INTEGER,       GL_INT,             GL_R32I }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_FLT16 },    { GL_RED,               GL_HALF_FLOAT,      GL_R16F }},
        {{ vrTextureFormat::TEXTURE_FMT_RED,    vrTextureType::TEXTURE_TYPE_FLT32 },    { GL_RED,               GL_FLOAT,           GL_R32F }},

        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_U8I },      { GL_RG_INTEGER,        GL_UNSIGNED_BYTE,   GL_RG8UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_U16I },     { GL_RG_INTEGER,        GL_UNSIGNED_SHORT,  GL_RG16UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_U32I },     { GL_RG_INTEGER,        GL_UNSIGNED_INT,    GL_RG32UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_8I },       { GL_RG_INTEGER,        GL_BYTE,            GL_RG8I }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_16I },      { GL_RG_INTEGER,        GL_SHORT,           GL_RG16I }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_32I },      { GL_RG_INTEGER,        GL_INT,             GL_RG32I }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_FLT16 },    { GL_RG,                GL_HALF_FLOAT,      GL_RG16F }},
        {{ vrTextureFormat::TEXTURE_FMT_RG,     vrTextureType::TEXTURE_TYPE_FLT32 },    { GL_RG,                GL_FLOAT,           GL_RG32F }},

        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_U8I },      { GL_RGB_INTEGER,       GL_UNSIGNED_BYTE,   GL_RGB8UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_U16I },     { GL_RGB_INTEGER,       GL_UNSIGNED_SHORT,  GL_RGB16UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_U32I },     { GL_RGB_INTEGER,       GL_UNSIGNED_INT,    GL_RGB32UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_8I },       { GL_RGB_INTEGER,       GL_BYTE,            GL_RGB8I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_16I },      { GL_RGB_INTEGER,       GL_SHORT,           GL_RGB16I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_32I },      { GL_RGB_INTEGER,       GL_INT,             GL_RGB32I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_FLT16 },    { GL_RGB,               GL_HALF_FLOAT,      GL_RGB16F }},
        {{ vrTextureFormat::TEXTURE_FMT_RGB,    vrTextureType::TEXTURE_TYPE_FLT32 },    { GL_RGB,               GL_FLOAT,           GL_RGB32F }},

        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_U8I },      { GL_RGBA_INTEGER,      GL_UNSIGNED_BYTE,   GL_RGBA8UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_U16I },     { GL_RGBA_INTEGER,      GL_UNSIGNED_SHORT,  GL_RGBA16UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_U32I },     { GL_RGBA_INTEGER,      GL_UNSIGNED_INT,    GL_RGBA32UI }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_8I },       { GL_RGBA_INTEGER,      GL_BYTE,            GL_RGBA8I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_16I },      { GL_RGBA_INTEGER,      GL_SHORT,           GL_RGBA16I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_32I },      { GL_RGBA_INTEGER,      GL_INT,             GL_RGBA32I }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_FLT16 },    { GL_RGBA,              GL_HALF_FLOAT,      GL_RGBA16F }},
        {{ vrTextureFormat::TEXTURE_FMT_RGBA,   vrTextureType::TEXTURE_TYPE_FLT32 },    { GL_RGBA,              GL_FLOAT,           GL_RGBA32F }},

        {{ vrTextureFormat::TEXTURE_FMT_DEPTH,  vrTextureType::TEXTURE_TYPE_FLT32 },    { GL_DEPTH_COMPONENT,   GL_FLOAT,           GL_DEPTH_COMPONENT32F }},
    };

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::CreateTexture(GLuint& textureID)
    {
        glGenTextures(1, &textureID);
    }

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::DestroyTexture(GLuint& textureID)
    {
        glDeleteTextures(1, &textureID);
    }

    template <vrTextureDim dim>
    typename vrOpenGLTextureImpl<dim>::GL_PAIRU3 vrOpenGLTextureImpl<dim>::GetLayout(vrTextureFormat format, vrTextureType type)
    {
        auto p3 = m_TexLayoutMap.find({ format, type });
        TINYVR_CORE_ASSERT(p3 != m_TexLayoutMap.end(), "Key value Not Find!");

        return p3->second;
    }

    template <vrTextureDim dim>
    GLuint vrOpenGLTextureImpl<dim>::GetDimFromEnum()
    {
        GLuint dimParam = GL_TEXTURE_2D;
        switch (dim)
        {
        case tinyvr::vrTextureDim::TEXTURE_DIM_1D:      dimParam = GL_TEXTURE_1D;   break;
        case tinyvr::vrTextureDim::TEXTURE_DIM_2D:      dimParam = GL_TEXTURE_2D;   break;
        case tinyvr::vrTextureDim::TEXTURE_DIM_3D:      dimParam = GL_TEXTURE_3D;   break;
        }
        return dimParam;
    }

    template <vrTextureDim dim>
    GLuint vrOpenGLTextureImpl<dim>::GetBorderFromEnum(vrTextureBorder border)
    {
        GLuint borderParam = GL_CLAMP_TO_EDGE;
        switch (border)
        {
        case tinyvr::vrTextureBorder::TEXTURE_BORDER_CLAMP:         borderParam = GL_CLAMP;             break;
        case tinyvr::vrTextureBorder::TEXTURE_BORDER_CLAMP_EDGE:    borderParam = GL_CLAMP_TO_EDGE;     break;
        case tinyvr::vrTextureBorder::TEXTURE_BORDER_CLAMP_BORDER:  borderParam = GL_CLAMP_TO_BORDER;   break;
        case tinyvr::vrTextureBorder::TEXTURE_BORDER_REPEAT:        borderParam = GL_REPEAT;            break;
        case tinyvr::vrTextureBorder::TEXTURE_BORDER_MIRROR_REPEAT: borderParam = GL_MIRRORED_REPEAT;   break;
        }
        return borderParam;
    }

    template <vrTextureDim dim>
    GLuint vrOpenGLTextureImpl<dim>::GetInterpFromEnum(vrTextureInterp interp)
    {
        GLuint filterParam = GL_LINEAR;
        switch (interp)
        {
        case tinyvr::vrTextureInterp::TEXTURE_INTERP_LINEAR:    filterParam = GL_LINEAR;    break;
        case tinyvr::vrTextureInterp::TEXTURE_INTERP_NEAREST:   filterParam = GL_NEAREST;   break;
        }
        return filterParam;
    }

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::Bind(GLuint texID)
    {
        glBindTexture(GetDimFromEnum(), texID);
    }

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::BindUnit(uint32_t idx, GLuint texID)
    {
        Bind(texID);
        glBindTextureUnit(idx, texID);
    }

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::BindImage(uint32_t idx, GLuint texID,
        vrTextureFormat format, vrTextureType type, vrTexImageAccess access)
    {
        Bind(texID);

        auto dimParam = GetDimFromEnum();
        GLboolean layered = (dimParam == GL_TEXTURE_3D ? GL_TRUE : GL_FALSE);
        GLenum accParam = GL_READ_ONLY;

        switch (access)
        {
        case tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READONLY:   accParam = GL_READ_ONLY;      break;
        case tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY:  accParam = GL_WRITE_ONLY;     break;
        case tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE:  accParam = GL_READ_WRITE;     break;
        }

        glBindImageTexture(idx, texID, 0, layered, 0, accParam, GetLayout(format, type).LAYOUT);
    }

    template <vrTextureDim dim>
    void vrOpenGLTextureImpl<dim>::InitTextureProperty(
        GLuint texID, GLuint lvl, vrTextureBorder border, vrTextureInterp interp, bool genMipMap)
    {
        Bind(texID);

        auto dimParam = GetDimFromEnum();
        auto borderParam = GetBorderFromEnum(border);
        auto interpParam = GetInterpFromEnum(interp);

        glTexParameteri(dimParam, GL_TEXTURE_MAG_FILTER, interpParam);
        glTexParameteri(dimParam, GL_TEXTURE_MIN_FILTER, interpParam);

        __TexDimOPSelector<dim> __selector;
        __selector.SetBorderParam(dimParam, borderParam);

        if (genMipMap)   glGenerateMipmap(dimParam);
    }
}

