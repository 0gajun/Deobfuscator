#pragma once
#include "pe.h"
#include "trace.h"
#include <memory>
#include <list>

// defined in trace.h
class BasicBlock;
// defined in pe.h
class PEEditor;

class PECommand abstract
{
public:
	virtual void execute(std::shared_ptr<PEEditor> editor) = 0;
	virtual bool isCommandInProgram() = 0;
};

class CreateShadowSectionCommand : public PECommand
{
protected:
	std::vector<char> code_buffer;
public:
	unsigned int requiredSize();
};

class NonReturningCallCommand : public CreateShadowSectionCommand
{
private:
	const std::shared_ptr<BasicBlock> caller_bb;
	const std::shared_ptr<BasicBlock> callee_bb;
	const std::shared_ptr<BasicBlock> jmp_target_bb;

public:
	NonReturningCallCommand(std::shared_ptr<BasicBlock> caller_bb,
		std::shared_ptr<BasicBlock> callee_bb, std::shared_ptr<BasicBlock> jmp_target_bb);
	void execute(std::shared_ptr<PEEditor> editor) override;
	bool isCommandInProgram() override;
};

class CallStackTamperingCommand : public CreateShadowSectionCommand
{

};

class OverlappingFunctionAndBasicBlockCommand : public CreateShadowSectionCommand
{
private:
	const std::shared_ptr<BasicBlock> prev_bb;
	const std::vector<std::shared_ptr<BasicBlock>> overlapped_bbs;
	const std::shared_ptr<BasicBlock> next_bb;

	bool isNeededNearJmp(std::shared_ptr<PEEditor> editor);
public:
	OverlappingFunctionAndBasicBlockCommand(const std::shared_ptr<BasicBlock> prev_bb,
		const std::vector<std::shared_ptr<BasicBlock>> overlapped_bbs, const std::shared_ptr<BasicBlock> next_bb);

	void execute(std::shared_ptr<PEEditor> editor) override;
	bool isCommandInProgram() override;
};

class RedundantJmpReductionCommand : public PECommand
{
private:
	const std::shared_ptr<BasicBlock> from_bb;
	const std::shared_ptr<BasicBlock> to_bb;
public:
	RedundantJmpReductionCommand(std::shared_ptr<BasicBlock> from_bb, std::shared_ptr<BasicBlock> to_bb);
	void execute(std::shared_ptr<PEEditor> editor) override;
	bool isCommandInProgram() override;
};

class CommandInvoker
{
private:
	std::list<std::shared_ptr<PECommand>> commands;

	void preInvoke(std::shared_ptr<PEEditor> editor);
	unsigned int estimateShadowSectionSize();
public:
	void addCommand(std::shared_ptr<PECommand> command);
	void invoke(std::shared_ptr<PEEditor> editor);
	bool hasCommand();
};