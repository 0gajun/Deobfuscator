#include "stdafx.h"
#include "pe.h"

SectionData::SectionData(char *data, int size) {
	value = std::tie(data, size);
}

char* SectionData::data() {
	return std::get<0>(value);
}

int SectionData::size() {
	return std::get<1>(value);
}