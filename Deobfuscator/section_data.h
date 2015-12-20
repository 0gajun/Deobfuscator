#pragma once
#include <tuple>

class SectionData {
private:
	std::tuple<char*, int> value;

public:
	SectionData(char *data, int size);

	char *data();
	int size();
};

