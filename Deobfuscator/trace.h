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
#include <queue>
#include <mutex>

#define TASK_QUEUE_SIZE 100

#define SHORT_JMP_INSN_SIZE 2
#define NEAR_JMP_INSN_SIZE 5
#define OPCODE_SHORT_JMP 0xEB
#define OPCODE_NEAR_JMP 0xE9
#define OPCODE_NOP 0x90

class CommandInvoker;
class PECommnad;
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
		static unsigned int estimateJmpInstructionSize(std::shared_ptr<PEEditor> editor, unsigned int insn_addr, unsigned int jmp_target_addr);
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
	unsigned int oep;
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
	class ParseTask
	{
	public:
		unsigned int bb_id;
		std::vector<std::string> insn_buffer;
		std::string code_bytes;

		explicit ParseTask(unsigned int bb_id, std::vector<std::string> insn_buffer, std::string code);
	};

	class ParseTaskQueue
	{
	private:
		std::mutex m;
		std::queue<std::shared_ptr<ParseTask>> data;
		size_t capacity;
		std::condition_variable c_enq;
		std::condition_variable c_deq;
		bool is_finished;
	public:
		explicit ParseTaskQueue(int capacity);
		size_t size();
		void enqueue(std::shared_ptr<ParseTask> t);
		std::shared_ptr<ParseTask> dequeue();
		void setFinished();
	};

	std::ifstream ifs;
	std::unique_ptr<TraceData> data;
	std::unique_ptr<ParseTaskQueue> parse_task_queue;

	std::mutex bb_registration_mutex;


	void parseAndRegistrationBasicBlockWorker();
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

#define TRACE_ANALYZER_ENABLE_OVERLAP_FUNC 0x00000010

class DisassembleResult;
class PEFormat;
class PECommand;

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
	unsigned int option_flags;

	const std::unique_ptr<TraceData> trace;
	std::unique_ptr<PEFormat> pe_fmt;
	std::stack<std::shared_ptr<CallStackInfo>> call_stack;
	std::pair<unsigned int, unsigned int> prologue_bb_range;
	std::pair<unsigned int, unsigned int> epilogue_bb_range;

	std::unique_ptr<DisassembleResult> disassembler_result;

	unsigned int getReturnAddressOfCallInsn(std::shared_ptr<Instruction> call_insn);
	unsigned int detectPrologueEpilogueCodeRegion();

	bool shouldIssueCommand(std::shared_ptr<PECommand> cmd);

	bool isInPrologueCode(unsigned int bb_id);
	bool isInEpilogueCode(unsigned int bb_id);

	// For overlapping and basic blocks
	bool isOverlapped(unsigned int actual_addr);

public:
	TraceAnalyzer(std::unique_ptr<TraceData> data, PEFormat pe_fmt);

	void setDisassembleResult(std::unique_ptr<DisassembleResult> result);

	std::unique_ptr<TraceAnalysisResult> analyze();
};