#include "stdafx.h"
#include "trace.h"
#include <sstream>

Instruction::Builder::Builder(std::vector<unsigned char> binary_code)
{
	insn = std::make_unique<Instruction>();
	insn->binary = binary_code;
}

Instruction::Builder::Builder()
{
	insn = std::make_unique<Instruction>();
}

Instruction::Builder::~Builder()
{
	insn.reset();
}

void Instruction::Builder::appendCode(std::vector<unsigned char> code)
{
	insn->binary.insert(insn->binary.end(), code.begin(), code.end());
}

Instruction::Builder* Instruction::Builder::setAddress(unsigned int address)
{
	insn->addr = address;
	return this;
}

Instruction::Builder * Instruction::Builder::setPrefix(std::string prefix)
{
	insn->prefix = prefix;
	return this;
}

Instruction::Builder * Instruction::Builder::setOpcode(std::string opcode)
{
	insn->opcode = opcode;
	return this;
}

Instruction::Builder * Instruction::Builder::addOperand(std::string operand)
{
	insn->operands.push_back(operand);
	return this;
}

Instruction Instruction::Builder::build()
{
	return *insn;
}

Instruction::JmpInsnBuilder::JmpInsnBuilder(unsigned int address, unsigned int jmp_target_addr)
	: Builder(), jmp_target_addr(jmp_target_addr)
{
	setAddress(address);
	setOpcode("jmp");
}

Instruction Instruction::JmpInsnBuilder::build(const std::shared_ptr<PEEditor> editor)
{
	signed int short_jmp_offset = editor->convertToOriginalVirtAddr(jmp_target_addr) - (editor->convertToOriginalVirtAddr(insn->addr) + SHORT_JMP_INSN_SIZE);
	if (-128 <= short_jmp_offset && short_jmp_offset <= 127) {
		// short jmp
		unsigned char offset = short_jmp_offset;
		std::vector<unsigned char> short_jmp_code({ OPCODE_SHORT_JMP, offset });
		appendCode(short_jmp_code);
		addOperand(std::to_string(offset));
	}
	else {
		// near jmp
		signed int near_jmp_offset = editor->convertToOriginalVirtAddr(jmp_target_addr) - (editor->convertToOriginalVirtAddr(insn->addr) + NEAR_JMP_INSN_SIZE);
		unsigned char *ptr = (unsigned char *)&near_jmp_offset;
		std::vector<unsigned char> near_jmp_code({ OPCODE_NEAR_JMP, ptr[0], ptr[1], ptr[2], ptr[3] });
		appendCode(near_jmp_code);
		addOperand(std::to_string(near_jmp_offset));
	}
	return *insn;
}

unsigned int Instruction::JmpInsnBuilder::estimateJmpInstructionSize(
	std::shared_ptr<PEEditor> editor, unsigned int insn_addr, unsigned int jmp_target_addr)
{
	signed int short_jmp_offset = editor->convertToOriginalVirtAddr(jmp_target_addr) - (editor->convertToOriginalVirtAddr(insn_addr) + SHORT_JMP_INSN_SIZE);
	return -128 <= short_jmp_offset && short_jmp_offset <= 127 ? SHORT_JMP_INSN_SIZE : NEAR_JMP_INSN_SIZE;
}

Instruction::PushInsnBuilder::PushInsnBuilder(unsigned int imm32)
	: Builder(), imm(imm32)
{
	setOpcode("push");
}

Instruction::PushInsnBuilder::PushInsnBuilder(unsigned short imm16)
	: Builder(), imm(imm16)
{
	setOpcode("push");
}

Instruction Instruction::PushInsnBuilder::build()
{
	char* value_ptr = (char *)&imm;
	std::vector<unsigned char> code({ 0x68 });
	if (imm <= USHRT_MAX) {
		// imm16
		code.push_back(value_ptr[0]);
		code.push_back(value_ptr[1]);
	}
	else {
		// imm32
		code.push_back(value_ptr[0]);
		code.push_back(value_ptr[1]);
		code.push_back(value_ptr[2]);
		code.push_back(value_ptr[3]);
	}

	addOperand(std::to_string(imm));
	appendCode(code);
	return *insn;
}

BasicBlock::BasicBlock(const int id) : original_id(id)
{
	this->block_id_set.insert(id);
}

std::vector<char> BasicBlock::getCode()
{
	std::vector<char> code;
	code.reserve(tail_insn_addr - head_insn_addr + 15);
	for (std::shared_ptr<Instruction> insn : insn_list) {
		code.insert(code.end(), insn->binary.begin(), insn->binary.end());
	}
	return code;
}
