#pragma once

#include <Windows.h>
#include <vector>

namespace Patching
{

	struct Patch
	{
		uintptr_t addr; // original address of the bytes
		uintptr_t newMem; // a.k.a where the program will jump to 
		std::vector<BYTE> newInstructions; // this will contain originalBytes, prepended at the beginning
		std::vector<BYTE> originalBytes; // this will hold the original bytes to replace at addr, disabling the patch.
		size_t newMemSize;
	};

	// NOTE: adds JMP automatically
	// Instruction Length => the size of the 2 instructions we are overriding for the JMP (much like CE does it)
	Patch* InstallPatch(
		uintptr_t addr, 
		size_t originalInstructionLength, 
		const std::vector<BYTE> &newInstructions, 
		size_t newMemSize = 1024, 
		bool copyOldInstructions = true);

	bool RemovePatch(Patch* patch);
}