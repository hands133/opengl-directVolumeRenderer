#include "rawReader.h"

rawFile::rawFile()
{
	d = nullptr;
	for (int i = 0; i < nameType.size(); ++i)
		str2Enum[nameType[i]] = typeType[i];
	reset();
}

size_t rawFile::numPoints() const
{
	auto& res = info.resolution;
	return res.x * res.y * res.z;
}

bool rawFile::read(const std::string& datPath)
{
	bool readDatSucc = readDatFile(datPath);
	bool dataMemAlloc = memoryAlloc();
	// must make sure the rawPath is set
	bool readRawSucc = readRawFile(info.rawPath);
	return readDatSucc && dataMemAlloc && readRawSucc;
}

bool rawFile::readDatFile(const std::string& datPath)
{
	std::ifstream datFile(datPath, std::ios::in);
	if (!datFile)
	{
		datFile.close();
		std::cerr << "[ERROR] Open File " << datPath << " failed!" << endl;
		return false;
	}

	std::string item;   // iter

	// Object file name
	datFile >> item;
	datFile >> info.rawPath;
	// Tagged file name
	datFile >> item;
	datFile >> info.tagPath;
	// Resolution
	datFile >> item;
	datFile >> item;
	info.resolution.x = std::stoi(item);
	datFile >> item;
	info.resolution.y = std::stoi(item);
	datFile >> item;
	info.resolution.z = std::stoi(item);
	// Slice thickness
	datFile >> item;
	datFile >> item;
	info.sliceThick.x = std::stof(item);
	datFile >> item;
	info.sliceThick.y = std::stof(item);
	datFile >> item;
	info.sliceThick.z = std::stof(item);
	// Format
	datFile >> item;
	datFile >> item;

	auto iter = str2Enum.find(item);
	if (iter == str2Enum.end())
	{
		datFile.close();
		std::cerr << "[ERROR] Invalid type of word 'Format' : " << item << endl;
		reset();
		return false;
	}
	type = iter->second;

	datFile.close();

	// update raw file path to absolute direction
	info.rawPath = datPath.substr(0, datPath.rfind('\\') + 1) + info.rawPath;

	return true;
}

glm::vec2 rawFile::spanX()
{
	glm::vec2 xrange = { 0.0, sliceThick().x * (resolution().x - 1) };
	float mid = (xrange.x + xrange.y) / 2;
	xrange -= mid;
	return xrange;
}

glm::vec2 rawFile::spanY()
{
	glm::vec2 yrange = { 0.0, sliceThick().y * (resolution().y - 1) };
	float mid = (yrange.x + yrange.y) / 2;
	yrange -= mid;
	return yrange;
}

glm::vec2 rawFile::spanZ()
{
	glm::vec2 zrange = { 0.0, sliceThick().z * (resolution().z - 1) };
	float mid = (zrange.x + zrange.y) / 2;
	zrange -= mid;
	return zrange;
}

bool rawFile::memoryAlloc()
{
	try
	{
		d = static_cast<void*>(malloc(numPoints() * itemSize()));
		memset(d, 0, numPoints() * itemSize());
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		reset();
		return false;
	}
	return true;
}

bool rawFile::readRawFile(const std::string& rawPath)
{
	std::ifstream file(rawPath, std::ios::binary | std::ios::in);
	if (!file)
	{
		std::cerr << "[ERROR] Read from file " << rawPath << " failed!" << endl;
		file.close();
		reset();
		return false;
	}
	try
	{
		file.read(static_cast<char*>(d), numPoints() * itemSize());
		calMinMax(this->type);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		file.close();
		reset();
		return false;
	}
	file.close();
	return true;
}

void rawFile::reset()
{
	info.rawPath = info.tagPath = "";
	info.resolution = { 0, 0, 0 };
	info.sliceThick = { 0.0, 0.0, 0.0 };
	minV = maxV = 0.0;
	if (d != nullptr)    free(d);
	d = nullptr;
}

std::ostream& rawFile::operator<<(std::ostream& out) const
{
	out << "=====================================" << std::endl;
	out << info;
	out << "Data Type : \t" << nameType[static_cast<int>(type)] << std::endl;
	out << "Element size : \t" << itemSize() << " Bytes" << std::endl;

	if (this->data() == nullptr)
		out << "[WARNING] Data description file and raw data have not been loaded!" << endl;
	else
	{
		out << "raw file loaded, total " << numPoints() * itemSize() << " Bytes." << std::endl;
		out << "data locates at " << data() << std::endl;
		out << "value range : \t[ " << minV << " , " << maxV << " ]" << std::endl;
	}

	out << "=====================================" << std::endl;
	return out;
}

void rawFile::calMinMax(TYPE t)
{
	std::pair<float, float> MM;		// min max
	switch (t)
	{
	case TYPE::NONE:
		MM = { 0.0, 0.0 };
		break;
	case TYPE::UCHAR:
	{
		unsigned char* iter = static_cast<unsigned char*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::CHAR:
	{
		char* iter = static_cast<char*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::USHORT:
	{
		unsigned short* iter = static_cast<unsigned short*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::SHORT:
	{
		short* iter = static_cast<short*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::INT:
	{
		int* iter = static_cast<int*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::UINT:
	{
		unsigned int* iter = static_cast<unsigned int*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::FLOAT:
	{
		float* iter = static_cast<float*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::LONG:
	{
		long* iter = static_cast<long*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::ULONG:
	{
		unsigned long* iter = static_cast<unsigned long*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::LLONG:
	{
		long long* iter = static_cast<long long*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::ULLONG:
	{
		unsigned long long* iter = static_cast<unsigned long long*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	case TYPE::DOUBLE:
	{
		double* iter = static_cast<double*>(d);
		auto mm = std::minmax_element(iter, iter + numPoints());
		MM = { *(mm.first), *(mm.second) };
	}
		break;
	default:
		break;
	}
	minV = MM.first;
	maxV = MM.second;
}