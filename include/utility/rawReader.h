#pragma once

// class for reading raw file and dat file
// dat file:
//          ObjectFileName: xxxxxx.raw
//          TaggedFilename: ---
//          Resolution: X Y Z
//          SliceThickness: ¦¤x ¦¤y ¦¤z
//          Format:         UCHAR

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <map>
#include <algorithm>

#include <glm/glm.hpp>

using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::string;

// describe file info
struct rawFileInfo
{
	std::string rawPath;
	std::string tagPath;    // what is this for?
	glm::uvec3 resolution;
	glm::fvec3 sliceThick;
};

// output stream operator overload for rawFileInfo
inline std::ostream& operator<<(std::ostream &out, const rawFileInfo &info)
{
	out << "raw file path: \t" << info.rawPath << std::endl;
	out << "tag file path: \t" << info.tagPath << std::endl;
	out << "resolution : \t[ " << info.resolution.x << ", " << info.resolution.y << ", " << info.resolution.z << " ]" << std::endl;
	out << "slice thick : \t[ " << info.sliceThick.x << ", " << info.sliceThick.y << ", " << info.sliceThick.z << " ]" << std::endl;
	return out;
}

enum class TYPE {
	NONE, UCHAR, CHAR, USHORT, SHORT, INT, UINT,
	FLOAT, LONG, ULONG, LLONG, ULLONG, DOUBLE
};

// the raw file class, origin from [ 0, 0, 0 ] -> [ X-1, Y-1, Z-1 ]
class rawFile
{
public:
	rawFile();
	~rawFile() { reset(); }
	// ADT method
	// file operation
	bool read(const std::string& datPath);
	// data info
	int itemSize() const { return eSize[static_cast<int>(type)]; }
	size_t numPoints() const;
	glm::vec2 spanX();
	glm::vec2 spanY();
	glm::vec2 spanZ();
	glm::vec2 spanValue() { return glm::vec2(minV, maxV); }
	// file info
	std::string path() const		{ return info.rawPath; }
	std::string tagPath() const		{ return info.tagPath; }
	glm::uvec3  resolution() const	{ return info.resolution; }
	glm::fvec3  sliceThick() const	{ return info.sliceThick; }
	// data
	void* data() const { return this->d; }
	// auxiliary functions
	std::ostream& operator<<(std::ostream& out) const;

private:
	// private auxiliary functions
	void reset();
	bool readDatFile(const std::string& datPath);
	bool memoryAlloc();
	bool readRawFile(const std::string& rawPath);
	void calMinMax(TYPE t);

	// data members
	void* d;
	TYPE type;
	rawFileInfo info;
	float minV, maxV;

	// auxiliary variables
	std::map<std::string, TYPE> str2Enum;
	std::vector<std::string> nameType = {
		"NONE", "UCHAR", "CHAR", "USHORT", "SHORT", "INT", "UINT",
		"FLOAT", "LONG", "ULONG", "LLONG", "ULLONG", "DOUBLE" };
	std::vector<TYPE> typeType = {
		TYPE::NONE, TYPE::UCHAR, TYPE::CHAR, TYPE::USHORT, TYPE::SHORT, TYPE::INT, TYPE::UINT, TYPE::FLOAT, TYPE::LONG, TYPE::ULONG, TYPE::LLONG, TYPE::ULLONG, TYPE::DOUBLE };
	std::vector<int8_t> eSize = { 0, 1, 1, 2, 2, 4, 4, 4, 4, 4, 8, 8, 8 };
};

inline std::ostream& operator<<(std::ostream& out, const rawFile& file)
{
	return file.operator<<(out);
}