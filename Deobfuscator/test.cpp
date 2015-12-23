#include "stdafx.h"

#include "test.h"
#include "trace.h"
#include <iostream>
#include <regex>
#include <string>

class TraceReaderTest : public TraceReader
{
public:
	std::shared_ptr<Instruction> parseInstructionWithoutBinary(std::string insn_str)
	{
		return TraceReader::parseInstructionWithoutBinary(insn_str);
	}
};

// テストフレームワークを準備できなかったので、いったん愚直に
bool trace_reader_test::parseInstructionTest()
{
	std::shared_ptr<Instruction> insn;
	TraceReaderTest reader;
	// with prefix
	
	std::string target("0x830a5949:  lock xadd %esi,(%eax)");
	insn = reader.parseInstructionWithoutBinary(target);
	if (insn->addr != 0x830a5949 || insn->prefix != "lock"
		|| insn->opcode != "xadd" || insn->operands.size() != 2) {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	// normal 
	target = std::string("0x830a5949:  xadd %esi,(%eax)");
	insn = reader.parseInstructionWithoutBinary(target);
	if (insn->addr != 0x830a5949 || !insn->prefix.empty()
		|| insn->opcode != "xadd" || insn->operands.size() != 2) {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	// without operand
	target = std::string("0x830a5949:  ret");
	insn = reader.parseInstructionWithoutBinary(target);
	if (insn->addr != 0x830a5949 || !insn->prefix.empty()
		|| insn->opcode != "ret" || insn->operands.size() != 0) {
		std::cout << "failed: " << target << std::endl;
		return false;
	}

	std::cout << "regexTest: success" << std::endl;
	return true;
}