#pragma once
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>

class PEReader
{
private:
	std::ifstream ifs;
	PEReader();
	~PEReader();

public:
	std::string signature();

	class Builder
	{
	public:
		Builder(const std::string file_path);
		PEReader *build();
		std::string getErrMsg();
	};
};

class PEFormat{
public:
	IMAGE_DOS_HEADER			dos_header;
	char						*dos_stub;
	int						dos_stub_size;
	IMAGE_NT_HEADERS			nt_headers;
	int						number_of_sections;
	std::vector<IMAGE_SECTION_HEADER>	section_headers;
	std::vector<SectionData>			section_data;
};

class SectionData {
private:
	std::tuple<char*, int> value;

public:
	SectionData(char *data, int size);

	char *data();
	int size();
};