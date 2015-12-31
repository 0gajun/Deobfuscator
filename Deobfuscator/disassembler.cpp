#include "stdafx.h"
#include "disassembler.h"
#include <iostream>

DisassembleResult::DisassembleResult()
{
}

DisassembleResult Disassembler::disasmPE(PEFormat pe_fmt)
{
	for (unsigned int i = 0; i < pe_fmt.section_headers.size(); i++) {
		IMAGE_SECTION_HEADER header = pe_fmt.section_headers[i];
		if ((header.Characteristics & IMAGE_SCN_MEM_EXECUTE)
			&& (header.Characteristics & IMAGE_SCN_MEM_READ)) {
			std::cout << header.Name << std::endl;
			std::vector<unsigned char> code = pe_fmt.section_data[i];
			unsigned int size = header.Misc.VirtualSize;
			execute(code, size, header.VirtualAddress);
		}
	}
	return result;
}

DisassembleResult Disassembler::execute(std::vector<unsigned char> code,
	unsigned int size, unsigned int base_addr)
{
	// thid function is c code because of using capstone
	csh handle;
	cs_insn *insn;
	size_t count;

	if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle) != CS_ERR_OK) {
		return DisassembleResult();
	}

	count = cs_disasm(handle, code.data(), size, base_addr, 0, &insn);

	if (count > 0) {
		for (unsigned int i = 0; i < count; i++) {
			result.insns_addr_set.insert((unsigned int)insn[i].address);
		}
	}
	else {
		return DisassembleResult();
	}

	cs_free(insn, count);
	return result;
}