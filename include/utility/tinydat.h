#pragma once

#ifndef tinydat_h
#define tinydat_h

#include <vector>
#include <string>
#include <stdint.h>
#include <cstddef>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>

namespace dat
{
    enum class Type : uint8_t
    {
        INVALID,
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
        INT64,
        UINT64,
        FLOAT32,
        FLOAT64
    };

    struct PropertyInfo
    {
        PropertyInfo() {}
        PropertyInfo(int stride, std::string str)
            : stride(stride), str(str) {}
        int stride { 0 };
        std::string str;
    };

    static std::map<Type, PropertyInfo> PropertyTable =
    {                                                               
        { Type::INT8,       PropertyInfo(1, std::string("char"))    },
        { Type::UINT8,      PropertyInfo(1, std::string("uchar"))   },
        { Type::INT16,      PropertyInfo(2, std::string("short"))   },
        { Type::UINT16,     PropertyInfo(2, std::string("ushort"))  },
        { Type::INT32,      PropertyInfo(4, std::string("int"))     },
        { Type::UINT32,     PropertyInfo(4, std::string("uint"))    },
        { Type::INT64,      PropertyInfo(8, std::string("llint"))   },
        { Type::UINT64,     PropertyInfo(8, std::string("ullint"))  },
        { Type::FLOAT32,    PropertyInfo(4, std::string("float"))   },
        { Type::FLOAT64,    PropertyInfo(8, std::string("double"))  },
        { Type::INVALID,    PropertyInfo(0, std::string("INVALID")) }
    };

	class Buffer
	{
		uint8_t * alias{ nullptr };
		struct delete_array { void operator()(uint8_t * p) { delete[] p; } };
		std::unique_ptr<uint8_t, decltype(Buffer::delete_array())> data;
		size_t size{ 0 };
	public:
		Buffer() {};
		Buffer(const size_t size) : data(new uint8_t[size], delete_array()), size(size) { alias = data.get(); } // allocating
		Buffer(uint8_t * ptr) : alias(ptr) { } // non-allocating, todo: set size?
		uint8_t * get() { return alias; }
		size_t size_bytes() const { return size; }
	};

    struct DATData
    {
        Type t;
        Buffer buffer;
        size_t count { 0 };
    };

    struct DATProperty
    {
        DATProperty(std::istream & is);
        DATProperty(Type type, std::string & _name) : name(_name), propertyType(type) {}

		std::string name;
		Type propertyType{ Type::INVALID };
    };

    struct DATElement
    {
        DATElement(std::istream & is);
        DATElement(const std::string & _name, size_t count) : name(_name), size(count) {}
        std::string name;
        size_t size{ 0 };
        std::vector<DATProperty> properties;    // actually only raw is available
    };

    struct DATFile
    {
        struct DATFileImpl;
        std::unique_ptr<DATFileImpl> impl;

        DATFile();
        ~DATFile();

		bool parse_header(std::istream & is);

		void read(std::istream & is);

		void write(std::ostream & os, bool isBinary);

		std::vector<std::string> get_info() const;

		std::shared_ptr<DATData> request_properties_from_element(const std::string & elementKey,
			const std::vector<std::string> propertyKeys, const uint32_t list_size_hint = 0);
    };
}



#endif