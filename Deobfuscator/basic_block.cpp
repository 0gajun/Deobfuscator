#include "stdafx.h"
#include "trace.h"

BasicBlock::BasicBlock(const int id) : original_id(id)
{
	this->block_id_set.insert(id);
}
