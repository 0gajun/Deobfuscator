#include "stdafx.h"
#include "trace.h"
#include "string_util.h"
#include <thread>
#include <sstream>

#include <iostream>

#define LOG_DELIMITER "="
#define ADDRESS_LEN_IN_LOGFILE 10

#define NUM_OF_CHARACTER_IN_A_BYTE 2

TraceReader::TraceReader()
{
	this->parse_task_queue = std::make_unique<ParseTaskQueue>(TASK_QUEUE_SIZE);
	this->data = std::make_unique<TraceData>();
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

std::unique_ptr<TraceData> TraceReader::read()
{
	if (!ifs.is_open()) {
		throw std::exception::exception("file isn't opened.");
	}

	size_t mp = std::thread::hardware_concurrency();
	if (mp == 0) {
		mp = 1;
	}
	std::vector<std::thread> threads(mp);

	for (std::thread& th : threads) {
		th = std::thread(&TraceReader::parseAndRegistrationBasicBlockWorker, this);
	}

	unsigned int bb_id = 0;
	std::string line;
	std::string code_bytes;
	std::vector<std::string> insn_buffer;
	std::shared_ptr<BasicBlock> prev_bb;
	while (std::getline(ifs, line)) {
		if (line.empty()) {
			continue;
		}
		if (line == LOG_DELIMITER) {
			std::getline(ifs, code_bytes);

			std::shared_ptr<TraceReader::ParseTask> task
				= std::make_shared<TraceReader::ParseTask>(bb_id, insn_buffer, code_bytes);
			parse_task_queue->enqueue(task);

			insn_buffer.clear();
			code_bytes.clear();
			bb_id++;
			continue;
		}
		insn_buffer.push_back(line);
	}

	parse_task_queue->setFinished();

	for (std::thread& th : threads) {
		th.join();
	}

	// set flow information
	for (unsigned int id = 0; id < data->basic_blocks.size(); id++) {
		std::shared_ptr<BasicBlock> bb = data->basic_blocks.at(id);
		if (prev_bb) {
			prev_bb->next_bbs.insert(std::unordered_map<int, std::shared_ptr<BasicBlock>>::value_type(id, bb));
			prev_bb->next_bb_addr = bb->head_insn_addr;
			bb->prev_bbs.insert(std::unordered_map<int, std::shared_ptr<BasicBlock>>::value_type(id - 1, prev_bb));
		}
		prev_bb = bb;
	}

	return std::move(data);
}

void TraceReader::parseAndRegistrationBasicBlockWorker()
{
	while (1) {
		std::shared_ptr<TraceReader::ParseTask> task = parse_task_queue->dequeue();
		if (!task) {
			// finished
			return;
		}
		registerBasicBlock(parseBasicBlock(task->bb_id, task->insn_buffer, task->code_bytes));
	}
}

// register basic block to Reader's TraceData object
// return: registered basic block
std::shared_ptr<BasicBlock> TraceReader::registerBasicBlock(std::shared_ptr<BasicBlock> bb)
{
	if (existsSameBasicBlock(bb)) {
		return registerExistingBasicBlock(bb);
	}
	else {
		return registerNewBasicBlock(bb);
	}
}

std::shared_ptr<BasicBlock> TraceReader::registerNewBasicBlock(std::shared_ptr<BasicBlock> bb)
{
	auto bbs_entry = std::map<unsigned int, std::shared_ptr<BasicBlock>>::value_type(bb->original_id, bb);
	auto bb_map_entry = std::map<unsigned int, std::shared_ptr<BasicBlock>>::value_type(bb->head_insn_addr, bb);

	{
		std::lock_guard<std::mutex> lock(bb_registration_mutex);
		data->basic_blocks.insert(bbs_entry);
		data->addr_bb_map.insert(bb_map_entry);
	}

	return bb;
}

std::shared_ptr<BasicBlock> TraceReader::registerExistingBasicBlock(std::shared_ptr<BasicBlock> bb)
{
	std::shared_ptr<BasicBlock> existing_bb = data->addr_bb_map[bb->head_insn_addr];
	existing_bb->block_id_set.insert(bb->original_id);
	auto bbs_entry = std::map<unsigned int, std::shared_ptr<BasicBlock>>::value_type(bb->original_id, existing_bb);

	{
		std::lock_guard<std::mutex> lock(bb_registration_mutex);
		data->basic_blocks.insert(bbs_entry);
	}

	return existing_bb;
}

bool TraceReader::existsSameBasicBlock(std::shared_ptr<BasicBlock> bb)
{
	return data->addr_bb_map.find(bb->head_insn_addr) != data->addr_bb_map.end();
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

	std::vector<std::string> insn_chunks = StringUtil::split(insn_str, ' ');
	// insn_str format of logfile is like this
	//
	// 0x8308594c:    lock       xadd       %esi,(%eax)
	// |  addr  |:__|(prefix)|_|opcode|__|  (operands)  | () <- is optional


	if (insn_chunks.size() < 2) {
		// insn_str should have two chunks(address and opcode) at least.
		//TODO: error handling correctly
		std::cout << "invalid insn format: " << insn_str << std::endl;
		exit(1);
	}

	insn->addr = std::stoul(insn_chunks[0].substr(0, 10), nullptr, 16);

	if (existsOperand(insn_chunks)) {
		std::string last_chunk = insn_chunks[insn_chunks.size() - 1];
		insn->operands = StringUtil::split(last_chunk, ',');
		insn_chunks.pop_back();
	}

	switch (insn_chunks.size())
	{
	case 2: // only opcode case
		insn->opcode = insn_chunks[1];
		break;
	case 3: // with prefix case
		insn->prefix = insn_chunks[1];
		insn->opcode = insn_chunks[2];
		break;
	case 1: // only address (unexpected case)
	default: // unexpected case
		// TODO: error handling correctly
		break;
	}
	return insn;
}

bool TraceReader::existsOperand(std::vector<std::string> insn_chunks)
{
	std::string last_chunk = insn_chunks[insn_chunks.size() - 1];

	return last_chunk.find("0x") != std::string::npos // including address or offset
			|| last_chunk.find("%") != std::string::npos // including register
			|| last_chunk.find("$") != std::string::npos; // including immediate value
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

TraceReader::ParseTask::ParseTask(unsigned int bb_id, std::vector<std::string> insn_buffer, std::string code)
	: bb_id(bb_id), insn_buffer(insn_buffer), code_bytes(code)
{
}

TraceReader::ParseTaskQueue::ParseTaskQueue(int capacity) : capacity(capacity)
{
}

void TraceReader::ParseTaskQueue::enqueue(std::shared_ptr<ParseTask> t)
{
	std::unique_lock<std::mutex> lock(m);
	c_enq.wait(lock, [this] { return data.size() != capacity; });
	data.push(t);
	c_deq.notify_one();
}

std::shared_ptr<TraceReader::ParseTask> TraceReader::ParseTaskQueue::dequeue()
{
	std::unique_lock<std::mutex> lock(m);
	c_deq.wait(lock, [this] { return !data.empty() || is_finished; });
	if (is_finished) {
		return nullptr;
	}
	std::shared_ptr<TraceReader::ParseTask> ret = data.front();
	data.pop();
	c_enq.notify_one();
	return ret;
}

void TraceReader::ParseTaskQueue::setFinished()
{
	this->is_finished = true;
	c_deq.notify_all();
}

size_t TraceReader::ParseTaskQueue::size()
{
	return data.size();
}