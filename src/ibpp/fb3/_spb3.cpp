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
#include "ibppfb3.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

void SPBFb3::Clear()
{
    mSPB->clear(mStatus);
}

const unsigned char* SPBFb3::GetBuffer()
{
    return mSPB->getBuffer(mStatus);
}

unsigned SPBFb3::GetBufferLength()
{
    return mSPB->getBufferLength(mStatus);
}

void SPBFb3::InsertTag(const unsigned char tag)
{
    mSPB->insertTag(mStatus, tag);
}

void SPBFb3::InsertString(unsigned char tag, const std::string& str)
{
    mSPB->insertString(mStatus, tag, str.c_str());
}

void SPBFb3::InsertByte(unsigned char tag, int8_t data)
{
    mSPB->insertBytes(mStatus, tag, &data, sizeof(data));
}

void SPBFb3::InsertQuad(unsigned char tag, int32_t data)
{
    mSPB->insertInt(mStatus, tag, data);
}

SPBFb3::SPBFb3(SPBType type)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    IUtil* util = FactoriesImplFb3::gUtil;

    int xpbType;
    switch (type)
    {
        case attach :
            { xpbType = IXpbBuilder::SPB_ATTACH; break; }
        case service_start :
            { xpbType = IXpbBuilder::SPB_START; break; }
        case query_send :
            { xpbType = IXpbBuilder::SPB_SEND; break; }
        case query_receive :
            { xpbType = IXpbBuilder::SPB_RECEIVE; break; }
        default:
          throw LogicExceptionImpl("SPBFb3::SPBFb3", _("Invalid type parameter"));
    }
    
    mStatus = new ThrowStatusWrapper(master->getStatus());
    mSPB = util->getXpbBuilder(mStatus, xpbType, NULL, 0);
}

SPBFb3::~SPBFb3()
{
    mSPB = nullptr;
    delete mStatus;
}
