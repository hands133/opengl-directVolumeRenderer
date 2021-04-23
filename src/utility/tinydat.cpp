#include "tinydat.h"
#include <glm/glm.hpp>

inline dat::Type property_type_from_string(const std::string & t) noexcept
{
    if (t == "int8" || t == "CHAR")           return dat::Type::INT8;
	else if (t == "uint8" || t == "UCHAR")    return dat::Type::UINT8;
	else if (t == "int16" || t == "SHORT")    return dat::Type::INT16;
	else if (t == "uint16" || t == "USHORT")  return dat::Type::UINT16;
	else if (t == "int32" || t == "INT")      return dat::Type::INT32;
	else if (t == "uint32" || t == "UINT")    return dat::Type::UINT32;
	else if (t == "int64" || t == "LLINT")    return dat::Type::INT64;
	else if (t == "uint64" || t == "ULLINT")  return dat::Type::UINT64;
	else if (t == "float32" || t == "FLOAT")  return dat::Type::FLOAT32;
	else if (t == "float64" || t == "DOUBLE") return dat::Type::FLOAT64;
	return dat::Type::INVALID;
}

struct DATFileImpl
{
    std::string rawFilePath;
    std::string tagFilePath;
    glm::uvec3 resolution;
    glm::fvec3 sliceThickness;
    dat::Type type;

    std::unique_ptr<dat::DATElement> e;

    bool parser(std::istream & is);

    template<typename T> inline T dat_read_ascii(std::istream & is);

    void read_header_rawPath(std::istream & is);
    void read_header_tagPath(std::istream & is);
    void read_header_rawRes(std::istream & is);
    void read_header_rawSlice(std::istream & is);
    void reader_header_rawType(std::istream & is);
};

bool DATFileImpl::parser(std::istream & is)
{
    std::string line;
    bool success = true;
    while (std::getline(is, line))
    {
        std::istringstream ls(line);
        std::string token;
        ls >> token;
        if (token == "ObjectFileName:")     read_header_rawPath(ls);
        else if(token == "TaggedFileName:") read_header_tagPath(ls);
        else if(token == "Resolution:")     read_header_rawRes(ls);
        else if(token == "SliceThickness:") read_header_rawSlice(ls);
        else if(token == "Format")          reader_header_rawType(ls);
        else    success = false;
    }
    return success;
}

void DATFileImpl::read_header_rawPath(std::istream & is)
{
    is >> this->rawFilePath;
}

void DATFileImpl::read_header_tagPath(std::istream & is)
{
    is >> this->tagFilePath;
}

void DATFileImpl::read_header_rawRes(std::istream & is)
{
    auto& res = this->resolution;
    is >> res.x >> res.y >> res.z;
}

void DATFileImpl::read_header_rawSlice(std::istream & is)
{
    auto& slice = this->sliceThickness;
    is >> slice.x >> slice.y >> slice.z;
}

void DATFileImpl::reader_header_rawType(std::istream & is)
{
    std::string typeStr;
    
}



