#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <memory>
#include <regex>

class Instruction
{
public:
	unsigned int	 addr;
	std::string		opcode;
	std::vector<std::string> operands;
	std::vector<char> binary;
};

class BasicBlock
{
public:
	int original_id;
	std::set<int> block_id_set;
	std::vector<std::shared_ptr<Instruction>> insn_list;
	unsigned int head_insn_addr;
	unsigned int tail_insn_addr;
	unsigned int next_bb_addr;
	std::unordered_map<int, std::shared_ptr<BasicBlock>> prev_bbs;
	std::unordered_map<int, std::shared_ptr<BasicBlock>> next_bbs;
};

class TraceData
{
public:
	std::map<int, std::shared_ptr<BasicBlock>> basic_blocks;
};

class TraceReader
{
private:
	std::ifstream ifs;
	std::shared_ptr<TraceData> data;
	std::regex asm_insn_regex;

	std::shared_ptr<BasicBlock> parseBasicBlock(int id, std::vector<std::string> insn_buffer, std::string code_bytes);
	std::shared_ptr<Instruction> parseInstructionWithoutBinary(std::string insn);
	unsigned int parseBinaryCodeInBasicBlock(std::shared_ptr<BasicBlock> bb_ptr, std::string code_bytes);

public:
	TraceReader();
	~TraceReader();

	bool openTraceFile(const std::string trace_file_path);

	std::shared_ptr<TraceData> read();
};