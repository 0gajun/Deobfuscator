// Deobfuscator.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "pe.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "invalid arguments" << endl;
		return -1;
	}

	PEReader::Builder builder = PEReader::Builder(string(argv[1]));

	PEReader *reader;
	if ((reader = builder.build()) == nullptr) {
		cout << builder.getErrMsg() << endl;
		exit(1);
	}

	return 0;
}


/*
int main(int argc, char** argv)
{
	ifstream ifs;
	IMAGE_DOS_HEADER dos_header;
	IMAGE_NT_HEADERS nt_header;
	IMAGE_SECTION_HEADER *section_headers;

	if (argc != 2) {
		cout << "invalid arguments" << endl;
		return -1;
	}

	ifs.open(argv[1], ios::binary);
	if (!ifs) {
		cout << "Couldn't open file: " << argv[1] << endl;
		return -1;
	}

	// Read DOS_HEADER
	ifs.read((char *)&dos_header, sizeof(dos_header));

	// Read DOS_STUB
	int dos_stub_size = dos_header.e_lfanew - sizeof(IMAGE_DOS_HEADER);
	char *dos_stub = (char *)malloc(dos_stub_size);
	ifs.read(dos_stub, dos_stub_size);

	// Seek to start of NT_HEADER
	ifs.seekg(dos_header.e_lfanew, ios_base::beg);

	// Read NT_HEADER
	ifs.read((char *)&nt_header, sizeof(nt_header));

	const int number_of_sections = nt_header.FileHeader.NumberOfSections;

	section_headers = (IMAGE_SECTION_HEADER *)malloc(sizeof(IMAGE_SECTION_HEADER) * number_of_sections);
	// Read SECTION_HEADERs
	for (int i = 0; i < number_of_sections; i++) {
		ifs.read((char *)&section_headers[i], sizeof(IMAGE_SECTION_HEADER));
	}

	// Print results
	cout << setw(11) << "VirtualAddr:  SectionName" << endl;
	for (int i = 0; i < number_of_sections; i++) {
		cout << setw(11) << hex << section_headers[i].VirtualAddress;
		cout << ":  " << string(reinterpret_cast<char const*>(&section_headers[i].Name)) << endl;
	}

	char **section_data;
	section_data = (char **)malloc(sizeof(char *) * number_of_sections);

	for (int i = 0; i < number_of_sections; i++) {
		section_data[i] = (char *)malloc(section_headers[i].SizeOfRawData);
		ifs.seekg(section_headers[i].PointerToRawData, ios_base::beg);
		ifs.read(section_data[i], section_headers[i].SizeOfRawData);
	}

	IMAGE_SECTION_HEADER shadow_section{
		".shadow",
		0x1,
		0x4000,
		0x200,
		0x1000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x60000020
	};

	ofstream fout;
	fout.open("\\Users\\ogamal\\Documents\\hoge.exe", ios::binary);
	fout.write((char *)&dos_header, sizeof(IMAGE_DOS_HEADER));
	fout.write(dos_stub, dos_stub_size);
	fout.seekp(dos_header.e_lfanew, ios_base::beg);

	nt_header.FileHeader.NumberOfSections++;

	fout.write((char *)&nt_header, sizeof(IMAGE_NT_HEADERS));
	fout.write((char *)section_headers, number_of_sections * sizeof(IMAGE_SECTION_HEADER));
	
	fout.write((char *)&shadow_section, sizeof(IMAGE_SECTION_HEADER));

	for (int i = 0; i < number_of_sections; i++) {
		fout.seekp(section_headers[i].PointerToRawData, ios_base::beg);
		fout.write(section_data[i], section_headers[i].SizeOfRawData);
	}

	ifs.close();
	fout.close();
    return 0;
}
*/