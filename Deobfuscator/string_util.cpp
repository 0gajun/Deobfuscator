#include "stdafx.h"
#include "string_util.h"

std::vector<std::string> StringUtil::split(std::string src, char delim)
{
	std::vector<std::string> result;
	std::string elem;

	for (char c : src) {
		if (c == delim) {
			if (!elem.empty()) {
				result.push_back(elem);
			}
			elem.clear();
		}
		else {
			elem += c;
		}
		
	}
	if (!elem.empty()) {
		result.push_back(elem);
	}

	return result;
}
