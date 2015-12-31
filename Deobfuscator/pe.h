#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <fstream>
#include <tuple>

class TraceAnalysisResult;
#include "trace.h"

class PEFormat {
public:
	IMAGE_DOS_HEADER			dos_header;
	std::vector<char>		dos_stub;
	IMAGE_NT_HEADERS			nt_headers;
	int						number_of_sections;
	std::vector<IMAGE_SECTION_HEADER>	section_headers;
	std::vector<std::vector<unsigned char>>		section_data;
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
	std::unique_ptr<TraceAnalysisResult> analysis_result;
	boolean isValidFormat();
	boolean isValidSectionAlignment();
	boolean isValidFileAlignment();
	void commit();
	int calcImageSize();
	int calcHeaderSize();
	void execCommandIfNeeded();
	unsigned int getRealImageBase();
public:
	PEEditor(PEFormat pe_fmt);
	PEEditor(PEFormat pe_fmt, std::unique_ptr<TraceAnalysisResult> analysis_result);
	~PEEditor();

	PEFormat* result();

	// modification methods
	void addSection(IMAGE_SECTION_HEADER section_header, std::vector<unsigned char> section_virtual_data);
	boolean overwriteCode(std::vector<unsigned char> new_code, unsigned int raw_addr);

	// For shadow section
	void initializeShadowSectionBuilder(unsigned int size);
	unsigned int appendShadowSectionCode(std::vector<unsigned char> code);

	unsigned int convertFromVirtToRawAddr(unsigned int virt_addr);
	unsigned int convertToOriginalVirtAddr(unsigned int virt_addr);

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
		SectionHeaderBuilder* setVirtualSize(DWORD vsize);
		PEEditor::SectionHeaderBuilder * setVirtualAddress(DWORD vaddr);
		PEEditor::SectionHeaderBuilder * setSizeOfRawData(DWORD size_of_raw_data);
		PEEditor::SectionHeaderBuilder * setPointerToRawData(DWORD addr);
		PEEditor::SectionHeaderBuilder * setPointerToRelocations(DWORD addr);
		PEEditor::SectionHeaderBuilder * setPointerToLinenumbers(DWORD addr);
		PEEditor::SectionHeaderBuilder * setNumberOfRelocations(WORD num);
		PEEditor::SectionHeaderBuilder * setNumberOfLinenumbers(WORD num);
		PEEditor::SectionHeaderBuilder * setCharacteristcs(DWORD flag);
		IMAGE_SECTION_HEADER build();
	};

	class ShadowSectionBuilder
	{
	private:
		const unsigned int v_offset;
		const unsigned int v_size;
		std::vector<unsigned char> code_buffer;

	public:
		ShadowSectionBuilder(unsigned int virtual_offset, unsigned int virtual_size);

		unsigned int appendCode(std::vector<unsigned char> code);
		bool hasCode();
		unsigned int requiredSize();

		std::pair<IMAGE_SECTION_HEADER, std::vector<unsigned char>> build(unsigned int pointer_to_raw_data, unsigned int size_of_raw_data);
	};


private:
	std::unique_ptr<ShadowSectionBuilder> shadow_sec_builder;
};

class PEUtil
{
public:
	static unsigned int getVirtAddrBeforeRelocate(const unsigned int vaddr,
		const unsigned int actual_oep, PEFormat pe_fmt);
};