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

class PEFormat {
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
	PEFormat getPEFormat();
	class Builder
	{
	private:
		std::string file_path;
		std::string err_msg;
	public:
		Builder();
		Builder* setInputFilePath(const std::string file_path);
		PEReader *build();
		std::string getErrMsg();
	};
};

class PEWriter
{
private:
	PEFormat *pe;
	std::ofstream ofs;

	PEWriter(PEFormat *pe_fmt);
	~PEWriter();
	boolean open(const std::string output_file_path);
public:

	void setPEFormat(PEFormat *pe_fmt);
	boolean write();

	class Builder
	{
	private:
		PEFormat *pe_fmt;
		std::string output_file_path;
		std::string err_msg;
	public:
		Builder();
		Builder* setOutputFilePath(const std::string file_path);
		Builder* setPEFormat(PEFormat *pe_fmt);
		PEWriter *build();
		std::string getErrMsg();
	};
};