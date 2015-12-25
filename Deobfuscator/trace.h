#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <memory>
#include <stack>

class Instruction
{
public:
	unsigned int	 addr;
	std::string		prefix;
	std::string		opcode;
	std::vector<std::string> operands;
	std::vector<char> binary;
};

class BasicBlock
{
public:
	const int original_id;
	std::set<int> block_id_set;
	std::vector<std::shared_ptr<Instruction>> insn_list;
	unsigned int head_insn_addr;
	unsigned int tail_insn_addr;
	unsigned int next_bb_addr;

	std::unordered_map<int, std::shared_ptr<BasicBlock>> prev_bbs; // map<original_id, ptr>
	std::unordered_map<int, std::shared_ptr<BasicBlock>> next_bbs; // map<original_id, ptr>

	BasicBlock(const int id);
};

class TraceData
{
public:
	std::map<unsigned int, std::shared_ptr<BasicBlock>> basic_blocks;
	std::map<unsigned int, std::shared_ptr<BasicBlock>> addr_bb_map;
};

class TraceReader
{
private:
	std::ifstream ifs;
	std::shared_ptr<TraceData> data;

	std::shared_ptr<BasicBlock> registerBasicBlock(std::shared_ptr<BasicBlock> bb);
	std::shared_ptr<BasicBlock> registerNewBasicBlock(std::shared_ptr<BasicBlock> bb);
	std::shared_ptr<BasicBlock> registerExistingBasicBlock(std::shared_ptr<BasicBlock> bb);
	bool existsSameBasicBlock(std::shared_ptr<BasicBlock> bb);
	std::shared_ptr<BasicBlock> parseBasicBlock(int id, std::vector<std::string> insn_buffer, std::string code_bytes);
	unsigned int parseBinaryCodeInBasicBlock(std::shared_ptr<BasicBlock> bb_ptr, std::string code_bytes);

	bool existsOperand(std::vector<std::string> insn_chunks);

protected: // For testing
	std::shared_ptr<Instruction> parseInstructionWithoutBinary(std::string insn);

public:
	TraceReader();
	~TraceReader();

	bool openTraceFile(const std::string trace_file_path);

	std::shared_ptr<TraceData> read();
};

class TraceAnalyzer
{
private:
	class CallStackInfo
	{
	public:
		const unsigned int caller_bb_id;
		const unsigned int ret_addr;
		CallStackInfo(unsigned int caller_bb_id, unsigned int ret_addr);
	};

	TraceData trace;
	std::stack<std::shared_ptr<CallStackInfo>> call_stack;
	std::pair<unsigned int, unsigned int> prologue_bb_range;
	std::pair<unsigned int, unsigned int> epilogue_bb_range;

	unsigned int getReturnAddressOfCallInsn(std::shared_ptr<Instruction> call_insn);
	void detectPrologueEpilogueCode();
	inline bool isProgramCode(unsigned int address);

public:
	TraceAnalyzer(TraceData data);

	bool analyze();
};