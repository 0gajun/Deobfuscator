#include "stdafx.h"
#include "pe.h"

unsigned int PEUtil::getVirtAddrBeforeRelocate(const unsigned int vaddr,
	const unsigned int actual_oep, PEFormat pe_fmt)
{
	unsigned int actual_image_base
		= actual_oep - pe_fmt.nt_headers.OptionalHeader.AddressOfEntryPoint;

	return vaddr > actual_image_base ? vaddr - actual_image_base : vaddr;
}

bool PEUtil::canShortJmp(unsigned int from, unsigned to)
{
	signed int distance = to - from;
	return -128 <= distance && distance < 127;
}

bool PEUtil::isProgramCode(unsigned int address)
{
	// TODO: should use more correctly method, this is temporaly method...orz
	return address < 0x60000000;
}

bool PEUtil::isKernelCode(unsigned int address)
{
	// XXX: this can be correct in normal kernel config.
	// if increaseuserva is set, kernel base address is not 0x80000000. should deal with  this config.
	return address >= 0x80000000;
}