#pragma once
#include <algorithm>

#define FIND(arrayName) (std::find(std::begin(arrayName), std::end(arrayName), B) != std::end(arrayName))
#define B *b //Byte
#define R (*b >> 4) //Row
#define C (*b & 0xF) //Column
static constexpr uint8_t prefixes[] = { 0xF0, 0xF2, 0xF3, 0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65, 0x66, 0x67 };
static constexpr uint8_t op1modrm[] = { 0x62, 0x63, 0x69, 0x6B, 0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE, 0xFF };
static constexpr uint8_t op1imm8[] = { 0x6A, 0x6B, 0x80, 0x82, 0x83, 0xA8, 0xC0, 0xC1, 0xC6, 0xCD, 0xEB };
static constexpr uint8_t op1imm32[] = { 0x68, 0x69, 0x81, 0xA9, 0xC7, 0xE8, 0xE9 };
static constexpr uint8_t op2modrm[] = { 0x02, 0x03, 0x0D, 0xA3, 0xA4, 0xA5, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };

size_t ldisasm(const void* const address)
{
	size_t offset = 0;
	bool operandPrefix = false, addressPrefix = false, rexW = false;
	const uint8_t* b = reinterpret_cast<const uint8_t*>(address);

	auto parseModRM = [&]() {
		uint8_t modrm = *++b;

		bool hasSIB = false; //Check for SIB byte
		if (B < 0xC0 && (B & 0b111) == 0b100)
			hasSIB = true, b++;

		if (modrm >= 0x40 && modrm <= 0x7F) // disp8 (ModR/M)
			b++;
		else if ((modrm <= 0x3F && (modrm & 0b111) == 0b101) || (modrm >= 0x80 && modrm <= 0xBF)) //disp32 (ModR/M)
			b += 4;
		else if (hasSIB && (B & 0b111) == 0b101) //disp8,32 (SIB)
			b += (modrm & 0b01000000) ? 1 : 4;
	};

	//Parse legacy prefixes & REX prefixes
#ifdef _WIN64
		for (int i = 0; i < 15 && (FIND(prefixes) || R == 4); i++, b++)
#else
		for (int i = 0; i < 15 && FIND(prefixes); i++, b++)
#endif
	{
		if (B == 0x66)
			operandPrefix = true;
		else if (B == 0x67)
			addressPrefix = true;
		else if (R == 4 && C >= 8)
			rexW = true;
	}

	//Parse opcode(s)
	if (B == 0x0F) // 2,3 bytes
	{
		b++;
		if (B == 0x38 || B == 0x3A) // 3 bytes
		{
			b++;
			parseModRM();

			if (B == 0x3A)
				offset++;
		}
		else // 2 bytes
		{
			if (R == 8) //disp32
				offset += 4;
			else if ((R == 7 && C < 4) || B == 0xA4 || B == 0xC2 || (B > 0xC3 && B <= 0xC6)) //imm8
				offset++;

			//Check for ModR/M, SIB and displacement
			if (FIND(op2modrm) || (R != 3 && R > 0 && R < 7) || B >= 0xD0 || (R == 7 && C != 7) || R == 9 || R == 0xB || (R == 0xC && C < 8))
				parseModRM();
		}
	}
	else // 1 byte
	{
		//Check for immediate field
		if ((R == 0xE && C < 8) || (R == 0xB && C < 8) || R == 7 || (R < 4 && (C == 4 || C == 0xC)) || (B == 0xF6 && !(*(b + 1) & 56)) || FIND(op1imm8)) //imm8
			offset++;
		else if (B == 0xC2 || B == 0xCA) //imm16
			offset += 2;
		else if (B == 0xC8) //imm16 + imm8
			offset += 3;
		else if ((R < 4 && (C == 5 || C == 0xD)) || (R == 0xA && C < 4) || (R == 0xB && C >= 8) || (B == 0xF7 && !(*(b + 1) & 56)) || FIND(op1imm32)) //imm32,16
			offset += (rexW) ? 8 : ((operandPrefix || addressPrefix) ? 2 : 4);
		else if (B == 0xEA || B == 0x9A) //imm32,48
			offset += operandPrefix ? 4 : 6;

		//Check for ModR/M, SIB and displacement
		if (FIND(op1modrm) || (R < 4 && (C < 4 || (C >= 8 && C < 0xC))) || R == 8 || (R == 0xD && C >= 8))
			parseModRM();
	}

	return static_cast<size_t>(reinterpret_cast<ptrdiff_t>(++b + offset) - reinterpret_cast<ptrdiff_t>(address));
}