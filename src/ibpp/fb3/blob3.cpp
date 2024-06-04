// Blob class implementation
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
#include "fb3/ibppfb3.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void BlobImplFb3::Open()
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Open", _("Blob already opened."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Open", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Open", _("No Transaction is attached."));
	if (! mIdAssigned)
		throw LogicExceptionImpl("Blob::Open", _("Blob Id is not assigned."));

	IBS status;
	(*getGDS().Call()->m_open_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Open", _("isc_open_blob2 failed."));
	mWriteMode = false;
}

void BlobImplFb3::Create()
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Create", _("Blob already opened."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Create", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Create", _("No Transaction is attached."));

	IBS status;
	(*getGDS().Call()->m_create_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Create",
			_("isc_create_blob failed."));
	mIdAssigned = true;
	mWriteMode = true;
}

void BlobImplFb3::Close()
{
	if (mHandle == 0) return;	// Not opened anyway

	IBS status;
	(*getGDS().Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Close", _("isc_close_blob failed."));
	mHandle = 0;
}

void BlobImplFb3::Cancel()
{
	if (mHandle == 0) return;	// Not opened anyway

	if (! mWriteMode)
		throw LogicExceptionImpl("Blob::Cancel", _("Can't cancel a Blob opened for read"));

	IBS status;
	(*getGDS().Call()->m_cancel_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Cancel", _("isc_cancel_blob failed."));
	mHandle = 0;
	mIdAssigned = false;
}

int BlobImplFb3::Read(void* buffer, int size)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Read", _("The Blob is not opened"));
	if (mWriteMode)
		throw LogicExceptionImpl("Blob::Read", _("Can't read from Blob opened for write"));
	if (size < 1 || size > (64*1024-1))
		throw LogicExceptionImpl("Blob::Read", _("Invalid segment size (max 64Kb-1)"));

	IBS status;
	unsigned short bytesread;
	ISC_STATUS result = (*getGDS().Call()->m_get_segment)(status.Self(), &mHandle, &bytesread,
					(unsigned short)size, (char*)buffer);
	if (result == isc_segstr_eof) return 0;	// Fin du blob
	if (result != isc_segment && status.Errors())
		throw SQLExceptionImpl(status, "Blob::Read", _("isc_get_segment failed."));
	return (int)bytesread;
}

void BlobImplFb3::Write(const void* buffer, int size)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Write", _("The Blob is not opened"));
	if (! mWriteMode)
		throw LogicExceptionImpl("Blob::Write", _("Can't write to Blob opened for read"));
	if (size < 1 || size > (64*1024-1))
		throw LogicExceptionImpl("Blob::Write", _("Invalid segment size (max 64Kb-1)"));

	IBS status;
	(*getGDS().Call()->m_put_segment)(status.Self(), &mHandle,
		(unsigned short)size, (char*)buffer);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Write", _("isc_put_segment failed."));
}

void BlobImplFb3::Info(int* Size, int* Largest, int* Segments)
{
	char items[] = {isc_info_blob_total_length,
					isc_info_blob_max_segment,
					isc_info_blob_num_segments};

	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::GetInfo", _("The Blob is not opened"));

	IBS status;
	RB result(100);
	(*getGDS().Call()->m_blob_info)(status.Self(), &mHandle, sizeof(items), items,
		(short)result.Size(), result.Self());
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::GetInfo", _("isc_blob_info failed."));

	if (Size != 0) *Size = result.GetValue(isc_info_blob_total_length);
	if (Largest != 0) *Largest = result.GetValue(isc_info_blob_max_segment);
	if (Segments != 0) *Segments = result.GetValue(isc_info_blob_num_segments);
}

void BlobImplFb3::Save(const std::string& data)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Save", _("Blob already opened."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Save", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Save", _("No Transaction is attached."));

	IBS status;
	(*getGDS().Call()->m_create_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Save",
			_("isc_create_blob failed."));
	mIdAssigned = true;
	mWriteMode = true;

	size_t pos = 0;
	size_t len = data.size();
	while (len != 0)
	{
		size_t blklen = (len < 32*1024-1) ? len : 32*1024-1;
		status.Reset();
		(*getGDS().Call()->m_put_segment)(status.Self(), &mHandle,
			(unsigned short)blklen, const_cast<char*>(data.data()+pos));
		if (status.Errors())
			throw SQLExceptionImpl(status, "Blob::Save",
					_("isc_put_segment failed."));
		pos += blklen;
		len -= blklen;
	}

	status.Reset();
	(*getGDS().Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Save", _("isc_close_blob failed."));
	mHandle = 0;
}

void BlobImplFb3::Load(std::string& data)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Load", _("Blob already opened."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Load", _("No Database is attached."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Load", _("No Transaction is attached."));
	if (! mIdAssigned)
		throw LogicExceptionImpl("Blob::Load", _("Blob Id is not assigned."));

	IBS status;
	(*getGDS().Call()->m_open_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Load", _("isc_open_blob2 failed."));
	mWriteMode = false;

	size_t blklen = 32*1024-1;
	data.resize(blklen);

	size_t size = 0;
	size_t pos = 0;
	for (;;)
	{
		status.Reset();
		unsigned short bytesread;
		ISC_STATUS result = (*getGDS().Call()->m_get_segment)(status.Self(), &mHandle,
						&bytesread, (unsigned short)blklen,
							const_cast<char*>(data.data()+pos));
		if (result == isc_segstr_eof) break;	// End of blob
		if (result != isc_segment && status.Errors())
			throw SQLExceptionImpl(status, "Blob::Load", _("isc_get_segment failed."));

		pos += bytesread;
		size += bytesread;
		data.resize(size + blklen);
	}
	data.resize(size);

	status.Reset();
	(*getGDS().Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Load", _("isc_close_blob failed."));
	mHandle = 0;
}

IBPP::Database BlobImplFb3::DatabasePtr() const
{
	if (mDatabase == 0) throw LogicExceptionImpl("Blob::DatabasePtr",
			_("No Database is attached."));
	return mDatabase;
}

IBPP::Transaction BlobImplFb3::TransactionPtr() const
{
	if (mTransaction == 0) throw LogicExceptionImpl("Blob::TransactionPtr",
			_("No Transaction is attached."));
	return mTransaction;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void BlobImplFb3::Init()
{
	mIdAssigned = false;
	mWriteMode = false;
	mHandle = 0;
	mDatabase = 0;
	mTransaction = 0;
}

void BlobImplFb3::SetId(ISC_QUAD* quad)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("BlobImplFb3::SetId", _("Can't set Id on an opened BlobImplFb3."));
	if (quad == 0)
		throw LogicExceptionImpl("BlobImplFb3::SetId", _("Null Id reference detected."));

	memcpy(&mId, quad, sizeof(mId));
	mIdAssigned = true;
}

void BlobImplFb3::GetId(ISC_QUAD* quad)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("BlobImplFb3::GetId", _("Can't get Id on an opened BlobImplFb3."));
	if (! mWriteMode)
		throw LogicExceptionImpl("BlobImplFb3::GetId", _("Can only get Id of a newly created Blob."));
	if (quad == 0)
		throw LogicExceptionImpl("BlobImplFb3::GetId", _("Null Id reference detected."));

	memcpy(quad, &mId, sizeof(mId));
}

void BlobImplFb3::AttachDatabaseImpl(DatabaseImplFb3* database)
{
	if (database == 0) throw LogicExceptionImpl("Blob::AttachDatabase",
			_("Can't attach a NULL Database object."));

	if (mDatabase != 0) mDatabase->DetachBlobImpl(this);
	mDatabase = database;
	mDatabase->AttachBlobImpl(this);
}

void BlobImplFb3::AttachTransactionImpl(TransactionImplFb3* transaction)
{
	if (transaction == 0) throw LogicExceptionImpl("Blob::AttachTransaction",
			_("Can't attach a NULL Transaction object."));

	if (mTransaction != 0) mTransaction->DetachBlobImpl(this);
	mTransaction = transaction;
	mTransaction->AttachBlobImpl(this);
}

void BlobImplFb3::DetachDatabaseImpl()
{
	if (mDatabase == 0) return;

	mDatabase->DetachBlobImpl(this);
	mDatabase = 0;
}

void BlobImplFb3::DetachTransactionImpl()
{
	if (mTransaction == 0) return;

	mTransaction->DetachBlobImpl(this);
	mTransaction = 0;
}

BlobImplFb3::BlobImplFb3(DatabaseImplFb3* database, TransactionImplFb3* transaction)
{
	Init();
	AttachDatabaseImpl(database);
	if (transaction != 0) AttachTransactionImpl(transaction);
}

BlobImplFb3::~BlobImplFb3()
{
	try
	{
		if (mHandle != 0)
		{
			if (mWriteMode) Cancel();
			else Close();
		}
	}
	catch (...) { }

	try { if (mTransaction != 0) mTransaction->DetachBlobImpl(this); }
		catch (...) { }
	try { if (mDatabase != 0) mDatabase->DetachBlobImpl(this); }
		catch (...) { }
}
