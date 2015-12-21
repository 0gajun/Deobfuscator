#include "stdafx.h"
#include "pe.h"
#include <sstream>

PEEditor::PEEditor(PEFormat pe)
{
	this->pe_fmt = pe;
}

PEEditor::~PEEditor()
{
}

PEFormat PEEditor::result()
{
	return this->pe_fmt;
}

void PEEditor::addSection(IMAGE_SECTION_HEADER section_header,
	char* section_raw_data, int section_raw_size)
{

}

/*
 * SectionHeaderBuilder
 */
PEEditor::SectionHeaderBuilder::SectionHeaderBuilder()
{
	memcpy(this->name, 0, IMAGE_SIZEOF_SHORT_NAME);
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setName(BYTE* name, int size)
{
	if (size > IMAGE_SIZEOF_SHORT_NAME) {
		char tmp_name[IMAGE_SIZEOF_SHORT_NAME];
		strncpy(tmp_name, (char *)name, IMAGE_SIZEOF_SHORT_NAME - 1);
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

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setVirtualSize(int vsize)
{
	this->virtual_size = vsize;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setVirtualAddress(int vaddr)
{
	this->virtual_address = vaddr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setSizeOfRawData(int size_of_raw_data)
{
	this->size_of_raw_data = size_of_raw_data;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToRawData(int addr)
{
	this->pointer_to_raw_data = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToRelocations(int addr)
{
	this->pointer_to_relocations = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setPointerToLinenumbers(int addr)
{
	this->pointer_to_linenumbers = addr;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setNumberOfRelocations(int num)
{
	this->number_of_relocations = num;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setNumberOfLinenumbers(int num)
{
	this->number_of_linenumbers;
	return this;
}

PEEditor::SectionHeaderBuilder* PEEditor::SectionHeaderBuilder::setCharacteristcs(int flag)
{
	this->characteristics |= flag;
	return this;
}

IMAGE_SECTION_HEADER PEEditor::SectionHeaderBuilder::build()
{
	IMAGE_SECTION_HEADER header = {
		*name,
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
	return header;
}