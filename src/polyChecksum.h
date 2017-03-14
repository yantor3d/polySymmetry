/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_CHECKSUM_H
#define POLY_CHECKSUM_H

// based on code found at http://www.relisoft.com/science/CrcOptim.html

class PolyChecksum
{
public:
                        PolyChecksum();
    virtual void        putBytes(void* bytes, size_t dataSize);
    virtual int         getResult();

public:
	unsigned long       _table[256];
	unsigned long       _register = 0;
	unsigned long       _key = 0x04c11db7;
};

#endif