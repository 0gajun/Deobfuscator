#include "stdafx.h"
#include <string>
#include "pe.h"

PEWriter::PEWriter(PEFormat *pe_fmt) {
	this->pe = pe_fmt;
}

PEWriter::~PEWriter() {
	if (ofs.is_open()) {
		ofs.close();
	}
}

boolean PEWriter::open(const std::string output_file_path) {
	ofs.open(output_file_path, std::ios::binary);
	return ofs.is_open();
}

void PEWriter::setPEFormat(PEFormat *pe_fmt) {
	this->pe = pe_fmt;
}

boolean PEWriter::write() {
	// start from first position of target file.
	ofs.seekp(0, std::ios::beg);

	// write DOS_HEADER and DOS_STUB
	ofs.write((char *)&pe->dos_header, sizeof(IMAGE_DOS_HEADER));
	ofs.write(pe->dos_stub.data(), pe->dos_stub.size());

	// write NT_HEADERS
	ofs.seekp(pe->dos_header.e_lfanew, std::ios::beg);
	ofs.write((char *)&pe->nt_headers, sizeof(IMAGE_NT_HEADERS));

	// write SECTION_HEADERS
	for (int i = 0; i < pe->number_of_sections; i++) {
		ofs.write((char *)&pe->section_headers[i], sizeof(IMAGE_SECTION_HEADER));
	}

	// write section data
	for (int i = 0; i < pe->number_of_sections; i++) {
		ofs.seekp(pe->section_headers[i].PointerToRawData, std::ios::beg);
		ofs.write((char *)pe->section_data[i].data(), pe->section_data[i].size());
	}
	return TRUE;
}

PEWriter::Builder::Builder()
{
}

PEWriter::Builder* PEWriter::Builder::setOutputFilePath(const std::string file_path)
{
	this->output_file_path = file_path;
	return this;
}

PEWriter::Builder* PEWriter::Builder::setPEFormat(PEFormat *pe)
{
	this->pe_fmt = pe;
	return this;
}

PEWriter* PEWriter::Builder::build()
{
	PEWriter *writer = new PEWriter(pe_fmt);

	if (!writer->open(output_file_path)) {
		err_msg = "Couldn't open output file";
		return nullptr;
	}

	return writer;
}

std::string PEWriter::Builder::getErrMsg()
{
	return err_msg;
}