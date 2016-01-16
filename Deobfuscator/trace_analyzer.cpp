#include "stdafx.h"
#include "trace.h"
#include "pe.h"
#include "disassembler.h"
#include <iostream>

#define IS_REDUNDANT_JMP_REDUCION_ENABLED TRUE

TraceAnalyzer::TraceAnalyzer(std::unique_ptr<TraceData> data, PEFormat pe_fmt)
	: trace(std::move(data))
{
	this->pe_fmt = std::make_unique<PEFormat>(pe_fmt);
}

void TraceAnalyzer::setDisassembleResult(std::unique_ptr<DisassembleResult> result)
{
	this->disassembler_result = std::move(result);
	option_flags |= TRACE_ANALYZER_ENABLE_OVERLAP_FUNC;
}

TraceAnalysisResult::TraceAnalysisResult()
{
	invoker = std::make_unique<CommandInvoker>();
}

std::unique_ptr<TraceAnalysisResult> TraceAnalyzer::analyze()
{
	std::unique_ptr<TraceAnalysisResult> result = std::make_unique<TraceAnalysisResult>();
	result->original_entry_point_vaddr = detectPrologueEpilogueCodeRegion();

	for (auto it = trace->basic_blocks.begin(); it != trace->basic_blocks.end(); it++) {
		std::shared_ptr<BasicBlock> bb = it->second;
		const unsigned int bb_id = it->first;

		if (isInPrologueCode(bb_id)) {
			continue;
		}
		
		if (isInEpilogueCode(bb_id)) {
			break;
		}

		std::shared_ptr<Instruction> last_insn = bb->insn_list.back();

		// reduction bbs which has only one basic block
		// (this deobfuscates simple overlapping functions and basic blocks
		if (bb->insn_list.size() == 1 && last_insn->opcode == "jmp" && IS_REDUNDANT_JMP_REDUCION_ENABLED) {
			std::shared_ptr<BasicBlock> prev_bb = trace->basic_blocks.at(bb_id - 1);
			int reduction_count = 0;

			// move iterator to the bb which doesn't have only one jmp
			// (we call such that bb as target_bb below
			while (it->second->insn_list.size() == 1 && it->second->insn_list.back()->opcode == "jmp") {
				reduction_count++;
				it++;
			}

			std::shared_ptr<RedundantJmpReductionCommand> cmd;
			if (prev_bb->insn_list.back()->opcode == "jmp") {
				// can jmp to target_bb from prev_bb directly
				cmd = std::make_shared<RedundantJmpReductionCommand>(prev_bb, it->second);
			}
			// check whether reduction is needed or not
			// decrement reduction_count because use first jmp bb, so cannot reduce that block
			else if (reduction_count - 1 > 0) {
				cmd = std::make_shared<RedundantJmpReductionCommand>(bb, it->second);
			}

			if (cmd && shouldIssueCommand(cmd)) {
				result->invoker->addCommand(cmd);
			}
			// decrement iterator because iterator will be incremented after continue;
			it--;
			continue;
		}

		// Check Overlapping functions and basic blocks
		if ((option_flags & TRACE_ANALYZER_ENABLE_OVERLAP_FUNC) && disassembler_result->insns_addr_set.size() != 0) {
			unsigned int actual_addr = PEUtil::getVirtAddrBeforeRelocate(bb->head_insn_addr, result->original_entry_point_vaddr, *pe_fmt);

			if (isOverlapped(actual_addr)) {
				std::cout << "overlapped!" << std::endl;
				std::shared_ptr<BasicBlock> prev_bb = trace->basic_blocks.at(bb_id - 1);
				std::vector<std::shared_ptr<BasicBlock>> overlapped_bbs;

				// collect continuous overlapped basic blocks
				while (isOverlapped(actual_addr)) {
					overlapped_bbs.push_back(it->second);
					it++;
					actual_addr = PEUtil::getVirtAddrBeforeRelocate(it->second->head_insn_addr, result->original_entry_point_vaddr, *pe_fmt);
				}
				std::shared_ptr<PECommand> cmd = std::make_shared<OverlappingFunctionAndBasicBlockCommand>(prev_bb, overlapped_bbs, it->second);
				if (shouldIssueCommand(cmd)) {
					result->invoker->addCommand(cmd);
				}
				continue;
			}
		}

		// Check non-returning call
		if (last_insn->opcode == "call") {
			unsigned int ret_addr = getReturnAddressOfCallInsn(last_insn);
			call_stack.push(std::make_shared<CallStackInfo>(bb_id, ret_addr));
		}

		if (last_insn->opcode == "ret") {
			unsigned int ret_addr = trace->basic_blocks.at(bb_id + 1)->head_insn_addr;
			bool is_call_stack_tampered = (ret_addr != call_stack.top()->ret_addr);

			if (is_call_stack_tampered) {
				std::cout << "caller: " << call_stack.top()->caller_bb_id << ", callee: " << bb_id << std::endl;
				if (call_stack.top()->caller_bb_id + 1 != bb_id) {
					// TODO: BasicBlockを超えたNon-Returning Callsにも対応する
					std::cout << "call stack is tampered. However, this deobfuscator version"
						<< " cannot fix non-returning call over basic block." << std::endl;
					std::cout << "So, skip...(addr: 0x" << std::hex << bb->head_insn_addr << std::endl;
				}
				else {
					std::shared_ptr<BasicBlock> caller = trace->basic_blocks.at(call_stack.top()->caller_bb_id);
					std::shared_ptr<BasicBlock> callee = bb;
					std::shared_ptr<BasicBlock> jmp_target = trace->basic_blocks.at(bb_id + 1);
					std::shared_ptr<NonReturningCallCommand> cmd
						= std::make_shared<NonReturningCallCommand>(caller, callee, jmp_target);
					if (shouldIssueCommand(cmd)) {
						result->invoker->addCommand(cmd);
					}
				}
			}
			call_stack.pop();
		}
	}
	return result;
}

// ret value: original entry point's virtual address
unsigned int TraceAnalyzer::detectPrologueEpilogueCodeRegion()
{
	bool is_started_program_code = false;

	auto it = trace->basic_blocks.begin();

	// move iterator to program code
	for (; !PEUtil::isProgramCode(it->second->head_insn_addr); it++) {
	}

	unsigned int original_entry_point_vaddr = it->second->head_insn_addr;

	prologue_bb_range.first = 0;
	prologue_bb_range.second = it->first - 1;

	unsigned int non_program_code_start_bb_id = 0;

	// detect epilogue code
	for (; it != trace->basic_blocks.end(); it++) {
		unsigned int bb_addr = it->second->head_insn_addr;

		if (non_program_code_start_bb_id == 0 && !PEUtil::isProgramCode(bb_addr)) {
			// entering non program code
			non_program_code_start_bb_id = it->first;
		}
		else if (non_program_code_start_bb_id != 0 && PEUtil::isProgramCode(bb_addr)) {
			// exit non program code
			non_program_code_start_bb_id = 0;
		}
	}

	epilogue_bb_range.first = non_program_code_start_bb_id;
	epilogue_bb_range.second = trace->basic_blocks.size() - 1;

	return original_entry_point_vaddr;
}

inline bool TraceAnalyzer::shouldIssueCommand(std::shared_ptr<PECommand> cmd)
{
	return cmd->isCommandInProgram();
}

inline bool TraceAnalyzer::isInPrologueCode(unsigned int bb_id)
{
	return prologue_bb_range.first <= bb_id && bb_id <= prologue_bb_range.second;
}

inline bool TraceAnalyzer::isInEpilogueCode(unsigned int bb_id)
{
	return epilogue_bb_range.first <= bb_id && bb_id <= epilogue_bb_range.second;
}

bool TraceAnalyzer::isOverlapped(unsigned int actual_addr)
{
	return PEUtil::isProgramCode(actual_addr)
		&& disassembler_result->insns_addr_set.find(actual_addr) == disassembler_result->insns_addr_set.end();
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