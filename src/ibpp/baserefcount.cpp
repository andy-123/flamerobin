//  base class - refcount implementation
/*
    (C) Copyright 2000-2024 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

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

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;
using namespace IBPP;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

template <class T>
T* IBaseRefCount<T>::AddRef()
{
    ASSERTION(mRefCount >= 0);
    ++mRefCount;
    return dynamic_cast<T*>(this);
}

template <class T>
void IBaseRefCount<T>::Release()
{
    // Release cannot throw, except in DEBUG builds on assertion
    ASSERTION(mRefCount >= 0);
    --mRefCount;
    try { if (mRefCount <= 0) delete (T*)this; }
        catch (...) { }
}

// No need to call this TemporaryFunction() function,
// it's just to avoid link error.
void TemporaryFunction ()
{
    IBaseRefCount<IArray> TempBlobObj;
    IBaseRefCount<IBlob> TempArrayObj;
    IBaseRefCount<IDatabase> TempDatabaseObj;
    IBaseRefCount<IEvents> TempEventsObj;
    IBaseRefCount<IService> TempServiceObj;
    IBaseRefCount<IStatement> TempStatementObj;
    IBaseRefCount<ITransaction> TempTransactionObj;
    IBaseRefCount<IRow> TempRowObj;
}
