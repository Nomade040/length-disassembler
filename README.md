# length-disassembler
A length disassembler for x86-32 and x86-64.

Possible problems
 - Some invalid opcodes may not get parsed correctly(Untested).
 - No support for AVX instructions is present.

Example
```C++
#include "ldisasm.h"

int main()
{
  char buffer[] = { 0x55, 0x8B, 0xEC, 0x53, 0x8B, 0xD9, 0x56, 0x57 };
  size_t instructionLength = disasm(buffer);
}
```

Unexpected results may happen if the disassembler encounters invalid instructions or VEX instructions.

References:
 * Intel® 64 and IA-32 Architectures Software Developer’s Manual Volume 2 (2A, 2B, 2C & 2D): Instruction Set Reference, A-Z
   - Section 2.1
   - Section 2.2.1
   - Appendix A
