#include "stdafx.h"
#include "trace.h"
#include <iostream>


TraceAnalyzer::TraceAnalyzer(TraceData data) : trace(data)
{
}

bool TraceAnalyzer::analyze()
{
	detectPrologueEpilogueCodeRegion();

	for (auto it = trace.basic_blocks.begin(); it != trace.basic_blocks.end(); it++) {
		std::shared_ptr<BasicBlock> bb = it->second;
		const unsigned int bb_id = it->first;

		if (isInPrologueCode(bb_id)) {
			continue;
		}
		
		if (isInEpilogueCode(bb_id)) {
			break;
		}

		std::shared_ptr<Instruction> last_insn = bb->insn_list[bb->insn_list.size() - 1];
		if (last_insn->opcode == "call") {
			unsigned int ret_addr = getReturnAddressOfCallInsn(last_insn);
			call_stack.push(std::make_shared<CallStackInfo>(bb_id, ret_addr));
		}

		if (last_insn->opcode == "ret") {
			unsigned int ret_addr = trace.basic_blocks.at(bb_id + 1)->head_insn_addr;
			bool is_call_stack_tampered = (ret_addr != call_stack.top()->ret_addr);

			if (is_call_stack_tampered) {
				std::cout << "call_stack_tampered! : "  << bb_id << std::endl;
				std::cout << "opcode: " << last_insn->opcode << std::endl;
				std::cout << "addr: " << std::hex << last_insn->addr << std::endl;
				std::cout << "caller: " << trace.basic_blocks.at(call_stack.top()->caller_bb_id)->tail_insn_addr << std::endl;
			}
			call_stack.pop();
		}
	}
}

void TraceAnalyzer::detectPrologueEpilogueCodeRegion()
{
	bool is_started_program_code = false;

	auto it = trace.basic_blocks.begin();

	// move iterator to program code
	for (; !isProgramCode(it->second->head_insn_addr); it++) {
	}

	prologue_bb_range.first = 0;
	prologue_bb_range.second = it->first - 1;

	unsigned int non_program_code_start_bb_id = 0;

	// detect epilogue code
	for (; it != trace.basic_blocks.end(); it++) {
		unsigned int bb_addr = it->second->head_insn_addr;

		if (non_program_code_start_bb_id == 0 && !isProgramCode(bb_addr)) {
			// entering non program code
			non_program_code_start_bb_id = it->first;
		}
		else if (non_program_code_start_bb_id != 0 && isProgramCode(bb_addr)) {
			// exit non program code
			non_program_code_start_bb_id = 0;
		}
	}

	epilogue_bb_range.first = non_program_code_start_bb_id;
	epilogue_bb_range.second = trace.basic_blocks.size() - 1;
}

inline bool TraceAnalyzer::isProgramCode(unsigned int address)
{
	// TODO: should use more correctly method, this is temporaly method...orz
	return address < 0x60000000;
}

inline bool TraceAnalyzer::isInPrologueCode(unsigned int bb_id)
{
	return prologue_bb_range.first <= bb_id && bb_id <= prologue_bb_range.second;
}

inline bool TraceAnalyzer::isInEpilogueCode(unsigned int bb_id)
{
	return epilogue_bb_range.first <= bb_id && bb_id <= epilogue_bb_range.second;
}

unsigned int TraceAnalyzer::getReturnAddressOfCallInsn(std::shared_ptr<Instruction> call_insn)
{
	if (call_insn->opcode != "call") {
		abort();
	}
	return call_insn->addr + call_insn->binary.size();
}

TraceAnalyzer::CallStackInfo::CallStackInfo(unsigned int caller_bb_id, unsigned int ret_addr)
	: caller_bb_id(caller_bb_id), ret_addr(ret_addr)
{
}