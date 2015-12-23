#include "stdafx.h"
#include "trace.h"
#include "string_util.h"
#include <sstream>


#define LOG_DELIMITER "="
#define ADDRESS_LEN_IN_LOGFILE 10

#define NUM_OF_CHARACTER_IN_A_BYTE 2

TraceReader::TraceReader()
{
	this->asm_insn_regex = std::regex(LOGFILE_ASM_LINE_REGEX);

	this->data = std::make_shared<TraceData>();
}

TraceReader::~TraceReader()
{
	if (ifs.is_open()) {
		ifs.close();
	}
}

bool TraceReader::openTraceFile(const std::string trace_file_path)
{
	ifs.open(trace_file_path, std::ios::in);
	return ifs.is_open();
}

std::shared_ptr<TraceData> TraceReader::read()
{
	if (!ifs.is_open()) {
		throw std::exception::exception("file isn't opened.");
	}

	int bb_id = 0;
	std::string line;
	std::string code_bytes;
	std::vector<std::string> insn_buffer;
	while (std::getline(ifs, line)) {
		if (line.empty()) {
			continue;
		}
		if (line == LOG_DELIMITER) {
			std::getline(ifs, code_bytes);
			std::shared_ptr<BasicBlock> bb = parseBasicBlock(bb_id++, insn_buffer, code_bytes);
			data->basic_blocks.insert(
				std::map<int, std::shared_ptr<BasicBlock>>::value_type(bb->original_id, bb));
			insn_buffer.clear();
			code_bytes.clear();
			continue;
		}

		insn_buffer.push_back(line);
	}
	return data;
}

std::shared_ptr<BasicBlock> TraceReader::parseBasicBlock(
	int id,
	std::vector<std::string> insn_buffer,
	std::string code_bytes)
{
	std::shared_ptr<BasicBlock> bb = std::make_shared<BasicBlock>(id);
	
	// Parse each instruction with out code bytes
	for (std::string insn : insn_buffer) {
		std::shared_ptr<Instruction> ptr = parseInstructionWithoutBinary(insn);
		bb->insn_list.push_back(ptr);
	}
	bb->head_insn_addr = bb->insn_list[0]->addr;
	bb->tail_insn_addr = bb->insn_list[bb->insn_list.size() - 1]->addr;

	parseBinaryCodeInBasicBlock(bb, code_bytes);

	return bb;
}

// parse an instruction line to Instruction Object
// it includes address, opcode and operands.
// this method don't generate binary code into Instruction object
std::shared_ptr<Instruction> TraceReader::parseInstructionWithoutBinary(std::string insn_str)
{
	std::shared_ptr<Instruction> insn = std::make_shared<Instruction>();
	std::smatch result;
	
	std::regex_match(insn_str, result, this->asm_insn_regex);

	std::string addr = result[1].str();
	insn->addr = std::stoul(addr, nullptr, 16);
	insn->opcode = result[2].str();

	insn->operands = StringUtil::split(result[3].str(), ',');

	return insn;
}

// parse binary code and insert into Instruction object
// return value: next basic block address
unsigned int TraceReader::parseBinaryCodeInBasicBlock(std::shared_ptr<BasicBlock> bb_ptr, std::string code_bytes)
{
	int basic_block_insn_size = code_bytes.size() / NUM_OF_CHARACTER_IN_A_BYTE; // Based on that logfile's each code byte consists of two characters
	int offset = 0;

	// Parse code bytes, excluding last instruction
	for (unsigned int i = 0; i < bb_ptr->insn_list.size() - 1; i++) {
		// calc size of instruction
		int insn_size = bb_ptr->insn_list[i + 1]->addr - bb_ptr->insn_list[i]->addr;
		
		// convert hex string to byte vector
		int j;
		for (j = 0; j < insn_size; j++) {
			std::string code_in_a_byte
				= code_bytes.substr(offset + j * NUM_OF_CHARACTER_IN_A_BYTE, NUM_OF_CHARACTER_IN_A_BYTE);
			bb_ptr->insn_list[i]->binary.push_back((unsigned char)std::stoi(code_in_a_byte, nullptr, 16));
		}
		offset = offset + j * NUM_OF_CHARACTER_IN_A_BYTE;
	}

	// Parse code bytes of last instruction
	std::shared_ptr<Instruction> last_insn = bb_ptr->insn_list[bb_ptr->insn_list.size() - 1];
	for (unsigned int i = 0; offset + i * NUM_OF_CHARACTER_IN_A_BYTE < code_bytes.size(); i++){
		std::string code_in_a_byte = code_bytes.substr(offset + i * NUM_OF_CHARACTER_IN_A_BYTE, NUM_OF_CHARACTER_IN_A_BYTE);
		last_insn->binary.push_back((unsigned char)std::stoi(code_in_a_byte, nullptr, 16));
	}
		
	return basic_block_insn_size;
}