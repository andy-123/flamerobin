//	Array class implementation
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

#include <math.h>

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void ArrayImplFb3::Describe(const std::string& table, const std::string& column)
{
	//if (mIdAssigned)
	//	throw LogicExceptionImpl("Array::Lookup", _("Array already in use."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Array::Lookup", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Array::Lookup", _("No Transaction is attached."));

	ResetId();	// Re-use this array object if was previously assigned

	IBS status;
    isc_db_handle dbh = mDatabase->GetHandle();
	(*getGDS().Call()->m_array_lookup_bounds)(status.Self(), &dbh,
		mTransaction->GetHandlePtr(), const_cast<char*>(table.c_str()),
			const_cast<char*>(column.c_str()), &mDesc);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Array::Lookup",
			_("isc_array_lookup_bounds failed."));

	AllocArrayBuffer();

	mDescribed = true;
}

void ArrayImplFb3::SetBounds(int dim, int low, int high)
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::SetBounds", _("Array description not set."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Array::SetBounds", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Array::SetBounds", _("No Transaction is attached."));
	if (dim < 0 || dim > mDesc.array_desc_dimensions-1)
		throw LogicExceptionImpl("Array::SetBounds", _("Invalid dimension."));
	if (low > high ||
		low < mDesc.array_desc_bounds[dim].array_bound_lower ||
		low > mDesc.array_desc_bounds[dim].array_bound_upper ||
		high > mDesc.array_desc_bounds[dim].array_bound_upper ||
		high < mDesc.array_desc_bounds[dim].array_bound_lower)
		throw LogicExceptionImpl("Array::SetBounds",
			_("Invalid bounds. You can only narrow the bounds."));

	mDesc.array_desc_bounds[dim].array_bound_lower = short(low);
	mDesc.array_desc_bounds[dim].array_bound_upper = short(high);

	AllocArrayBuffer();
}

IBPP::SDT ArrayImplFb3::ElementType()
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::ElementType",
			_("Array description not set."));

	IBPP::SDT value;
	switch (mDesc.array_desc_dtype)
	{
		case blr_text :			value = IBPP::sdString;		break;
		case blr_varying : 		value = IBPP::sdString;		break;
		case blr_cstring : 		value = IBPP::sdString;		break;
		case blr_short :		value = IBPP::sdSmallint;	break;
		case blr_long :			value = IBPP::sdInteger;	break;
		case blr_int64 :		value = IBPP::sdLargeint;	break;
		case blr_float :		value = IBPP::sdFloat;		break;
		case blr_double :		value = IBPP::sdDouble;		break;
		case blr_timestamp :	value = IBPP::sdTimestamp;	break;
		case blr_sql_date :		value = IBPP::sdDate;		break;
		case blr_sql_time :		value = IBPP::sdTime;		break;
		default : throw LogicExceptionImpl("Array::ElementType",
						_("Found an unknown sqltype !"));
	}

	return value;
}

int ArrayImplFb3::ElementSize()
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::ElementSize",
			_("Array description not set."));

	return mDesc.array_desc_length;
}

int ArrayImplFb3::ElementScale()
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::ElementScale",
			_("Array description not set."));

	return mDesc.array_desc_scale;
}

int ArrayImplFb3::Dimensions()
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::Dimensions",
			_("Array description not set."));

	return mDesc.array_desc_dimensions;
}

void ArrayImplFb3::Bounds(int dim, int* low, int* high)
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::Bounds", _("Array description not set."));
	if (dim < 0 || dim > mDesc.array_desc_dimensions-1)
		throw LogicExceptionImpl("Array::Bounds", _("Invalid dimension."));
	if (low == 0 || high == 0)
		throw LogicExceptionImpl("Array::Bounds", _("Null reference detected."));

	*low =  mDesc.array_desc_bounds[dim].array_bound_lower;
	*high = mDesc.array_desc_bounds[dim].array_bound_upper;
}

/*

COMMENTS

1)
For an array column of type CHAR(X), the internal type returned or expected is blr_text.
In such case, the byte array received or submitted to get/put_slice is formatted in
elements of X bytes, which correspond to what is reported in array_desc_length.
The elements are not '\0' terminated but are right-padded with spaces ' '.

2)
For an array column of type VARCHAR(X), the internal type is blr_varying.
The underlying format is rather curious and different than what is used in XSQLDA.
The element size is reported in array_desc_length as X.
Yet each element of the byte array is expected to be of size X+2 (just as if we were
to stuff a short in the first 2 bytes to store the length (as is done with XSQLDA).
No. The string of X characters maximum has to be stored in the chunks of X+2 bytes as
a zero-terminated c-string. Note that the buffer is indeed one byte too large.
Internally, the API probably convert in-place in these chunks the zero-terminated string
to a variable-size string with a short in front and the string data non zero-terminated
behind.

3)
With Interbase 6.0 and Firebird 1.0 (initial april 2002 release), the int64 support is
broken regarding the arrays. It is not possible to work on arrays using a datatype stored
in an int64, as for instance any NUMERIC(x,0) where x is equal or greater than 10. That's
a bug in the engine, not in IBPP, which has been fixed in june 2002. Engines compiled from
the current Firebird CVS code as of july 2002 are okay. As will be the 1.01 Firebird version.
We have no idea if this is fixed or not in Interbase 6.5 though.

*/

void ArrayImplFb3::ReadTo(IBPP::ADT adtype, void* data, int datacount)
{
	if (! mIdAssigned)
		throw LogicExceptionImpl("Array::ReadTo", _("Array Id not read from column."));
	if (! mDescribed)
		throw LogicExceptionImpl("Array::ReadTo", _("Array description not set."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Array::ReadTo", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Array::ReadTo", _("No Transaction is attached."));
	if (datacount != mElemCount)
		throw LogicExceptionImpl("Array::ReadTo", _("Wrong count of array elements"));

	IBS status;
	ISC_LONG lenbuf = mBufferSize;
    isc_db_handle dbh = mDatabase->GetHandle();
	(*getGDS().Call()->m_array_get_slice)(status.Self(), &dbh,
		mTransaction->GetHandlePtr(), &mId, &mDesc, mBuffer, &lenbuf);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Array::ReadTo", _("isc_array_get_slice failed."));
	if (lenbuf != mBufferSize)
		throw SQLExceptionImpl(status, "Array::ReadTo", _("Internal buffer size discrepancy."));

	// Now, convert the types and copy values to the user array...
	int len;
	char* src = (char*)mBuffer;
	char* dst = (char*)data;

	switch (mDesc.array_desc_dtype)
	{
		case blr_text :
			if (adtype == IBPP::adString)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					strncpy(dst, src, mElemSize);
					dst[mElemSize] = '\0';
					src += mElemSize;
					dst += (mElemSize + 1);
				}
			}
			else if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*src == 't' || *src == 'T' || *src == 'y' || *src == 'Y' ||	*src == '1')
						*(bool*)dst = true;
					else *(bool*)dst = false;
					src += mElemSize;
					dst += sizeof(bool);
				}
			}
			else throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			break;

		case blr_varying :
			if (adtype == IBPP::adString)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					len = (int)strlen(src);
					if (len > mElemSize-2) len = mElemSize-2;
					strncpy(dst, src, len);
					dst[len] = '\0';
					src += mElemSize;
					dst += (mElemSize - 2 + 1);
				}
			}
			else if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*src == 't' || *src == 'T' || *src == 'y' || *src == 'Y' ||	*src == '1')
						*(bool*)dst = true;
					else *(bool*)dst = false;
					src += mElemSize;
					dst += sizeof(bool);
				}
			}
			else throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			break;

		case blr_short :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(bool*)dst = (*(short*)src != 0) ? true : false;
					src += mElemSize;
					dst += sizeof(bool);
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst = *(short*)src;
					src += mElemSize;
					dst += sizeof(short);
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int*)dst = (int)*(short*)src;
					src += mElemSize;
					dst += sizeof(int);
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = (int64_t)*(short*)src;
					src += mElemSize;
					dst += sizeof(int64_t);
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(float*)dst = (float)(*(short*)src / divisor);
					src += mElemSize;
					dst += sizeof(float);
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst = (double)(*(short*)src / divisor);
					src += mElemSize;
					dst += sizeof(double);
				}
			}
			else throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			break;

		case blr_long :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(bool*)dst = (*(long*)src != 0) ? true : false;
					src += mElemSize;
					dst += sizeof(bool);
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(long*)src < consts::min16 || *(long*)src > consts::max16)
						throw LogicExceptionImpl("Array::ReadTo",
							_("Out of range numeric conversion !"));
					*(short*)dst = short(*(long*)src);
					src += mElemSize;
					dst += sizeof(short);
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst = *(long*)src;
					src += mElemSize;
					dst += sizeof(long);
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = (int64_t)*(long*)src;
					src += mElemSize;
					dst += sizeof(int64_t);
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(float*)dst = (float)(*(long*)src / divisor);
					src += mElemSize;
					dst += sizeof(float);
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst = (double)(*(long*)src / divisor);
					src += mElemSize;
					dst += sizeof(double);
				}
			}
			else throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			break;

		case blr_int64 :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(bool*)dst = (*(int64_t*)src != 0) ? true : false;
					src += mElemSize;
					dst += sizeof(bool);
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(int64_t*)src < consts::min16 || *(int64_t*)src > consts::max16)
						throw LogicExceptionImpl("Array::ReadTo",
							_("Out of range numeric conversion !"));
					*(short*)dst = short(*(int64_t*)src);
					src += mElemSize;
					dst += sizeof(short);
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(int64_t*)src < consts::min32 || *(int64_t*)src > consts::max32)
						throw LogicExceptionImpl("Array::ReadTo",
							_("Out of range numeric conversion !"));
					*(long*)dst = (long)*(int64_t*)src;
					src += mElemSize;
					dst += sizeof(long);
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = *(int64_t*)src;
					src += mElemSize;
					dst += sizeof(int64_t);
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(float*)dst = (float)(*(int64_t*)src / divisor);
					src += mElemSize;
					dst += sizeof(float);
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst = (double)(*(int64_t*)src / divisor);
					src += mElemSize;
					dst += sizeof(double);
				}
			}
			else throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			break;

		case blr_float :
			if (adtype != IBPP::adFloat || mDesc.array_desc_scale != 0)
				throw LogicExceptionImpl("Array::ReadTo", _("Incompatible types."));
			for (int i = 0; i < mElemCount; i++)
			{
				*(float*)dst = *(float*)src;
				src += mElemSize;
				dst += sizeof(float);
			}
			break;

		case blr_double :
			if (adtype != IBPP::adDouble) throw LogicExceptionImpl("Array::ReadTo",
										_("Incompatible types."));
			if (mDesc.array_desc_scale != 0)
			{
				// Round to scale of NUMERIC(x,y)
				double divisor = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst =	(double)(*(double*)src / divisor);
					src += mElemSize;
					dst += sizeof(double);
				}
			}
			else
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst = *(double*)src;
					src += mElemSize;
					dst += sizeof(double);
				}
			}
			break;

		case blr_timestamp :
        {
			if (adtype != IBPP::adTimestamp) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->decodeTimestamp(*(IBPP::Timestamp*)dst, *(ISC_TIMESTAMP*)src);
				src += mElemSize;
				dst += sizeof(IBPP::Timestamp);
			}
			break;
        }

		case blr_sql_date :
        {
			if (adtype != IBPP::adDate) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->decodeDate(*(IBPP::Date*)dst, *(ISC_DATE*)src);
				src += mElemSize;
				dst += sizeof(IBPP::Date);
			}
			break;
        }

		case blr_sql_time :
        {
			if (adtype != IBPP::adTime) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->decodeTime(*(IBPP::Time*)dst, *(ISC_TIME*)src);
				src += mElemSize;
				dst += sizeof(IBPP::Time);
			}
			break;
        }

		default :
			throw LogicExceptionImpl("Array::ReadTo", _("Unknown sql type."));
	}
}

void ArrayImplFb3::WriteFrom(IBPP::ADT adtype, const void* data, int datacount)
{
	if (! mDescribed)
		throw LogicExceptionImpl("Array::WriteFrom", _("Array description not set."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Array::WriteFrom", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Array::WriteFrom", _("No Transaction is attached."));
	if (datacount != mElemCount)
		throw LogicExceptionImpl("Array::ReadTo", _("Wrong count of array elements"));

	// Read user data and convert types to the mBuffer
	int len;
	char* src = (char*)data;
	char* dst = (char*)mBuffer;

	switch (mDesc.array_desc_dtype)
	{
		case blr_text :
			if (adtype == IBPP::adString)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					len = (int)strlen(src);
					if (len > mElemSize) len = mElemSize;
					strncpy(dst, src, len);
					while (len < mElemSize) dst[len++] = ' ';
					src += (mElemSize + 1);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*dst = *(bool*)src ? 'T' : 'F';
					len = 1;
					while (len < mElemSize) dst[len++] = ' ';
					src += sizeof(bool);
					dst += mElemSize;
				}
			}
			else throw LogicExceptionImpl("Array::WriteFrom", _("Incompatible types."));
			break;

		case blr_varying :
			if (adtype == IBPP::adString)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					len = (int)strlen(src);
					if (len > mElemSize-2) len = mElemSize-2;
					strncpy(dst, src, len);
					dst[len] = '\0';
					src += (mElemSize - 2 + 1);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst = (short)1;
					dst[2] = *(bool*)src ? 'T' : 'F';
					src += sizeof(bool);
					dst += mElemSize;
				}
			}
			else throw LogicExceptionImpl("Array::WriteFrom", _("Incompatible types."));
			break;

		case blr_short :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst = short(*(bool*)src ? 1 : 0);
					src += sizeof(bool);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst = *(short*)src;
					src += sizeof(short);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(long*)src < consts::min16 || *(long*)src > consts::max16)
						throw LogicExceptionImpl("Array::WriteFrom",
							_("Out of range numeric conversion !"));
					*(short*)dst = (short)*(int*)src;
					src += sizeof(int);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(int64_t*)src < consts::min16 || *(int64_t*)src > consts::max16)
						throw LogicExceptionImpl("Array::WriteFrom",
							_("Out of range numeric conversion !"));
					*(short*)dst = (short)*(int64_t*)src;
					src += sizeof(int64_t);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst =
						(short)floor(*(float*)src * multiplier + 0.5);
					src += sizeof(float);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(short*)dst =
						(short)floor(*(double*)src * multiplier + 0.5);
					src += sizeof(double);
					dst += mElemSize;
				}
			}
			else throw LogicExceptionImpl("Array::WriteFrom", _("Incompatible types."));
			break;

		case blr_long :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst = *(bool*)src ? 1 : 0;
					src += sizeof(bool);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst = *(short*)src;
					src += sizeof(short);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst = *(long*)src;
					src += sizeof(long);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					if (*(int64_t*)src < consts::min32 || *(int64_t*)src > consts::max32)
						throw LogicExceptionImpl("Array::WriteFrom",
							_("Out of range numeric conversion !"));
					*(long*)dst = (long)*(int64_t*)src;
					src += sizeof(int64_t);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_INT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst =
						(long)floor(*(float*)src * multiplier + 0.5);
					src += sizeof(float);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_INT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(long*)dst =
						(long)floor(*(double*)src * multiplier + 0.5);
					src += sizeof(double);
					dst += mElemSize;
				}
			}
			else throw LogicExceptionImpl("Array::WriteFrom", _("Incompatible types."));
			break;

		case blr_int64 :
			if (adtype == IBPP::adBool)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = *(bool*)src ? 1 : 0;
					src += sizeof(bool);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt16)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = *(short*)src;
					src += sizeof(short);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt32)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = *(long*)src;
					src += sizeof(long);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adInt64)
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst = *(int64_t*)src;
					src += sizeof(int64_t);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adFloat)
			{
				// This SQL_INT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst =
						(int64_t)floor(*(float*)src * multiplier + 0.5);
					src += sizeof(float);
					dst += mElemSize;
				}
			}
			else if (adtype == IBPP::adDouble)
			{
				// This SQL_INT is a NUMERIC(x,y), scale it !
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(int64_t*)dst =
						(int64_t)floor(*(double*)src * multiplier + 0.5);
					src += sizeof(double);
					dst += mElemSize;
				}
			}
			else
				throw LogicExceptionImpl("Array::WriteFrom",
					_("Incompatible types (blr_int64 and ADT %d)."), (int)adtype);
			break;

		case blr_float :
			if (adtype != IBPP::adFloat || mDesc.array_desc_scale != 0)
				throw LogicExceptionImpl("Array::WriteFrom", _("Incompatible types."));
			for (int i = 0; i < mElemCount; i++)
			{
				*(float*)dst = *(float*)src;
				src += sizeof(float);
				dst += mElemSize;
			}
			break;

		case blr_double :
			if (adtype != IBPP::adDouble) throw LogicExceptionImpl("Array::WriteFrom",
										_("Incompatible types."));
			if (mDesc.array_desc_scale != 0)
			{
				// Round to scale of NUMERIC(x,y)
				double multiplier = consts::dscales[-mDesc.array_desc_scale];
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst =
						floor(*(double*)src * multiplier + 0.5) / multiplier;
					src += sizeof(double);
					dst += mElemSize;
				}
			}
			else
			{
				for (int i = 0; i < mElemCount; i++)
				{
					*(double*)dst = *(double*)src;
					src += sizeof(double);
					dst += mElemSize;
				}
			}
			break;

		case blr_timestamp :
        {
			if (adtype != IBPP::adTimestamp) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->encodeTimestamp(*(ISC_TIMESTAMP*)dst, *(IBPP::Timestamp*)src);
				src += sizeof(IBPP::Timestamp);
				dst += mElemSize;
			}
			break;
        }

		case blr_sql_date :
        {
			if (adtype != IBPP::adDate) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->encodeDate(*(ISC_DATE*)dst, *(IBPP::Date*)src);
				src += sizeof(IBPP::Date);
				dst += mElemSize;
			}
			break;
        }

		case blr_sql_time :
        {
			if (adtype != IBPP::adTime) throw LogicExceptionImpl("Array::ReadTo",
												_("Incompatible types."));
            UtlFb3* utl = FactoriesImplFb3::gUtlFb3;
			for (int i = 0; i < mElemCount; i++)
			{
				utl->encodeTime(*(ISC_TIME*)dst, *(IBPP::Time*)src);
				src += sizeof(IBPP::Time);
				dst += mElemSize;
			}
			break;
        }

		default :
			throw LogicExceptionImpl("Array::WriteFrom", _("Unknown sql type."));
	}

	IBS status;
	ISC_LONG lenbuf = mBufferSize;
    isc_db_handle dbh = mDatabase->GetHandle();
	(*getGDS().Call()->m_array_put_slice)(status.Self(), &dbh,
		mTransaction->GetHandlePtr(), &mId, &mDesc, mBuffer, &lenbuf);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Array::WriteFrom", _("isc_array_put_slice failed."));
	if (lenbuf != mBufferSize)
		throw SQLExceptionImpl(status, "Array::WriteFrom", _("Internal buffer size discrepancy."));
}

IBPP::Database ArrayImplFb3::DatabasePtr() const
{
	if (mDatabase == 0) throw LogicExceptionImpl("Array::DatabasePtr",
			_("No Database is attached."));
	return mDatabase;
}

IBPP::Transaction ArrayImplFb3::TransactionPtr() const
{
	if (mTransaction == 0) throw LogicExceptionImpl("Array::TransactionPtr",
			_("No Transaction is attached."));
	return mTransaction;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void ArrayImplFb3::Init()
{
	ResetId();
	mDescribed = false;
	mDatabase = 0;
	mTransaction = 0;
	mBuffer = 0;
	mBufferSize = 0;
}

void ArrayImplFb3::SetId(ISC_QUAD* quad)
{
	if (quad == 0)
		throw LogicExceptionImpl("ArrayImplFb3::SetId", _("Null Id reference detected."));

	memcpy(&mId, quad, sizeof(mId));
	mIdAssigned = true;
}

void ArrayImplFb3::GetId(ISC_QUAD* quad)
{
	if (quad == 0)
		throw LogicExceptionImpl("ArrayImplFb3::GetId", _("Null Id reference detected."));

	memcpy(quad, &mId, sizeof(mId));
}

void ArrayImplFb3::ResetId()
{
	memset(&mId, 0, sizeof(mId));
	mIdAssigned = false;
}

void ArrayImplFb3::AllocArrayBuffer()
{
	// Clean previous buffer if any
	if (mBuffer != 0) delete [] (char*)mBuffer;
	mBuffer = 0;

	// Computes total number of elements in the array or slice
	mElemCount = 1;
	for (int i = 0; i < mDesc.array_desc_dimensions; i++)
	{
		mElemCount = mElemCount *
			(mDesc.array_desc_bounds[i].array_bound_upper -
				mDesc.array_desc_bounds[i].array_bound_lower + 1);
	}

	// Allocates a buffer for this count of elements
	mElemSize = mDesc.array_desc_length;
	if (mDesc.array_desc_dtype == blr_varying) mElemSize += 2;
	else if (mDesc.array_desc_dtype == blr_cstring) mElemSize += 1;
	mBufferSize = mElemSize * mElemCount;
	mBuffer = (void*) new char[mBufferSize];
}

void ArrayImplFb3::AttachDatabaseImpl(DatabaseImplFb3* database)
{
	if (database == 0) throw LogicExceptionImpl("Array::AttachDatabase",
			_("Can't attach a 0 Database object."));

	if (mDatabase != 0) mDatabase->DetachArrayImpl(this);
	mDatabase = database;
	mDatabase->AttachArrayImpl(this);
}

void ArrayImplFb3::AttachTransactionImpl(TransactionImplFb3* transaction)
{
	if (transaction == 0) throw LogicExceptionImpl("Array::AttachTransaction",
			_("Can't attach a 0 Transaction object."));

	if (mTransaction != 0) mTransaction->DetachArrayImpl(this);
	mTransaction = transaction;
	mTransaction->AttachArrayImpl(this);
}

void ArrayImplFb3::DetachDatabaseImpl()
{
	if (mDatabase == 0) return;

	mDatabase->DetachArrayImpl(this);
	mDatabase = 0;
}

void ArrayImplFb3::DetachTransactionImpl()
{
	if (mTransaction == 0) return;

	mTransaction->DetachArrayImpl(this);
	mTransaction = 0;
}

ArrayImplFb3::ArrayImplFb3(DatabaseImplFb3* database, TransactionImplFb3* transaction)
{
	Init();
	AttachDatabaseImpl(database);
	if (transaction != 0) AttachTransactionImpl(transaction);
}

ArrayImplFb3::~ArrayImplFb3()
{
	try { if (mTransaction != 0) mTransaction->DetachArrayImpl(this); }
		catch (...) {}
	try { if (mDatabase != 0) mDatabase->DetachArrayImpl(this); }
		catch (...) {}
	try { if (mBuffer != 0) delete [] (char*)mBuffer; }
		catch (...) {}
}
