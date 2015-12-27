#include "stdafx.h"
#include "pe.h"
#include <sstream>
#include <algorithm>

PEEditor::PEEditor(PEFormat pe)
{
	this->pe_fmt = pe;
}

PEEditor::~PEEditor()
{
}

PEFormat* PEEditor::result()
{
	if (!isValidFormat()) {
		// TODO: implement error message notification
		return nullptr;
	}
	commit();
	return &this->pe_fmt;
}

boolean PEEditor::isValidFormat()
{
	return isValidSectionAlignment() && isValidFileAlignment();
}

boolean PEEditor::isValidSectionAlignment()
{
	// TODO: implement validation
	return true;
}

boolean PEEditor::isValidFileAlignment()
{
	// TODO: implement validation
	return true;
}

void PEEditor::commit()
{
	// Update number of sections
	pe_fmt.nt_headers.FileHeader.NumberOfSections = pe_fmt.number_of_sections;

	// Update Image size which is needed in loading image file
	pe_fmt.nt_headers.OptionalHeader.SizeOfImage = calcImageSize();

	// Update Header size
	pe_fmt.nt_headers.OptionalHeader.SizeOfHeaders = calcHeaderSize();
}

int PEEditor::calcImageSize()
{
	int size = 0;
	for (std::vector<IMAGE_SECTION_HEADER>::iterator it = pe_fmt.section_headers.begin();
			it != pe_fmt.section_headers.end(); it++) {
		size = std::max<int>(size, (*it).VirtualAddress + (*it).Misc.VirtualSize);
	}
	return size;
}

int PEEditor::calcHeaderSize()
{
	// Header size is sum of headers including dos_stub.
	int not_aligned_size = sizeof(IMAGE_DOS_HEADER)
		+ pe_fmt.dos_stub.size()
		+ sizeof(IMAGE_NT_HEADERS)
		+ sizeof(IMAGE_SECTION_HEADER) * pe_fmt.number_of_sections;

	// SizeOfHeader should be multiples of FileAlignment
	return (not_aligned_size / pe_fmt.nt_headers.OptionalHeader.FileAlignment + 1)
		* pe_fmt.nt_headers.OptionalHeader.FileAlignment;
}

void PEEditor::addSection(IMAGE_SECTION_HEADER section_header, std::vector<char> section_virtual_data)
{
	int section_raw_size 
		= (section_virtual_data.size() / pe_fmt.nt_headers.OptionalHeader.FileAlignment + 1)
		* pe_fmt.nt_headers.OptionalHeader.FileAlignment;

	std::vector<char> section_raw_data(section_raw_size);

	memset(section_raw_data.data(), 0, section_raw_size);
	memcpy(section_raw_data.data(), &section_virtual_data[0], section_virtual_data.size());

	pe_fmt.section_headers.push_back(section_header);
	pe_fmt.section_data.push_back(section_raw_data);
	pe_fmt.number_of_sections++;
}

boolean PEEditor::overwriteCode(char *new_code, int size, int raw_addr)
{
	for (unsigned int i = 0; i < pe_fmt.section_headers.size(); i++) {
		int section_start_addr = pe_fmt.section_headers[i].PointerToRawData;
		int section_end_addr = section_start_addr + pe_fmt.section_headers[i].SizeOfRawData - 1;

		if (section_start_addr <= raw_addr && raw_addr + size <= section_end_addr) {
			memcpy(pe_fmt.section_data[i].data(), new_code, size);
			return true;
		}
	}
	return false;
}

/*
 * SectionHeaderBuilder
 */
PEEditor::SectionHeaderBuilder::SectionHeaderBuilder()
{
	memset(this->name, 0, IMAGE_SIZEOF_SHORT_NAME);
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setName(BYTE* name, int size)
{
	if (size > IMAGE_SIZEOF_SHORT_NAME) {
		char tmp_name[IMAGE_SIZEOF_SHORT_NAME];
		memcpy(tmp_name, (char *)name, IMAGE_SIZEOF_SHORT_NAME - 1);
		tmp_name[IMAGE_SIZEOF_SHORT_NAME - 1] = '\0';
		std::ostringstream stream;
		stream << "section header name is too long : " << tmp_name;
		err_msg = stream.str();
		has_error = true;
	}
	else {
		memset(this->name, 0, IMAGE_SIZEOF_SHORT_NAME);
		memcpy(this->name, name, size);
	}
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setVirtualSize(DWORD vsize)
{
	this->virtual_size = vsize;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setVirtualAddress(DWORD vaddr)
{
	this->virtual_address = vaddr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setSizeOfRawData(DWORD size_of_raw_data)
{
	this->size_of_raw_data = size_of_raw_data;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToRawData(DWORD addr)
{
	this->pointer_to_raw_data = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToRelocations(DWORD addr)
{
	this->pointer_to_relocations = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToLinenumbers(DWORD addr)
{
	this->pointer_to_linenumbers = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setNumberOfRelocations(WORD num)
{
	this->number_of_relocations = num;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setNumberOfLinenumbers(WORD num)
{
	this->number_of_linenumbers;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setCharacteristcs(DWORD flag)
{
	this->characteristics |= flag;
	return this;
}

IMAGE_SECTION_HEADER PEEditor::SectionHeaderBuilder::build()
{
	IMAGE_SECTION_HEADER header = {
		{0},
		virtual_size,
		virtual_address,
		size_of_raw_data,
		pointer_to_raw_data,
		pointer_to_relocations,
		pointer_to_linenumbers,
		number_of_relocations,
		number_of_linenumbers,
		characteristics
	};
	memcpy(header.Name, name, IMAGE_SIZEOF_SHORT_NAME);
	return header;
}

PEEditor::ShadowSectionBuilder::ShadowSectionBuilder(unsigned int virtual_offset, unsigned int virtual_size)
	: v_offset(virtual_offset), v_size(virtual_size)
{
}

// return : virtual address of beginning of code
unsigned int PEEditor::ShadowSectionBuilder::appendCode(std::vector<char> code)
{
	unsigned int virtual_address = v_offset + code_buffer.size;
	code_buffer.insert(code_buffer.end(), code.begin(), code.end());
	if (code.size() > v_size) {
		// TODO: error
	}
	return virtual_address;
}

std::pair<IMAGE_SECTION_HEADER, std::vector<char>>
	PEEditor::ShadowSectionBuilder::build(unsigned int pointer_to_raw_data, unsigned int size_of_raw_data)
{
	BYTE section_name[7] = {'.', 's', 'h', 'a', 'd', 'o', 'w'};
	IMAGE_SECTION_HEADER section_header
		= PEEditor::SectionHeaderBuilder().setName(section_name, 7)
		->setVirtualSize(code_buffer.size())
		->setVirtualAddress(v_offset)
		->setSizeOfRawData(size_of_raw_data)
		->setPointerToRawData(pointer_to_raw_data)
		->setCharacteristcs(IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)
		->build();
	return std::pair<IMAGE_SECTION_HEADER, std::vector<char>>(section_header, code_buffer);
}