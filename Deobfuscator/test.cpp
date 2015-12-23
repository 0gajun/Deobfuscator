#include "stdafx.h"

#include "test.h"
#include "trace.h"
#include <iostream>
#include <regex>
#include <string>


// テストフレームワークを準備できなかったので、いったん愚直に
bool trace_reader_test::regexTest()
{
	std::regex rex(LOGFILE_ASM_LINE_REGEX);
	std::smatch result;

	// with prefix
	std::string target("0x830a5949:  lock xadd %esi,(%eax)");
	std::regex_match(target, result, rex);
	if (result[1].str() != "830a5949" || result[2].str() != "lock"
		|| result[3].str() != "xadd" || result[4].str() != "%esi,(%eax)") {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	// normal 
	target = std::string("0x830a5949:  xadd %esi,(%eax)");
	std::regex_match(target, result, rex);
	if (result[1].str() != "830a5949" || !result[2].str().empty()
		|| result[3].str() != "xadd" || result[4].str() != "%esi,(%eax)") {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	// without operand
	target = std::string("0x830a5949:  ret");
	std::regex_match(target, result, rex);
	if (result[1].str() != "830a5949" || !result[2].str().empty()
		|| result[3].str() != "ret" || !result[4].str().empty()) {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	std::cout << "regexTest: success" << std::endl;
	return true;
}