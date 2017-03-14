#include "polyChecksum.h"

PolyChecksum::PolyChecksum()
{
	// for all possible byte values
	for (unsigned i = 0; i < 256; ++i)
	{
		unsigned long reg = i << 24;
		// for all bits in a byte
		for (int j = 0; j < 8; ++j)
		{
			bool topBit = (reg & 0x80000000) != 0;
			reg <<= 1;

			if (topBit)
				reg ^= _key;
		}
		_table [i] = reg;
	}
}

void PolyChecksum::putBytes(void* bytes, size_t dataSize)
{
    unsigned char* ptr = (unsigned char*) bytes;

    for (size_t i = 0; i < dataSize; i++)
    {
        unsigned byte = *(ptr + i);
        unsigned top = _register >> 24;
        top ^= byte;
        top &= 255;

        _register = (_register << 8) ^ _table [top];
    }
}

int PolyChecksum::getResult()
{
    return (int) this->_register;
}
