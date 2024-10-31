///////////////////////////////////////////////////////////////////////////////
//
//	Subject : IBPP, Row class implementation
//
///////////////////////////////////////////////////////////////////////////////
//
//	(C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)
//
//	The contents of this file are subject to the IBPP License (the "License");
//	you may not use this file except in compliance with the License.  You may
//	obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
//	file which must have been distributed along with this file.
//
//	This software, distributed under the License, is distributed on an "AS IS"
//	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
//	License for the specific language governing rights and limitations
//	under the License.
//

#ifdef _MSC_VER
#pragma warning(disable: 4786 4996)
#ifndef _DEBUG
#pragma warning(disable: 4702)
#endif
#endif

#include "_ibpp.h"
#include "_utils.h"
#include "ibppfb3.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <cmath>
#include <ctime>

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void RowImplFb3::CheckIsInit(const char* callername)
{
    if (mMeta == nullptr)
        throw LogicExceptionImpl(callername, _("The row is not initialized."));
}

void RowImplFb3::CheckColnum(const char* callername, int colnum)
{
    if (colnum < 1 || colnum > mColCount)
        throw LogicExceptionImpl(callername, _("Variable index out of range."));
}

void RowImplFb3::SetNull(int param)
{
    CheckIsInit("Row::SetNull");

    PCOLINFO col = &mColumns[param-1];
    if (!col->isNullable)
        throw LogicExceptionImpl("Row::SetNull", _("This column can't be null."));

    // Set the column to SQL NULL
    *col->notNullPtr = -1;
    col->data.updated = true;
}

void RowImplFb3::Set(int param, bool value)
{
    CheckIsInit("Row::Set[bool]");

    SetValue(param, ivBool, &value);
}

void RowImplFb3::Set(int param, const char* cstring)
{
    CheckIsInit("Row::Set[char*]");
    if (cstring == 0)
        throw LogicExceptionImpl("Row::Set[char*]", _("null char* pointer detected."));

    SetValue(param, ivByte, cstring, (int)strlen(cstring));
}

void RowImplFb3::Set(int param, const void* bindata, int len)
{
    CheckIsInit("Row::Set[void*]");
    if (bindata == 0)
        throw LogicExceptionImpl("Row::Set[void*]", _("null char* pointer detected."));
    if (len < 0)
        throw LogicExceptionImpl("Row::Set[void*]", _("Length must be >= 0"));

    SetValue(param, ivByte, bindata, len);
}

void RowImplFb3::Set(int param, const std::string& s)
{
    CheckIsInit("Row::Set[string]");

    SetValue(param, ivString, (void*)&s);
}

void RowImplFb3::Set(int param, int16_t value)
{
    CheckIsInit("Row::Set[int16_t]");

    SetValue(param, ivInt16, &value);
}

void RowImplFb3::Set(int param, int32_t value)
{
    CheckIsInit("Row::Set[int32_t]");

    SetValue(param, ivInt32, &value);
}

void RowImplFb3::Set(int param, int64_t value)
{
    CheckIsInit("Row::Set[int64_t]");

    SetValue(param, ivInt64, &value);
}

void RowImplFb3::Set(int param, IBPP::ibpp_int128_t value)
{
    CheckIsInit("Row::Set[int128_t]");

    SetValue(param, ivInt128, &value);
}

void RowImplFb3::Set(int param, float value)
{
    CheckIsInit("Row::Set[float]");

    SetValue(param, ivFloat, &value);
}

void RowImplFb3::Set(int param, double value)
{
    CheckIsInit("Row::Set[double]");

    SetValue(param, ivDouble, &value);
}

void RowImplFb3::Set(int param, IBPP::ibpp_dec16_t value)
{
    CheckIsInit("Row::Set[dec16_t]");

    SetValue(param, ivDec16, &value);
}

void RowImplFb3::Set(int param, IBPP::ibpp_dec34_t value)
{
    CheckIsInit("Row::Set[dec34_t]");

    SetValue(param, ivDec34, &value);
}

void RowImplFb3::Set(int param, const IBPP::Timestamp& value)
{
    CheckIsInit("Row::Set[Timestamp]");

    SetValue(param, ivTimestamp, &value);
}

void RowImplFb3::Set(int param, const IBPP::Date& value)
{
    CheckIsInit("Row::Set[Date]");

    if (mDialect == 1)
    {
        // In dialect 1, IBPP::Date is supposed to work with old 'DATE'
        // fields which are actually ISC_TIMESTAMP.
        IBPP::Timestamp timestamp(value);
        SetValue(param, ivTimestamp, &timestamp);
    }
    else
    {
        // Dialect 3
        SetValue(param, ivDate, (void*)&value);
    }
}

void RowImplFb3::Set(int param, const IBPP::Time& value)
{
    CheckIsInit("Row::Set[Time]");
    if (mDialect == 1)
        throw LogicExceptionImpl("Row::Set[Time]", _("Requires use of a dialect 3 database."));

    SetValue(param, ivTime, &value);
}

void RowImplFb3::Set(int param, const IBPP::Blob& blob)
{
    CheckIsInit("Row::Set[Blob]");
    if (mDatabase != 0 && blob->DatabasePtr() != mDatabase)
        throw LogicExceptionImpl("Row::Set[Blob]",
            _("IBlob and Row attached to different databases"));
    if (mTransaction != 0 && blob->TransactionPtr() != mTransaction)
        throw LogicExceptionImpl("Row::Set[Blob]",
            _("IBlob and Row attached to different transactions"));

    SetValue(param, ivBlob, blob.intf());
}

void RowImplFb3::Set(int param, const IBPP::Array& array)
{
    CheckIsInit("Row::Set[Array]");
    if (mDatabase != 0 && array->DatabasePtr() != mDatabase)
        throw LogicExceptionImpl("Row::Set[Array]",
            _("IArray and Row attached to different databases"));
    if (mTransaction != 0 && array->TransactionPtr() != mTransaction)
        throw LogicExceptionImpl("Row::Set[Array]",
            _("IArray and Row attached to different transactions"));

    SetValue(param, ivArray, (void*)array.intf());
}

void RowImplFb3::Set(int param, const IBPP::DBKey& key)
{
    CheckIsInit("Row::Set[DBKey]");

    SetValue(param, ivDBKey, (void*)&key);
}

bool RowImplFb3::IsNull(int column)
{
    CheckIsInit("Row::IsNull");
    CheckColnum("Row::IsNull", column);

    PCOLINFO col = &mColumns[column-1];
    return col->GetIsNull();
}

bool RowImplFb3::Get(int column, bool& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivBool);
    if (pvalue != 0)
        retvalue = (*(char*)pvalue == 0 ? false : true);
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, char* retvalue)
{
    CheckIsInit("Row::Get");
    if (retvalue == 0)
        throw LogicExceptionImpl("Row::Get", _("Null pointer detected"));

    int sqllen;
    void* pvalue = GetValue(column, ivByte, &sqllen);
    if (pvalue != 0)
    {
        memcpy(retvalue, pvalue, sqllen);
        retvalue[sqllen] = '\0';
    }
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, void* bindata, int& userlen)
{
    CheckIsInit("Row::Get");
    if (bindata == 0)
        throw LogicExceptionImpl("Row::Get", _("Null pointer detected"));
    if (userlen < 0)
        throw LogicExceptionImpl("Row::Get", _("Length must be >= 0"));

    int sqllen;
    void* pvalue = GetValue(column, ivByte, &sqllen);
    if (pvalue != 0)
    {
        // userlen says how much bytes the user can accept
        // let's shorten it, if there is less bytes available
        if (sqllen < userlen) userlen = sqllen;
        memcpy(bindata, pvalue, userlen);
    }
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, std::string& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivString, &retvalue);
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, int16_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivInt16);
    if (pvalue != 0)
        retvalue = *(int16_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, int32_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivInt32);
    if (pvalue != 0)
        retvalue = *(int32_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, int64_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivInt64);
    if (pvalue != 0)
        retvalue = *(int64_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::ibpp_int128_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivInt128);
    if (pvalue != 0)
        retvalue = *(IBPP::ibpp_int128_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, float& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivFloat);
    if (pvalue != 0)
        retvalue = *(float*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, double& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivDouble);
    if (pvalue != 0)
        retvalue = *(double*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::ibpp_dec16_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivDec16);
    if (pvalue != 0)
        retvalue = *(IBPP::ibpp_dec16_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::ibpp_dec34_t& retvalue)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivDec34);
    if (pvalue != 0)
        retvalue = *(IBPP::ibpp_dec34_t*)pvalue;
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::Timestamp& timestamp)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivTimestamp, (void*)&timestamp);
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::Date& date)
{
    CheckIsInit("Row::Get");

    if (mDialect == 1)
    {
        // Dialect 1. IBPP::Date is supposed to work with old 'DATE'
        // fields which are actually ISC_TIMESTAMP.
        IBPP::Timestamp timestamp;
        void* pvalue = GetValue(column, ivTimestamp, (void*)&timestamp);
        if (pvalue != 0) date = timestamp;
        return pvalue == 0 ? true : false;
    }
    else
    {
        void* pvalue = GetValue(column, ivDate, (void*)&date);
        return pvalue == 0 ? true : false;
    }
}

bool RowImplFb3::Get(int column, IBPP::Time& time)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivTime, (void*)&time);
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::Blob& retblob)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivBlob, (void*)retblob.intf());
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::DBKey& retkey)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivDBKey, (void*)&retkey);
    return pvalue == 0 ? true : false;
}

bool RowImplFb3::Get(int column, IBPP::Array& retarray)
{
    CheckIsInit("Row::Get");

    void* pvalue = GetValue(column, ivArray, (void*)retarray.intf());
    return pvalue == 0 ? true : false;
}

/*
const IBPP::Value RowImplFb3::Get(int column)
{
    if (mMeta == nullptr)
        throw LogicExceptionImpl("Row::Get", _("The row is not initialized."));

    //void* value = GetValue(column, ivArray, (void*)retarray.intf());
    //return value == 0 ? true : false;
    return IBPP::Value();
}
*/

bool RowImplFb3::IsNull(const std::string& name)
{
    CheckIsInit("Row::IsNull");

    return IsNull(ColumnNum(name));
}

bool RowImplFb3::Get(const std::string& name, bool& retvalue)
{
    CheckIsInit("Row::Get[bool]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, char* retvalue)
{
    CheckIsInit("Row::Get[char*]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, void* retvalue, int& count)
{
    CheckIsInit("Row::Get[void*,int]");

    return Get(ColumnNum(name), retvalue, count);
}

bool RowImplFb3::Get(const std::string& name, std::string& retvalue)
{
    CheckIsInit("Row::Get[string]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, int16_t& retvalue)
{
    CheckIsInit("Row::Get[int16_t]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, int32_t& retvalue)
{
    CheckIsInit("Row::Get[int32_t]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, int64_t& retvalue)
{
    CheckIsInit("Row::Get[int64_t]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, float& retvalue)
{
    CheckIsInit("Row::Get[float]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, double& retvalue)
{
    CheckIsInit("Row::Get[double]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, IBPP::Timestamp& retvalue)
{
    CheckIsInit("Row::Get[Timestamp]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, IBPP::Date& retvalue)
{
    CheckIsInit("Row::Get[Date]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, IBPP::Time& retvalue)
{
    CheckIsInit("Row::Get[Time]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string&name, IBPP::Blob& retblob)
{
    CheckIsInit("Row::Get[Blob]");

    return Get(ColumnNum(name), retblob);
}

bool RowImplFb3::Get(const std::string& name, IBPP::DBKey& retvalue)
{
    CheckIsInit("Row::Get[DBKey]");

    return Get(ColumnNum(name), retvalue);
}

bool RowImplFb3::Get(const std::string& name, IBPP::Array& retarray)
{
    CheckIsInit("Row::Get[Array]");

    return Get(ColumnNum(name), retarray);
}

/*
const IBPP::Value RowImplFb3::Get(const std::string& name)
{
    if (mMeta == nullptr)
        throw LogicExceptionImpl("Row::Get", _("The row is not initialized."));

    return Get(ColumnNum(name));
}
*/

int RowImplFb3::Columns()
{
    CheckIsInit("Row::Columns");

    return mColCount;
}

int RowImplFb3::ColumnNum(const std::string& name)
{
    CheckIsInit("Row::ColumnNum");
    if (name.empty())
        throw LogicExceptionImpl("Row::ColumnNum", _("Column name <empty> not found."));

    /*XSQLVAR* var;*/
    PCOLINFO col;
    // Local upper case copy of the column name
    std::string Uname = StrToUpper(name);

    // Loop through the columns of the descriptor
    for (int i = 0; i < mColCount; i++)
    {
        col = &mColumns[i];
        if (col->fieldName == Uname)
            return i+1;
    }

    // Failed finding the column name, let's retry using the aliases
    for (int i = 0; i < mColCount; i++)
    {
        col = &mColumns[i];
        if (col->aliasName == Uname)
            return i+1;
    }

    throw LogicExceptionImpl("Row::ColumnNum", _("Could not find matching column."));
}

/*
ColumnName, ColumnAlias, ColumnTable : all these 3 have a mistake.
Ideally, the strings should be stored elsewhere (like _Numerics and so on) to
take into account the final '\0' which needs to be added. For now, we insert
the '\0' in the original data, which will cut the 32th character. Not terribly
bad, but should be cleanly rewritten.
*/

const char* RowImplFb3::ColumnName(int varnum)
{
    CheckIsInit("Row::ColumnName");
    CheckColnum("Row::ColumnName", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->fieldName.c_str();
}

const char* RowImplFb3::ColumnAlias(int varnum)
{
    CheckIsInit("Row::ColumnAlias");
    CheckColnum("Row::ColumnAlias", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->aliasName.c_str();
}

const char* RowImplFb3::ColumnTable(int varnum)
{
    CheckIsInit("Row::ColumnTable");
    CheckColnum("Row::ColumnTable", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->relationName.c_str();
}

int RowImplFb3::ColumnSQLType(int varnum)
{
    CheckIsInit("Row::ColumnSQLType");
    CheckColnum("Row::ColumnSQLType", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->sqltype;
}

IBPP::SDT RowImplFb3::ColumnType(int varnum)
{
    CheckIsInit("Row::ColumnType");
    CheckColnum("Row::ColumnType", varnum);

    IBPP::SDT value;
    PCOLINFO col = &mColumns[varnum-1];

    switch (col->sqltype)
    {
        case SQL_TEXT :      value = IBPP::sdString;    break;
        case SQL_VARYING :   value = IBPP::sdString;    break;
        case SQL_SHORT :     value = IBPP::sdSmallint;  break;
        case SQL_LONG :      value = IBPP::sdInteger;   break;
        case SQL_INT64 :     value = IBPP::sdLargeint;  break;
        case SQL_FLOAT :     value = IBPP::sdFloat;     break;
        case SQL_DOUBLE :    value = IBPP::sdDouble;    break;
        case SQL_TIMESTAMP : value = IBPP::sdTimestamp; break;
        case SQL_TYPE_DATE : value = IBPP::sdDate;      break;
        case SQL_TYPE_TIME : value = IBPP::sdTime;      break;
        case SQL_BLOB :      value = IBPP::sdBlob;      break;
        case SQL_ARRAY :     value = IBPP::sdArray;     break;
        case SQL_BOOLEAN :   value = IBPP::sdBoolean;     break;
        case SQL_TIME_TZ :   value = IBPP::sdTimeTz;    break;
        case SQL_TIMESTAMP_TZ : value = IBPP::sdTimestampTz; break;
        case SQL_INT128 :    value = IBPP::sdInt128;    break;
        case SQL_DEC16 :     value = IBPP::sdDec16;     break;
        case SQL_DEC34 :     value = IBPP::sdDec34;     break;
        default : throw LogicExceptionImpl("Row::ColumnType",
                            _("Found an unknown sqltype !"));
    }

    return value;
}

int RowImplFb3::ColumnSubtype(int varnum)
{
    CheckIsInit("Row::ColumnSubtype");
    CheckColnum("Row::ColumnSubtype", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->sqlsubtype;
}

int RowImplFb3::ColumnSize(int varnum)
{
    CheckIsInit("Row::ColumnSize");
    CheckColnum("Row::ColumnSize", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->dataLength;
}

int RowImplFb3::ColumnScale(int varnum)
{
    CheckIsInit("Row::ColumnScale");
    CheckColnum("Row::ColumnScale", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return -col->sqlscale;
}

bool RowImplFb3::ColumnUpdated(int varnum)
{
    CheckIsInit("Row::ColumnUpdated");
    CheckColnum("Row::ColumnUpdated", varnum);

    PCOLINFO col = &mColumns[varnum-1];
    return col->data.updated;
}

bool RowImplFb3::Updated()
{
    CheckIsInit("Row::Updated");

    for (int i = 0; i < mColCount; i++)
    {
        PCOLINFO col = &mColumns[i];
        if (col->data.updated)
            return true;
    }
    return false;
}

IBPP::Database RowImplFb3::DatabasePtr() const
{
    return mDatabase;
}

IBPP::Transaction RowImplFb3::TransactionPtr() const
{
    return mTransaction;
}

IBPP::IRow* RowImplFb3::Clone()
{
    // By definition the clone of an IBPP Row is a new row (so refcount=0).

    RowImplFb3* clone = new RowImplFb3(*this);
    return clone;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void RowImplFb3::SetValue(int varnum, IITYPE ivType, const void* value, int userlen)
{
    CheckColnum("Row::SetValue", varnum);
    if (value == 0)
        throw LogicExceptionImpl("RowImplFb3::SetValue", _("Unexpected null pointer detected."));

    int16_t len;
    PCOLINFO col = &mColumns[varnum-1];
    Firebird::IUtil* util = nullptr;

    switch (col->sqltype)
    {
        case SQL_TEXT :
            if (ivType == ivString)
            {
                std::string* svalue = (std::string*)value;
                len = (int16_t)svalue->length();
                if (len > col->dataLength)
                    len = col->dataLength;
                strncpy(col->dataPtr, svalue->c_str(), len);
                while (len < col->dataLength)
                    col->dataPtr[len++] = ' ';
            }
            else if (ivType == ivByte)
            {
                if (userlen > col->dataLength)
                    userlen = col->dataLength;
                memcpy(col->dataPtr, value, userlen);
                while (userlen < col->dataLength)
                    col->dataPtr[userlen++] = ' ';
            }
            else if (ivType == ivDBKey)
            {
                IBPP::DBKey* key = (IBPP::DBKey*)value;
                key->GetKey(col->dataPtr, col->dataLength);
            }
            else if (ivType == ivBool)
            {
                col->dataPtr[0] = *(bool*)value ? 'T' : 'F';
                len = 1;
                while (len < col->dataLength)
                    col->dataPtr[len++] = ' ';
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_VARYING :
            if (ivType == ivString)
            {
                std::string* svalue = (std::string*)value;
                len = (int16_t)svalue->length();
                if (len > col->dataLength)
                    len = col->dataLength;
                *(int16_t*)col->dataPtr = (int16_t)len;
                strncpy(col->dataPtr+2, svalue->c_str(), len);
            }
            else if (ivType == ivByte)
            {
                if (userlen > col->dataLength)
                    userlen = col->dataLength;
                *(int16_t*)col->dataPtr = (int16_t)userlen;
                memcpy(col->dataPtr+2, value, userlen);
            }
            else if (ivType == ivBool)
            {
                *(int16_t*)col->dataPtr = (int16_t)1;
                col->dataPtr[2] = *(bool*)value ? 'T' : 'F';
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_SHORT :
            if (ivType == ivBool)
            {
                *(int16_t*)col->dataPtr = int16_t(*(bool*)value ? 1 : 0);
            }
            else if (ivType == ivInt16)
            {
                *(int16_t*)col->dataPtr = *(int16_t*)value;
            }
            else if (ivType == ivInt32)
            {
                if (*(int32_t*)value < consts::min16 || *(int32_t*)value > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::SetValue",
                        _("Out of range numeric conversion !"));
                *(int16_t*)col->dataPtr = (int16_t)*(int32_t*)value;
            }
            else if (ivType == ivInt64)
            {
                if (*(int64_t*)value < consts::min16 || *(int64_t*)value > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::SetValue",
                        _("Out of range numeric conversion !"));
                *(int16_t*)col->dataPtr = (int16_t)*(int64_t*)value;
            }
            else if (ivType == ivInt128)
            {
                if (*(int64_t*)value < consts::min16 || *(int64_t*)value > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::SetValue",
                        _("Out of range numeric conversion !"));
                *(int16_t*)col->dataPtr = (int16_t)*(int64_t*)value;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_SHORT is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(int16_t*)col->dataPtr =
                    (int16_t)floor(*(float*)value * multiplier + 0.5);
            }
            else if (ivType == ivDouble)
            {
                // This SQL_SHORT is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(int16_t*)col->dataPtr =
                    (int16_t)floor(*(double*)value * multiplier + 0.5);
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue",  col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_LONG :
            if (ivType == ivBool)
            {
                *(ISC_LONG*)col->dataPtr = *(bool*)value ? 1 : 0;
            }
            else if (ivType == ivInt16)
            {
                *(ISC_LONG*)col->dataPtr = *(int16_t*)value;
            }
            else if (ivType == ivInt32)
            {
                *(ISC_LONG*)col->dataPtr = *(ISC_LONG*)value;
            }
            else if (ivType == ivInt64)
            {
                if (*(int64_t*)value < consts::min32 || *(int64_t*)value > consts::max32)
                    throw LogicExceptionImpl("RowImplFb3::SetValue",
                        _("Out of range numeric conversion !"));
                *(ISC_LONG*)col->dataPtr = (ISC_LONG)*(int64_t*)value;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_LONG is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(ISC_LONG*)col->dataPtr =
                    (ISC_LONG)floor(*(float*)value * multiplier + 0.5);
            }
            else if (ivType == ivDouble)
            {
                // This SQL_LONG is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(ISC_LONG*)col->dataPtr =
                    (ISC_LONG)floor(*(double*)value * multiplier + 0.5);
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            break;

        case SQL_INT64 :
            if (ivType == ivBool)
            {
                *(int64_t*)col->dataPtr = *(bool*)value ? 1 : 0;
            }
            else if (ivType == ivInt16)
            {
                *(int64_t*)col->dataPtr = *(int16_t*)value;
            }
            else if (ivType == ivInt32)
            {
                *(int64_t*)col->dataPtr = *(int32_t*)value;
            }
            else if (ivType == ivInt64)
            {
                *(int64_t*)col->dataPtr = *(int64_t*)value;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(int64_t*)col->dataPtr =
                    (int64_t)floor(*(float*)value * multiplier + 0.5);
            }
            else if (ivType == ivDouble)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double multiplier = consts::dscales[-col->sqlscale];
                *(int64_t*)col->dataPtr =
                    (int64_t)floor(*(double*)value * multiplier + 0.5);
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            break;

        case SQL_FLOAT :
            if (ivType != ivFloat || col->sqlscale != 0)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            *(float*)col->dataPtr = *(float*)value;
            break;

        case SQL_DOUBLE :
            if (ivType != ivDouble)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            if (col->sqlscale < 0)
            {
                // Round to scale of NUMERIC(x,y)
                double multiplier = consts::dscales[-col->sqlscale];
                *(double*)col->dataPtr =
                    floor(*(double*)value * multiplier + 0.5) / multiplier;
            }
            else *(double*)col->dataPtr = *(double*)value;
            break;

        case SQL_TIMESTAMP :
        {
            if (ivType != ivTimestamp)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->encodeTimestamp(*(ISC_TIMESTAMP*)col->dataPtr, *(IBPP::Timestamp*)value);
            break;
        }

        case SQL_TYPE_DATE :
        {
            if (ivType != ivDate)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->encodeDate(*(ISC_DATE*)col->dataPtr, *(IBPP::Date*)value);
            break;
        }

        case SQL_TYPE_TIME :
        {
            if (ivType != ivTime)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                            _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->encodeTime(*(ISC_TIME*)col->dataPtr, *(IBPP::Time*)value);
            break;
        }

        case SQL_BLOB :
            if (ivType == ivBlob)
            {
                BlobImplFb3* blob = (BlobImplFb3*)value;
                blob->GetId((ISC_QUAD*)col->dataPtr);
            }
            else if (ivType == ivString)
            {
                BlobImplFb3 blob(mDatabase, mTransaction);
                blob.Save(*(std::string*)value);
                blob.GetId((ISC_QUAD*)col->dataPtr);
            }
            else throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            break;

        case SQL_ARRAY :
            if (ivType != ivArray)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            {
                ArrayImplFb3* array = (ArrayImplFb3*)value;
                array->GetId((ISC_QUAD*)col->dataPtr);
                // When an array has been affected to a column, we want to reset
                // its ID. This way, the next WriteFrom() on the same Array object
                // will allocate a new ID. This protects against storing the same
                // array ID in multiple columns or rows.
                array->ResetId();
            }
            break;

        case SQL_BOOLEAN:
            if (ivType != ivBool)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                    _("Incompatible types."));
            *(bool*)col->dataPtr = *(bool*)value;
            break;
        default : throw LogicExceptionImpl("RowImplFb3::SetValue",
                        _("The field uses an unsupported SQL type !"));
    }

    // Remove the 0 flag
    if (col->isNullable)
        *col->notNullPtr = 0;
    // update marker
    col->data.updated = true;
}

void* RowImplFb3::GetValue(int varnum, IITYPE ivType, void* retvalue)
{
    CheckColnum("Row::GetValue", varnum);

    void* value;
    int len;
    PCOLINFO col = &mColumns[varnum-1];

    // When there is no value (SQL NULL)
    if ((col->isNullable) && (*col->notNullPtr != 0))
        return 0;

    switch (col->sqltype)
    {
            case SQL_BOOLEAN : // Firebird v3
            if (ivType == ivString)
            {
                // In case of ivString, 'void* retvalue' points to a std::string where we
                // will directly store the data.
                std::string* str = (std::string*)retvalue;
                str->assign(col->dataPtr[0] == 1 ? "1" : "0");
                value = retvalue;
            }
            else if (ivType == ivBool)
            {
                value = col->dataPtr;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            break;

            case SQL_TEXT :
            if (ivType == ivString)
            {
                // In case of ivString, 'void* retvalue' points to a std::string where we
                // will directly store the data.
                std::string* str = (std::string*)retvalue;
                str->erase();
                str->append(col->dataPtr, col->dataLength);
                value = retvalue;	// value != 0 means 'not null'
            }
            else if (ivType == ivByte)
            {
                // In case of ivByte, void* retvalue points to an int where we
                // will store the len of the available data
                if (retvalue != 0)
                    *(int*)retvalue = col->dataLength;
                value = col->dataPtr;
            }
            else if (ivType == ivDBKey)
            {
                IBPP::DBKey* key = (IBPP::DBKey*)retvalue;
                key->SetKey(col->dataPtr, col->dataLength);
                value = retvalue;
            }
            else if (ivType == ivBool)
            {
                char c = 0;
                if (col->dataLength >= 1)
                    char c = *col->dataPtr;
                col->data.SetBoolFromChar(c);
                value = &col->data.vBool;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_VARYING :
            if (ivType == ivString)
            {
                // In case of ivString, 'void* retvalue' points to a std::string where we
                // will directly store the data.
                std::string* str = (std::string*)retvalue;
                str->erase();
                str->append(col->dataPtr+2, (int32_t)*(int16_t*)col->dataPtr);
                value = retvalue;
            }
            else if (ivType == ivByte)
            {
                // In case of ivByte, void* retvalue points to an int where we
                // will store the len of the available data
                if (retvalue != 0)
                    *(int*)retvalue = (int)*(int16_t*)col->dataPtr;
                value = col->dataPtr+2;
            }
            else if (ivType == ivBool)
            {
                char c = 0;
                len = *(int16_t*)col->dataPtr;
                if (len >= 1)
                    c = col->dataPtr[2];
                col->data.SetBoolFromChar(c);
                value = &col->data.vBool;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_SHORT :
            if (ivType == ivInt16)
            {
                value = col->dataPtr;
            }
            else if (ivType == ivInt32)
            {
                col->data.vInt32 = *(int16_t*)col->dataPtr;
                value = &col->data.vInt32;
            }
            else if (ivType == ivInt64)
            {
                col->data.vInt64 = *(int16_t*)col->dataPtr;
                value = &col->data.vInt64;
            }
            else if (ivType == ivInt128)
            {
                col->data.vInt128 = *(int16_t*)col->dataPtr;
                value = &col->data.vInt128;
            }
            else if (ivType == ivBool)
            {
                if (*(int16_t*)col->dataPtr == 0)
                    col->data.vBool = 0;
                else
                    col->data.vBool = 1;
                value = &col->data.vBool;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_SHORT is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vFloat = (float)(*(int16_t*)col->dataPtr / divisor);
                value = &col->data.vFloat;
            }
            else if (ivType == ivDouble)
            {
                // This SQL_SHORT is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vNumeric = *(int16_t*)col->dataPtr / divisor;
                value = &col->data.vNumeric;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_LONG :
            if (ivType == ivInt32)
            {
                value = col->dataPtr;
            }
            else if (ivType == ivInt16)
            {
                int32_t tmp = *(int32_t*)col->dataPtr;
                if (tmp < consts::min16 || tmp > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt16 = (int16_t)tmp;
                value = &col->data.vInt16;
            }
            else if (ivType == ivInt64)
            {
                col->data.vInt64 = *(int32_t*)col->dataPtr;
                value = &col->data.vInt64;
            }
            else if (ivType == ivInt128)
            {
                col->data.vInt128 = *(int32_t*)col->dataPtr;
                value = &col->data.vInt128;
            }
            else if (ivType == ivBool)
            {
                if (*(int32_t*)col->dataPtr == 0)
                    col->data.vBool = 0;
                else
                    col->data.vBool = 1;
                value = &col->data.vBool;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_LONG is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vFloat = (float)(*(int32_t*)col->dataPtr / divisor);
                value = &col->data.vFloat;
            }
            else if (ivType == ivDouble)
            {
                // This SQL_LONG is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vNumeric = *(int32_t*)col->dataPtr / divisor;
                value = &col->data.vNumeric;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_INT64 :
            if (ivType == ivInt64)
            {
                value = col->dataPtr;
            }
            else if (ivType == ivInt16)
            {
                int64_t tmp = *(int64_t*)col->dataPtr;
                if (tmp < consts::min16 || tmp > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt16 = (int16_t)tmp;
                value = &col->data.vInt16;
            }
            else if (ivType == ivInt32)
            {
                int64_t tmp = *(int64_t*)col->dataPtr;
                if (tmp < consts::min32 || tmp > consts::max32)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt32 = (int32_t)tmp;
                value = &col->data.vInt32;
            }
            else if (ivType == ivInt128)
            {
                col->data.vInt128 = *(int64_t*)col->dataPtr;
                value = &col->data.vInt128;
            }
            else if (ivType == ivBool)
            {
                if (*(int64_t*)col->dataPtr == 0)
                    col->data.vBool = 0;
                else
                    col->data.vBool = 1;
                value = &col->data.vBool;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vFloat = (float)(*(int64_t*)col->dataPtr / divisor);
                value = &col->data.vFloat;
            }
            else if (ivType == ivDouble)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vNumeric = *(int64_t*)col->dataPtr / divisor;
                value = &col->data.vNumeric;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_INT128 :
            if (ivType == ivInt128)
            {
                value = col->dataPtr;
            }
            #ifdef HAVE_INT128
            else if (ivType == ivInt16)
            {
                __int128 tmp = *(__int128*)col->dataPtr;
                if (tmp < consts::min16 || tmp > consts::max16)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt16 = (int16_t)tmp;
                value = &col->data.vInt16;
            }
            else if (ivType == ivInt32)
            {
                __int128_t tmp = *(__int128_t*)col->dataPtr;
                if (tmp < consts::min32 || tmp > consts::max32)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt32 = (int32_t)tmp;
                value = &col->data.vInt32;
            }
            else if (ivType == ivInt64)
            {
                __int128_t tmp = *(__int128_t*)col->dataPtr;
                if (tmp < consts::min64 || tmp > consts::max64)
                    throw LogicExceptionImpl("RowImplFb3::GetValue",
                        _("Out of range numeric conversion !"));
                col->data.vInt64 = *(int64_t*)col->dataPtr;
                value = &col->data.vInt64;
            }
            else if (ivType == ivBool)
            {
                if (*(__int128_t*)col->dataPtr == 0)
                    col->data.vBool = 0;
                else
                    col->data.vBool = 1;
                value = &col->data.vBool;
            }
            else if (ivType == ivFloat)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vFloat = (float)(*(__int128_t*)col->dataPtr / divisor);
                value = &col->data.vFloat;
            }
            else if (ivType == ivDouble)
            {
                // This SQL_INT64 is a NUMERIC(x,y), scale it !
                double divisor = consts::dscales[-col->sqlscale];
                col->data.vNumeric = *(__int128_t*)col->dataPtr / divisor;
                value = &col->data.vNumeric;
            }
            #endif          
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_DEC16 :
            if (ivType == ivDec16)
            {
                value = col->dataPtr;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_DEC34 :
            if (ivType == ivDec34)
            {
                value = col->dataPtr;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                     _("Incompatible types."));
            break;

        case SQL_FLOAT :
            if (ivType != ivFloat)
                throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            value = col->dataPtr;
            break;

        case SQL_DOUBLE :
            if (ivType != ivDouble)
                throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            if (col->sqlscale < 0)
            {
                // Round to scale y of NUMERIC(x,y)
                double multiplier = consts::dscales[-col->sqlscale];
                col->data.vNumeric =
                    floor(*(double*)col->dataPtr * multiplier + 0.5) / multiplier;
                value = &col->data.vNumeric;
            }
            else value = col->dataPtr;
            break;

        case SQL_TIMESTAMP :
        {
            if (ivType != ivTimestamp)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->decodeTimestamp(*(IBPP::Timestamp*)retvalue, *(ISC_TIMESTAMP*)col->dataPtr);
            value = retvalue;
            break;
        }

        case SQL_TYPE_DATE :
        {
            if (ivType != ivDate)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->decodeDate(*(IBPP::Date*)retvalue, *(ISC_DATE*)col->dataPtr);
            value = retvalue;
            break;
        }

        case SQL_TYPE_TIME :
        {
            if (ivType != ivTime)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->decodeTime(*(IBPP::Time*)retvalue, *(ISC_TIME*)col->dataPtr);
            value = retvalue;
            break;
        }

        case SQL_BLOB :
            if (ivType == ivBlob)
            {
                BlobImplFb3* blob = (BlobImplFb3*)retvalue;
                blob->SetId((ISC_QUAD*)col->dataPtr);
                value = retvalue;
            }
            else if (ivType == ivString)
            {
                BlobImplFb3 blob(mDatabase, mTransaction);
                blob.SetId((ISC_QUAD*)col->dataPtr);
                std::string* str = (std::string*)retvalue;
                blob.Load(*str);
                value = retvalue;
            }
            else throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            break;

        case SQL_ARRAY :
            if (ivType != ivArray)
                throw WrongTypeImpl("RowImplFb3::GetValue", col->sqltype, ivType,
                                    _("Incompatible types."));
            {
                ArrayImplFb3* array = (ArrayImplFb3*)retvalue;
                array->SetId((ISC_QUAD*)col->dataPtr);
                value = retvalue;
            }
            break;

        case SQL_TIMESTAMP_TZ :
        {
            if (ivType != ivTimestamp)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->decodeTimestampTz(*(IBPP::Timestamp*)retvalue, *(ISC_TIMESTAMP_TZ*)col->dataPtr);
            value = retvalue;
            break;
        }

        case SQL_TIME_TZ :
        {
            if (ivType != ivTime)
                throw WrongTypeImpl("RowImplFb3::SetValue", col->sqltype, ivType,
                                        _("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
            utl->decodeTimeTz(*(IBPP::Time*)retvalue, *(ISC_TIME_TZ*)col->dataPtr);
            value = retvalue;
            break;
        }

        default : throw LogicExceptionImpl("RowImplFb3::GetValue",
                                           _("Found an unknown sqltype !"));
    }

    return value;
}

void RowImplFb3::FreeBuffer()
{
    if (mBuffer == nullptr)
        return;
    delete[] mBuffer;
    mBuffer = nullptr;
}

bool RowImplFb3::MissingValues()
{
    for (int i = 0; i < mColCount; i++)
    {
        PCOLINFO col = &mColumns[i];
        if (!col->data.updated)
            return true;
    }
    return false;
}

void RowImplFb3::SetFbIntfMeta(Firebird::IMessageMetadata* meta)
{
    FreeBuffer();

    mMeta = meta;
    mColCount = meta->getCount(mThrowStatus);
    // init buffer
    mBufferLength = meta->getMessageLength(mThrowStatus);
    mBuffer = new char[mBufferLength]();
    // init columns info
    mColumns.clear();
    for (unsigned i1 = 0; i1 < mColCount; i1++)
    {
        COLINFO ci;
        ci.sqltype = mMeta->getType(mThrowStatus, i1);
        ci.sqlsubtype = mMeta->getSubType(mThrowStatus, i1);
        ci.sqlscale = mMeta->getScale(mThrowStatus, i1);
        ci.dataOffset = mMeta->getOffset(mThrowStatus, i1);
        ci.notNullOffset = mMeta->getNullOffset(mThrowStatus, i1);
        ci.dataLength = mMeta->getLength(mThrowStatus, i1);
        ci.isNullable = mMeta->isNullable(mThrowStatus, i1);
        ci.dataPtr    = mBuffer + ci.dataOffset;
        ci.notNullPtr = (short*)(mBuffer + ci.notNullOffset);
        ci.fieldName = mMeta->getField(mThrowStatus, i1);
        ci.aliasName = mMeta->getAlias(mThrowStatus, i1);
        ci.relationName = mMeta->getRelation(mThrowStatus, i1);
        ci.data.vBool = false;
        ci.data.vInt16 = 0;
        ci.data.vInt32 = 0;
        ci.data.vInt64 = 0;
        ci.data.vInt128 = 0;
        ci.data.vFloat = 0.0;
        ci.data.vNumeric = 0.0;
        ci.data.vString.erase();
        ci.data.updated = false;
        mColumns.push_back(ci);
    }
}

RowImplFb3& RowImplFb3::operator=(const RowImplFb3& copied)
{
    FreeBuffer();

    mMeta = copied.mMeta;
    mColCount = copied.mColCount;
    // init buffer
    mBufferLength = copied.mBufferLength;
    mBuffer = new char[mBufferLength]();

    // Pointers init, real data copy
    mColumns = copied.mColumns;

    mDialect = copied.mDialect;
    mDatabase = copied.mDatabase;
    mTransaction = copied.mTransaction;

    return *this;
}

void RowImplFb3::COLDATA::SetBoolFromChar(char c)
{
    if (c == 't' || c == 'T' || c == 'y' || c == 'Y' || c == '1')
        vBool = true;
    else
        vBool = false;
}

RowImplFb3::RowImplFb3(const RowImplFb3& copied)
    : IBPP::IRow()
{
    // mRefCount and mDescrArea are set to 0 before using the assignment operator
    // The assignment operator does the real copy
    *this = copied;
}

RowImplFb3::RowImplFb3(int dialect, DatabaseImplFb3* db, TransactionImplFb3* tr)
    : mMeta(nullptr), mBuffer(nullptr), mBufferLength(0), mColCount(0)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mThrowStatus = new ThrowStatusWrapper(master->getStatus());

    mDialect = dialect;
    mDatabase = db;
    mTransaction = tr;
}

RowImplFb3::~RowImplFb3()
{
    try { FreeBuffer(); }
        catch (...) { }

    delete mThrowStatus;
}
