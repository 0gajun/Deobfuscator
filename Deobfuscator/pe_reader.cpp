#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <new>
#include <memory>
#include "pe.h"

PEReader::PEReader() {
}

PEReader::~PEReader() {
	if (ifs.is_open()) {
		ifs.close();
	}
}

boolean PEReader::open(const std::string file_path) {
	ifs.open(file_path, std::ios::binary);
	if (!ifs) {
		return FALSE;
	}
	return TRUE;
}

boolean PEReader::readPE() {
	// Read DOS_HEADER
	ifs.read((char *)&pe.dos_header, sizeof(IMAGE_DOS_HEADER));
	// Read DOS_STUB
	pe.dos_stub.resize(pe.dos_header.e_lfanew - sizeof(IMAGE_DOS_HEADER));
	ifs.read(pe.dos_stub.data(), pe.dos_stub.size());
	// Seek to start position of NT_HEADER
	ifs.seekg(pe.dos_header.e_lfanew, std::ios::beg);
	// Read NT_HEADER
	ifs.read((char *)&pe.nt_headers, sizeof(IMAGE_NT_HEADERS));

	// Read Sections
	pe.number_of_sections = pe.nt_headers.FileHeader.NumberOfSections;
	readSections(pe.number_of_sections);
	return TRUE;
}

void PEReader::readSections(int num_of_sections) {
	// Read SECTION_HEADERs
	for (int i = 0; i < num_of_sections; i++) {
		IMAGE_SECTION_HEADER header;
		ifs.read((char *)&header, sizeof(IMAGE_SECTION_HEADER));
		pe.section_headers.push_back(header);
	}

	// Read section data
	for (int i = 0; i < pe.number_of_sections; i++) {
		int size = pe.section_headers[i].SizeOfRawData;
		std::vector<char> data(size);
		ifs.seekg(pe.section_headers[i].PointerToRawData, std::ios::beg);
		ifs.read(data.data(), size);

		pe.section_data.push_back(data);
	}
}

boolean PEReader::isPEFormat() {
	char *signature = (char *)&pe.nt_headers.Signature;
	return signature[0] == 'P' && signature[1] == 'E';
}

PEFormat PEReader::getPEFormat() {
	return this->pe;
}

PEReader::Builder::Builder() {
}

PEReader::Builder* PEReader::Builder::setInputFilePath(const std::string file_path)
{
	this->file_path = file_path;
	return this;
}

PEReader* PEReader::Builder::build() {
	PEReader *reader = new PEReader();
	if (!reader->open(file_path)) {
		err_msg = "cannot open file";
		return nullptr;
	}
	if (!reader->readPE()) {
		err_msg = "cannot read file";
		return nullptr;
	}
	if (!reader->isPEFormat()) {
		err_msg = "the file isn't PE Executable";
		return nullptr;
	}
	return reader;
}

std::string PEReader::Builder::getErrMsg() {
	return err_msg;
}