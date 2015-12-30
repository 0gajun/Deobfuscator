#include "stdafx.h"
#include "command.h"
#include <iostream>


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

	editor->overwriteCode(new_jmp_insn.binary, editor->convertFromVirtToRawAddr(from_bb_last_insn->addr));
}