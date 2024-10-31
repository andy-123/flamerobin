//	Internal SPB class implementation
//	* SPB == Service Parameter Block/Buffer, see Interbase 6.0 C-API

/*
    (C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

    The contents of this file are subject to the IBPP License (the "License");
    you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
    file which must have been distributed along with this file.

    This software, distributed under the License, is distributed on an "AS IS"
    basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
    License for the specific language governing rights and limitations
    under the License.
*/

#ifdef _MSC_VER
#pragma warning(disable: 4786 4996)
#ifndef _DEBUG
#pragma warning(disable: 4702)
#endif
#endif

#include "_ibpp.h"
#include "fb1/ibppfb1.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

const int SPBFb1::BUFFERINCR = 128;

void SPBFb1::Grow(int needed)
{
	if ((mSize + needed) > mAlloc)
	{
		// We need to grow the buffer. We use increments of BUFFERINCR bytes.
		needed = (needed / BUFFERINCR + 1) * BUFFERINCR;
		char* newbuffer = new char[mAlloc + needed];
		if (mBuffer != 0)
		{
			// Move the old buffer content to the new one
			memcpy(newbuffer, mBuffer, mSize);
			delete [] mBuffer;
		}
		mBuffer = newbuffer;
		mAlloc += needed;
	}
}

void SPBFb1::Insert(char opcode)
{
	Grow(1);
	mBuffer[mSize++] = opcode;
}

void SPBFb1::InsertString(char type, int lenwidth, const char* data)
{
	int16_t len = (int16_t)strlen(data);

	Grow(1 + lenwidth + len);
	mBuffer[mSize++] = type;
	switch (lenwidth)
	{
		case 1 :	mBuffer[mSize] = char(len); mSize++; break;
		case 2 :	*(int16_t*)&mBuffer[mSize] = int16_t((*gds.Call()->m_vax_integer)((char*)&len, 2));
					mSize += 2; break;
		default :	throw LogicExceptionImpl("IISPBFb1::IISPB", _("Invalid length parameter"));
	}
	strncpy(&mBuffer[mSize], data, len);
	mSize += len;
}

void SPBFb1::InsertByte(char type, char data)
{
	Grow(1 + 1);
	mBuffer[mSize++] = type;
	mBuffer[mSize++] = data;
}

void SPBFb1::InsertQuad(char type, int32_t data)
{
	Grow(1 + 4);
	mBuffer[mSize++] = type;
	*(int32_t*)&mBuffer[mSize] = int32_t((*gds.Call()->m_vax_integer)((char*)&data, 4));
	mSize += 4;
}

void SPBFb1::Reset()
{
	if (mBuffer != 0)
	{
		delete [] mBuffer;
		mBuffer = 0;
		mSize = 0;
		mAlloc = 0;
    }
}

/*
void SPBFb1::Insert(char type, short data)
{
	Grow(1 + 3);
	mBuffer[mSize++] = type;
	mBuffer[mSize++] = char(2);
	*(short*)&mBuffer[mSize] = data;
	mSize += 2;
}

void SPBFb1::Insert(char type, bool data)
{
	Grow(1 + 2);
	mBuffer[mSize++] = type;
	mBuffer[mSize++] = char(1);
	mBuffer[mSize++] = char(data ? 1 : 0);
}
*/