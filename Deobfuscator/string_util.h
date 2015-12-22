#pragma once
#include <string>
#include <vector>

class StringUtil
{
public:
	static std::vector<std::string> split(std::string src, char delimiter);
};