#pragma once

#include <Windows.h>
#include <vector>
#include "section_data.h"

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
