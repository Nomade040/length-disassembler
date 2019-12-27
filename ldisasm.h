#pragma once
#include <cstdint>
#define OPROW (*b >> 4) // Four high-order bits of an opcode to index a row of the opcode table
#define OPCOL (*b & 0xF) // Four low-order bits to index a column of the table

static constexpr uint8_t legacyPrefixes[] = { 0xF0, 0xF2, 0xF3, 0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65, 0x66, 0x67 };
static constexpr uint8_t op1modrm[] = { 0x62, 0x63, 0x69, 0x6B, 0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE, 0xFF };
static constexpr uint8_t op1imm8[] = { 0x6A, 0x6B, 0x80, 0x82, 0x83, 0xA8, 0xC0, 0xC1, 0xC6, 0xCD, 0xD4, 0xD5, 0xEB };
static constexpr uint8_t op1imm32[] = { 0x68, 0x69, 0x81, 0xA9, 0xC7, 0xE8, 0xE9 };
static constexpr uint8_t op2modrm[] = { 0x0D, 0xA3, 0xA4, 0xA5, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };

template <size_t N> bool findOpcode(const uint8_t(&arr)[N], const uint8_t x) { for (size_t i = 0; i < N; i++) { if (arr[i] == x) { return true; } }; return false; }

size_t ldisasm(const void* const address, const bool x86_64_mode = false)
{
	size_t offset = 0; 
	bool operandPrefix = false, addressPrefix = false, rexW = false;
	const uint8_t* b = reinterpret_cast<const uint8_t*>(address);

	auto parseModRM = [&]()
	{
		uint8_t modrm = *++b;

		if (!addressPrefix || (addressPrefix && *b >= 0x40))
		{
			bool hasSIB = false; //Check for SIB byte
			if (*b < 0xC0 && (*b & 0b111) == 0b100 && !addressPrefix)
				hasSIB = true, b++;

			if (modrm >= 0x40 && modrm <= 0x7F) // disp8 (ModR/M)
				b++;
			else if ((modrm <= 0x3F && (modrm & 0b111) == 0b101) || (modrm >= 0x80 && modrm <= 0xBF)) //disp16,32 (ModR/M)
				b += (addressPrefix) ? 2 : 4;
			else if (hasSIB && (*b & 0b111) == 0b101) //disp8,32 (SIB)
				b += (modrm & 0b01000000) ? 1 : 4;
		}
		else if (addressPrefix && modrm == 0x26)
			b += 2;
	};

	//Parse legacy prefixes & REX prefixes
	for(; findOpcode(legacyPrefixes, *b) || ((x86_64_mode) ? (OPROW == 4) : false); b++)
	{
		if (*b == 0x66)
			operandPrefix = true;
		else if (*b == 0x67)
			addressPrefix = true;
		else if (OPROW == 4 && OPCOL >= 8)
			rexW = true;
	}

	//Parse opcode(s)
	if (*b == 0x0F) // 2,3 bytes
	{
		b++;
		if (*b == 0x38 || *b == 0x3A) // 3 bytes
		{
			if (*b++ == 0x3A)
				offset++;

			parseModRM();
		}
		else // 2 bytes
		{
			if (OPROW == 8) //disp32
				offset += 4;
			else if ((OPROW == 7 && OPCOL < 4) || *b == 0xA4 || *b == 0xC2 || (*b > 0xC3 && *b <= 0xC6) || *b == 0xBA || *b == 0xAC) //imm8
				offset++;

			//Check for ModR/M, SIB and displacement
			if (findOpcode(op2modrm, *b) || (OPROW != 3 && OPROW > 0 && OPROW < 7) || *b >= 0xD0 || (OPROW == 7 && OPCOL != 7) || OPROW == 9 || OPROW == 0xB || (OPROW == 0xC && OPCOL < 8) || (OPROW == 0 && OPCOL < 4))
				parseModRM();
		}
	}
	else // 1 byte
	{
		//Check for immediate field
		if ((OPROW == 0xE && OPCOL < 8) || (OPROW == 0xB && OPCOL < 8) || OPROW == 7 || (OPROW < 4 && (OPCOL == 4 || OPCOL == 0xC)) || (*b == 0xF6 && !(*(b + 1) & 48)) || findOpcode(op1imm8, *b)) //imm8
			offset++;
		else if (*b == 0xC2 || *b == 0xCA) //imm16
			offset += 2;
		else if (*b == 0xC8) //imm16 + imm8
			offset += 3;
		else if ((OPROW < 4 && (OPCOL == 5 || OPCOL == 0xD)) || (OPROW == 0xB && OPCOL >= 8) || (*b == 0xF7 && !(*(b + 1) & 48)) || findOpcode(op1imm32, *b)) //imm32,16
			offset += (rexW) ? 8 : (operandPrefix ? 2 : 4);
		else if (OPROW == 0xA && OPCOL < 4)
			offset += (rexW) ? 8 : (addressPrefix ? 2 : 4);
		else if (*b == 0xEA || *b == 0x9A) //imm32,48
			offset += operandPrefix ? 4 : 6;

		//Check for ModR/M, SIB and displacement
		if (findOpcode(op1modrm, *b) || (OPROW < 4 && (OPCOL < 4 || (OPCOL >= 8 && OPCOL < 0xC))) || OPROW == 8 || (OPROW == 0xD && OPCOL >= 8))
			parseModRM();
	}

	return static_cast<size_t>(reinterpret_cast<ptrdiff_t>(++b + offset) - reinterpret_cast<ptrdiff_t>(address));
}