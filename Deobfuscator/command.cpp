#include "stdafx.h"
#include "command.h"
#include <iostream>
#include <numeric>

unsigned int CreateShadowSectionCommand::requiredSize()
{
	return code_buffer.size();
}

NonReturningCallCommand::NonReturningCallCommand
(std::shared_ptr<BasicBlock> caller_bb, std::shared_ptr<BasicBlock> callee_bb, std::shared_ptr<BasicBlock> jmp_target_bb)
	: caller_bb(caller_bb), callee_bb(callee_bb), jmp_target_bb(jmp_target_bb)
{
}

void NonReturningCallCommand::execute(std::shared_ptr<PEEditor> editor)
{
	// create shadow section
	std::list<Instruction> shadow_insns;
	for (std::shared_ptr<Instruction> insn_ptr : callee_bb->insn_list) {
		shadow_insns.push_back(Instruction(*insn_ptr.get()));
	}

	std::shared_ptr<Instruction> caller_call_insn = caller_bb->insn_list.back();
	unsigned int original_ret_addr
		= caller_call_insn->addr + caller_call_insn->binary.size();

	// insert push original ret addr insn
	Instruction push_insn = Instruction::PushInsnBuilder(original_ret_addr).build();
	shadow_insns.push_front(push_insn);
	unsigned int shadow_code_addr = editor->appendShadowSectionCode(push_insn.binary);

	// remove ret insn at tail of callee block
	shadow_insns.pop_back();

	// insert pop insn without store value( = add 4 to esp register )
	std::vector<unsigned char> pop_insn({ 0x83, 0xC4, 0x04 });
	Instruction pop_without_store
		= Instruction::Builder(pop_insn)
		.setOpcode("add")
		->addOperand("$esp")
		->addOperand("0x04")
		->build();
	shadow_insns.push_back(pop_without_store);

	// insert jmp insn to target basic block
	unsigned int jmp_insn_addr = shadow_code_addr;
	for (Instruction insn : shadow_insns) {
		jmp_insn_addr += insn.binary.size();
	}
	Instruction jmp_callee_to_target
		= Instruction::JmpInsnBuilder(jmp_insn_addr, jmp_target_bb->head_insn_addr)
		.build(editor);
	shadow_insns.push_back(jmp_callee_to_target);

	std::vector<unsigned char> shadow_code_without_first_insn;
	for (auto it = ++shadow_insns.begin(); it != shadow_insns.end(); it++) {
		shadow_code_without_first_insn
			.insert(shadow_code_without_first_insn.end(), it->binary.begin(), it->binary.end());
	}

	editor->appendShadowSectionCode(shadow_code_without_first_insn);

	// overwrite caller's call method by jmp
	Instruction jmp_to_shadow
		= Instruction::JmpInsnBuilder(caller_call_insn->addr, shadow_code_addr)
		.build(editor);
	caller_bb->insn_list.pop_back();
	caller_bb->insn_list.push_back(std::make_shared<Instruction>(jmp_to_shadow));
	editor->overwriteCode(jmp_to_shadow.binary, editor->convertFromVirtToRawAddr(jmp_to_shadow.addr));
}

bool NonReturningCallCommand::isCommandInProgram()
{
	// modification target is only caller_bb and callee_bb.
	// jmp_target isn't modified.
	return PEUtil::isProgramCode(caller_bb->head_insn_addr)
		&& PEUtil::isProgramCode(callee_bb->head_insn_addr);
}

void CommandInvoker::addCommand(std::shared_ptr<PECommand> command)
{
	commands.push_back(command);
}

void CommandInvoker::invoke(std::shared_ptr<PEEditor> editor)
{
	preInvoke(editor);

	for (std::shared_ptr<PECommand> command : commands) {
		command->execute(editor);
	}
}

bool CommandInvoker::hasCommand()
{
	return !commands.empty();
}

void CommandInvoker::preInvoke(std::shared_ptr<PEEditor> editor)
{
	editor->initializeShadowSectionBuilder(estimateShadowSectionSize());
}

unsigned int CommandInvoker::estimateShadowSectionSize()
{
	unsigned int estimate_size = 0;
	for (std::shared_ptr<PECommand> command : commands) {
		if (CreateShadowSectionCommand *cmd = dynamic_cast<CreateShadowSectionCommand *>(command.get())) {
			estimate_size += cmd->requiredSize();
		}
	}
	return estimate_size;
}

RedundantJmpReductionCommand::RedundantJmpReductionCommand(std::shared_ptr<BasicBlock> from_bb, std::shared_ptr<BasicBlock> to_bb)
	: from_bb(from_bb), to_bb(to_bb)
{
}

void RedundantJmpReductionCommand::execute(std::shared_ptr<PEEditor> editor)
{
	// validation
	std::shared_ptr<Instruction> from_bb_last_insn = from_bb->insn_list.back();
	if (from_bb_last_insn->opcode != "jmp") {
		std::cout << "invalid instruction was passed in RedundantJmpReductionCommand::execute()" << std::endl;
		abort();
	}
	unsigned int jmp_base_addr = editor->convertToOriginalVirtAddr(from_bb_last_insn->addr + from_bb_last_insn->binary.size());
	unsigned int jmp_target_addr = editor->convertToOriginalVirtAddr(to_bb->head_insn_addr);
	
	Instruction new_jmp_insn
		= Instruction::JmpInsnBuilder(from_bb_last_insn->addr, jmp_target_addr)
		.build(editor);

	if (new_jmp_insn.binary.size() > from_bb_last_insn->binary.size()) {
		std::cout << "new jmp code is larget than old jmp code. So, cannot overwrite it! skip..." << std::endl;
		return;
	}

	std::cout << "redundant jmp reduction" << std::endl;
	editor->overwriteCode(new_jmp_insn.binary, editor->convertFromVirtToRawAddr(from_bb_last_insn->addr));
}

bool RedundantJmpReductionCommand::isCommandInProgram()
{
	// modification target is only from_bb
	// to_bb isn't modified.
	return PEUtil::isProgramCode(from_bb->head_insn_addr);
}

bool OverlappingFunctionAndBasicBlockCommand::isNeededNearJmp(std::shared_ptr<PEEditor> editor)
{
	unsigned int next_shadow_head_addr = editor->nextShadowCodeAddr();
	unsigned int from = editor->convertToOriginalVirtAddr(prev_bb->tail_insn_addr + prev_bb->insn_list.back()->binary.size());
	return !PEUtil::canShortJmp(from, next_shadow_head_addr);
}

OverlappingFunctionAndBasicBlockCommand::OverlappingFunctionAndBasicBlockCommand(const std::shared_ptr<BasicBlock> prev_bb, const std::vector<std::shared_ptr<BasicBlock>> overlapped_bbs, const std::shared_ptr<BasicBlock> next_bb)
	: prev_bb(prev_bb), overlapped_bbs(overlapped_bbs), next_bb(next_bb)
{
}

void OverlappingFunctionAndBasicBlockCommand::execute(std::shared_ptr<PEEditor> editor)
{
	std::shared_ptr<Instruction> prev_bb_last_insn = prev_bb->insn_list.back();
	if (prev_bb_last_insn->opcode != "jmp") {
		// TODO: Implement
		std::cout << "OverlappingFunctionAndBasicBlockCommand for non jmp insn isn't implemented now. So, skip..." << std::endl;
		return;
	}
	if (overlapped_bbs.size() != 1) {
		std::cout << "OverlappingFunctionAndBasicBlockCommand for multi overlapped bbs isn't implemented now. So, skip..." << std::endl;
		return;
	}

	std::shared_ptr<BasicBlock> overlapped_bb = overlapped_bbs[0];
	std::list<Instruction> shadow_section_insns;

	unsigned int shadow_head_addr = editor->nextShadowCodeAddr();
	switch (prev_bb_last_insn->binary[0]) {
	case OPCODE_SHORT_JMP:
		if (isNeededNearJmp(editor)) {
			// have to replace insns before jmp insn together
			std::cout << "OPCODE_SHORT_JMP : " << "neededNearJmp" << std::endl;

			unsigned int replace_insn_head_addr;
			int locatable_size = 0;
			int index;
			for (index = prev_bb->insn_list.size() - 1; index >= 0; index--) {
				locatable_size += prev_bb->insn_list[index]->binary.size();
				if (locatable_size >= NEAR_JMP_INSN_SIZE) {
					replace_insn_head_addr = editor->convertToOriginalVirtAddr(prev_bb->insn_list[index]->addr);
					break;
				}
			}

			if (index < 0) {
				// cannot allocate
				// TODO: error handling correctly
				std::cout << "Cannot allocate jpm replacement area" << std::endl;
				return;
			}

			int prev_bb_insn_list_size = prev_bb->insn_list.size();
			for (int i = index; i < prev_bb_insn_list_size;  i++) {
				shadow_section_insns.push_front(*prev_bb->insn_list.back());
				prev_bb->insn_list.pop_back();
			}

			unsigned int new_short_jmp_addr = prev_bb->tail_insn_addr + prev_bb->insn_list.back()->binary.size() - SHORT_JMP_INSN_SIZE;
			bool can_short_jmp 
				= SHORT_JMP_INSN_SIZE ==  Instruction::JmpInsnBuilder::estimateJmpInstructionSize(editor, new_short_jmp_addr, shadow_head_addr);

			unsigned int nop_size = can_short_jmp ? locatable_size - SHORT_JMP_INSN_SIZE : locatable_size - NEAR_JMP_INSN_SIZE;

			unsigned int replace_insns_head_raw_addr
				= editor->convertFromVirtToRawAddr(replace_insn_head_addr);
			std::vector<unsigned char> nop_code;
			for (int i = 0; i < nop_size; i++) {
				nop_code.push_back(OPCODE_NOP);
			}
			Instruction new_jmp = Instruction::JmpInsnBuilder(replace_insn_head_addr + nop_size, shadow_head_addr).build(editor);
			editor->overwriteCode(nop_code, replace_insns_head_raw_addr);
			editor->overwriteCode(new_jmp.binary, replace_insns_head_raw_addr + nop_code.size());
			break;
		}
	// In case of that near jmp isn't needed when opcode is short jmp,
	// we only have to replace jmp address to shadow section
	// FALL_THROUGH...
	case OPCODE_NEAR_JMP:
	{
		unsigned int jmp_from_addr = editor->convertToOriginalVirtAddr(
			prev_bb->tail_insn_addr + prev_bb->insn_list.back()->binary.size());
		Instruction new_jmp_insn = Instruction::JmpInsnBuilder(jmp_from_addr, shadow_head_addr).build(editor);
		editor->overwriteCode(new_jmp_insn.binary, editor->convertFromVirtToRawAddr(jmp_from_addr));
	}
		break;
	default:
		std::cout << "invalid opcode or not implemented @OverlappingFunctionAndBasicBlockCommand#execute" << std::endl;
		return;
	}

	// For returning original code section
	// remove original jmp code
	shadow_section_insns.pop_back();
	for (Instruction insn : shadow_section_insns) {
		editor->appendShadowSectionCode(insn.binary);
	}

	unsigned int next_bb_head_addr = editor->convertToOriginalVirtAddr(next_bb->head_insn_addr);
	unsigned int return_jmp_addr = editor->nextShadowCodeAddr();
	Instruction return_jmp = Instruction::JmpInsnBuilder(return_jmp_addr, next_bb_head_addr).build(editor);
	editor->appendShadowSectionCode(return_jmp.binary);
}

bool OverlappingFunctionAndBasicBlockCommand::isCommandInProgram()
{
	// modification target is only prev_bb and overlapped_bbs
	return PEUtil::isProgramCode(prev_bb->head_insn_addr)
		&& std::accumulate(overlapped_bbs.begin(), overlapped_bbs.end(), true,
			[&](bool current_val, std::shared_ptr<BasicBlock> bb) {
		return current_val && PEUtil::isProgramCode(bb->head_insn_addr);
	});
}