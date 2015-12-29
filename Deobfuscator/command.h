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
};

class CallStackTamperingCommand : public CreateShadowSectionCommand
{

};

class OverlappingFunctionAndBasicBlockCommand : public PECommand
{

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