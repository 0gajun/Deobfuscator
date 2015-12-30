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

#define SHORT_JMP_INSN_SIZE 2
#define NEAR_JMP_INSN_SIZE 5
#define OPCODE_SHORT_JMP 0xEB
#define OPCODE_NEAR_JMP 0xE9

class CommandInvoker;
#include "command.h"
class PEEditor;

class Instruction
{
public:
	unsigned int	 addr;
	std::string		prefix;
	std::string		opcode;
	std::vector<std::string> operands;
	std::vector<unsigned char> binary;

	class Builder
	{
	protected:
		std::unique_ptr<Instruction> insn;
		void appendCode(std::vector<unsigned char> code);
	public:
		Builder(std::vector<unsigned char> binary_code);
		Builder();
		~Builder();

		Builder* setAddress(unsigned int addr);
		Builder* setPrefix(std::string prefix);
		Builder* setOpcode(std::string opcode);
		Builder* addOperand(std::string operand);

		virtual Instruction build();
	};

	class JmpInsnBuilder : Builder
	{
	private:
		unsigned int jmp_target_addr;
	public:
		JmpInsnBuilder(unsigned int address, unsigned int jmp_target_addr);

		Instruction build(const std::shared_ptr<PEEditor> editor);
	};

	class PushInsnBuilder : Builder
	{
	private:
		unsigned int imm;

	public:
		PushInsnBuilder(unsigned int imm32);
		PushInsnBuilder(unsigned short imm16);

		Instruction build();
	};
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
	std::vector<char> getCode();
};

class TraceData
{
public:
	std::map<unsigned int, std::shared_ptr<BasicBlock>> basic_blocks;
	std::map<unsigned int, std::shared_ptr<BasicBlock>> addr_bb_map;
};

class TraceAnalysisResult
{
public:
	TraceAnalysisResult();
	std::unique_ptr<CommandInvoker> invoker;
	unsigned int original_entry_point_vaddr;
};

class TraceReader
{
private:
	std::ifstream ifs;
	std::unique_ptr<TraceData> data;

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

	std::unique_ptr<TraceData> read();
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

	std::unique_ptr<TraceData> trace;
	std::stack<std::shared_ptr<CallStackInfo>> call_stack;
	std::pair<unsigned int, unsigned int> prologue_bb_range;
	std::pair<unsigned int, unsigned int> epilogue_bb_range;

	unsigned int getReturnAddressOfCallInsn(std::shared_ptr<Instruction> call_insn);
	unsigned int detectPrologueEpilogueCodeRegion();

	bool isProgramCode(unsigned int address);
	bool isInPrologueCode(unsigned int bb_id);
	bool isInEpilogueCode(unsigned int bb_id);

public:
	TraceAnalyzer(std::unique_ptr<TraceData> data);

	std::unique_ptr<TraceAnalysisResult> analyze();
};