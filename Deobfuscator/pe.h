#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <fstream>
#include <tuple>

class PEFormat {
public:
	IMAGE_DOS_HEADER			dos_header;
	std::vector<char>		dos_stub;
	IMAGE_NT_HEADERS			nt_headers;
	int						number_of_sections;
	std::vector<IMAGE_SECTION_HEADER>	section_headers;
	std::vector<std::vector<char>>		section_data;
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
	void readSections(int num_of_sections);
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

class PEEditor
{
private:
	PEFormat pe_fmt;
	boolean isValidFormat();
	boolean isValidSectionAlignment();
	boolean isValidFileAlignment();
	void commit();
	int calcImageSize();
	int calcHeaderSize();
public:
	PEEditor(PEFormat pe_fmt);
	~PEEditor();

	PEFormat* result();

	// modification methods
	void addSection(IMAGE_SECTION_HEADER section_header, char * section_virtual_data, int section_vitrual_size);
	boolean overwriteCode(char * new_value, int size, int raw_addr);

	class SectionHeaderBuilder
	{
	private:
		BYTE		name[IMAGE_SIZEOF_SHORT_NAME];
		DWORD	virtual_size = 0;
		DWORD	virtual_address = 0;
		DWORD	size_of_raw_data = 0;
		DWORD	pointer_to_raw_data = 0;
		DWORD	pointer_to_relocations = 0;
		DWORD	pointer_to_linenumbers = 0;
		WORD		number_of_relocations = 0;
		WORD		number_of_linenumbers = 0;
		DWORD	characteristics = 0;

		boolean has_error = false;
		std::string err_msg;
	public:
		SectionHeaderBuilder();

		SectionHeaderBuilder* setName(BYTE* name, int size);
		SectionHeaderBuilder* setVirtualSize(int vsize);
		PEEditor::SectionHeaderBuilder * setVirtualAddress(int vaddr);
		PEEditor::SectionHeaderBuilder * setSizeOfRawData(int size_of_raw_data);
		PEEditor::SectionHeaderBuilder * setPointerToRawData(int addr);
		PEEditor::SectionHeaderBuilder * setPointerToRelocations(int addr);
		PEEditor::SectionHeaderBuilder * setPointerToLinenumbers(int addr);
		PEEditor::SectionHeaderBuilder * setNumberOfRelocations(int num);
		PEEditor::SectionHeaderBuilder * setNumberOfLinenumbers(int num);
		PEEditor::SectionHeaderBuilder * setCharacteristcs(int flag);
		IMAGE_SECTION_HEADER build();
	};
};