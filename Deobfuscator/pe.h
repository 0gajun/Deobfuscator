#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <fstream>
#include <tuple>

class SectionData {
private:
	std::tuple<char*, int> value;

public:
	SectionData(char *data, int size);

	char *data();
	int size();
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

class PEReader
{
private:
	PEFormat pe;
	std::ifstream ifs;

	PEReader();
	~PEReader();

	boolean open(const std::string file_path);
	boolean readPE();
	void readSections(int num_of_sections) throw(std::bad_alloc);
	char* data_malloc(int size) throw(std::bad_alloc);
	boolean PEReader::isPEFormat();
	
public:
	class Builder
	{
	private:
		std::string file_path;
		std::string err_msg;
	public:
		Builder(const std::string file_path);
		PEReader *build();
		std::string getErrMsg();
	};
};

class PEWriter
{

};