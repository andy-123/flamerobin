//	Subject : IBPP, internal DPB class implementation
//	COMMENTS
//  * DPB == Database Parameter Block/Buffer, see Interbase 6.0 C-API

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

#include "fb3/ibppfb3.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;
using namespace Firebird;

const int DPBFb3::BUFFERINCR = 128;

void DPBFb3::Clear()
{
    mDPB->clear(mStatus);
}

const unsigned char* DPBFb3::GetBuffer()
{
    return mDPB->getBuffer(mStatus);
}

unsigned DPBFb3::GetBufferLength()
{
    return mDPB->getBufferLength(mStatus);
}

void DPBFb3::InsertString(unsigned char tag, const char* str)
{
    mDPB->insertString(mStatus, tag, str);
}

/*void DPBFb3::Insert(char type, int16_t data)
{
    #ifdef FIXME
	Grow(2 + 2);
    mBuffer[mSize++] = type;
	mBuffer[mSize++] = char(2);
    *(int16_t*)&mBuffer[mSize] = int16_t((*gds.Call()->m_vax_integer)((char*)&data, 2));
    mSize += 2;
    #endif
}*/

void DPBFb3::InsertBool(unsigned char tag, bool data)
{
    unsigned char value = (unsigned char)data;
    mDPB->insertBytes(mStatus, tag, &value, sizeof(value));
}

/*void DPBFb3::Insert(char type, char data)
{
    #ifdef FIXME
	Grow(2 + 1);
    mBuffer[mSize++] = type;
	mBuffer[mSize++] = char(1);
    mBuffer[mSize++] = data;
    #endif
}*/

DPBFb3::DPBFb3()
{
    IMaster* master = FactoriesImplFb3::gMaster;
    IUtil* util = FactoriesImplFb3::gUtil;

    mStatus = new ThrowStatusWrapper(master->getStatus());
    mDPB = util->getXpbBuilder(mStatus, IXpbBuilder::DPB, NULL, 0);
}

DPBFb3::~DPBFb3()
{
    delete mStatus;
}
