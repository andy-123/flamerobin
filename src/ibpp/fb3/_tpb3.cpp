//	Internal TPB class implementation
//	* TPB == Transaction Parameter Block/Buffer, see Interbase 6.0 C-API

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
using namespace Firebird;

void TPBFb3::Clear()
{
    mTPB->clear(mStatus);
}

const unsigned char* TPBFb3::GetBuffer()
{
    return mTPB->getBuffer(mStatus);
}

unsigned TPBFb3::GetBufferLength()
{
    return mTPB->getBufferLength(mStatus);
}

void TPBFb3::InsertTag(const unsigned char tag)
{
    mTPB->insertTag(mStatus, tag);
}

void TPBFb3::InsertString(unsigned char tag, const std::string& str)
{
    mTPB->insertString(mStatus, tag, str.c_str());
}

TPBFb3::TPBFb3()
{
    IMaster* master = FactoriesImplFb3::gMaster;
    IUtil* util = FactoriesImplFb3::gUtil;

    mStatus = new ThrowStatusWrapper(master->getStatus());
    mTPB = util->getXpbBuilder(mStatus, IXpbBuilder::TPB, NULL, 0);
}

TPBFb3::~TPBFb3()
{
    mTPB = nullptr;
    delete mStatus;
}
