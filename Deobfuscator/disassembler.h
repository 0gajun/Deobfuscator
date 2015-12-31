#pragma once
#include "pe.h"
#include <capstone.h>

class DisassembleResult
{
public:
	DisassembleResult();
	std::set<unsigned int> insns_addr_set;
};

// not implemented completely.
// now, result is only address
class Disassembler
{
private:
	DisassembleResult result;
public:
	DisassembleResult disasmPE(PEFormat pe_fmt);

	DisassembleResult execute(std::vector<unsigned char> code,
		unsigned int size, unsigned int base_addr);
};