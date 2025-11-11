#include "pch.h"
#include <memory>
#include <string>
#include <stdexcept>
#include <format>
#include "PatchHelpers.h"

#define NOP 0x90
#define JMP 0xE9

// A typical jump instruction will contain the byte 0xE9 + a pointer
// for a total of 5 bytes, this macro is not neccessary but helps with reading
// specific parts of the code to understand why certain numbers are there
#define JMP_SIZE 5

namespace Patching
{
	// placeholder - big endian support may arrive later.
	static bool isLittleEndian()
	{
		// TODO: implement
		return true;
	}

	// appends 5 bytes, jmp [destination - src]
	static void relJump(std::vector<BYTE> &out, uintptr_t src, uintptr_t dst)
	{
		int32_t rel = static_cast<int32_t>(dst - src);
		out.push_back(JMP); // jmp

		// get the relative address as 4 bytes
		out.push_back(static_cast<BYTE>(rel & 0xFF));
		out.push_back(static_cast<BYTE>((rel >> 8) & 0xFF));
		out.push_back(static_cast<BYTE>((rel >> 16) & 0xFF));
		out.push_back(static_cast<BYTE>((rel >> 24) & 0xFF));
	}

	static bool WriteMemoryRaw(void* dst, const void* src, SIZE_T len) {
		// NOTE: addresses are void* here, as opposed to the rest being uintptr_t 
		// because there is no reason to cast on this function since all the types
		// will need to be converted to void* anyways, as memcpy virtualprotect, etc... uses void*
		DWORD old;
		if (!VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &old)) return false;
		memcpy(dst, src, len);
		DWORD tmp;
		VirtualProtect(dst, len, old, &tmp);
		FlushInstructionCache(GetCurrentProcess(), dst, len);
		return true;
	}

	//struct Patch
	//{
	//	uintptr_t addr; // original address of the bytes
	//	uintptr_t newMem; // a.k.a where the program will jump to 
	//	std::vector<BYTE> newInstructions; // this will contain originalBytes, prepended at the beginning
	//	std::vector<BYTE> originalBytes; // this will hold the original bytes to replace at addr, disabling the patch.
	//};

	// InstallPatch is an inherently dangerous function, it changes bytecode instructions at runtime
	// if anything goes wrong here, the game will either crash in a painfully undebuggable way or be lead to undefined behavior.
	// The way it's done should, in theory, be safe, since new memory is created which is where we actually put the new code,
	// but during development I've learned that nothing is safe when you are dealing with raw memory.
	// Another note - this is not fullproof to not crash even when used correctly, as currently, there is no mechanism that's stopping
	// read operations on instructions while they're being written or flushed, while this is not tested and
	// will *probably* never happen; but if it does, the game will most likely crash or burn with undefined behavior.
	Patch* InstallPatch(
		uintptr_t addr, 
		size_t originalInstructionLength, 
		const std::vector<BYTE> &newInstructions, 
		size_t newMemSize, 
		bool copyOldInstructions)
	{
		auto newMemory = reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, newMemSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		auto originalBytes = std::vector<BYTE>();

		// tramp is the new instructions (including the jmp back to normal execution)
		// it's error prone to change the newInstructions vector as it comes from constant
		// byte vectors that hold instructions per cheat, this means that changing those will corrupt
		// the cheat after the first time it's enabled and either crash the game or corrupt memory.
		auto tramp = newInstructions; 
		originalBytes.resize(originalInstructionLength);

		memcpy(originalBytes.data(), reinterpret_cast<void*>(addr), originalInstructionLength);

		std::string debugStr = "Original Memory:\n";

		for (size_t i = 0; i < originalBytes.size(); ++i) {
			debugStr += std::format("0x{:02X}", originalBytes[i]);
			if (i + 1 < originalBytes.size())
				debugStr += ' ';
		}

		OutputDebugStringA(debugStr.c_str());

		// append old instructions to newInstruction vector (similar to how CE does it, tho with CE this can be disabled, might make
		// a new function for installing a patch and discarding the old instructions)
		if (copyOldInstructions)
			tramp.insert(tramp.end(), originalBytes.begin(), originalBytes.end());

		// allocate new memory to jump to
		if (!newMemory)
		{
			OutputDebugStringA("Patching::InstallPatch - Failed to allocate new memory, VirtualAlloc returned nullptr.");
			return nullptr;
		}

		if (!isLittleEndian())
		{
			// this is a temporary don't crash for big endian systems which will probably never be relevant but
			// for now I'm just fixing it out since (at the time of writing) I'm mostly trying to get a working prototype for
			// bytecode injection
			OutputDebugStringA("Patching::InstallPatch - Big Endian detected, disabling patching.");
			return nullptr;
		}

		// jmp back to original after the JMP
		relJump(tramp, newMemory + tramp.size() + 5, addr + 5);

		memcpy(reinterpret_cast<void*>(newMemory), tramp.data(), tramp.size());

		// replacement bytes for original, a jmp and NOPs 
		auto patchBuffer = std::vector<BYTE>();
		// patchBuffer.resize(originalInstructionLength);
		// JMP to newmemory
		relJump(patchBuffer, addr, newMemory);

		// fill the rest of the instructions with NOPs
		if (originalInstructionLength > 5)
			patchBuffer.insert(patchBuffer.end(), originalInstructionLength - 5, NOP);

		// Write patch to memory
		if (!WriteMemoryRaw(reinterpret_cast<void*>(addr), patchBuffer.data(), patchBuffer.size()))
		{
			VirtualFree(reinterpret_cast<void*>(newMemory), 0, MEM_RELEASE);
			return nullptr;
		}

		Patch* p = new Patch();
		p->addr = addr;
		p->newInstructions = newInstructions;
		p->newMem = newMemory;
		p->newMemSize = newMemSize;
		p->originalBytes = originalBytes;
		return p;
	}

	// Note: DELETES PATCH POINTER IF SUCCESSFUL
	bool RemovePatch(Patch* patch)
	{
		if (!patch)
			return false;

		// re-write old memory
		bool ok = WriteMemoryRaw(reinterpret_cast<void*>(patch->addr), patch->originalBytes.data(), patch->originalBytes.size());

		// release old memory
		if(patch->newMem)
			VirtualFree(reinterpret_cast<void*>(patch->newMem), 0, MEM_RELEASE);

		delete patch;
		return true;
	}
}

#undef NOP
#undef JMP