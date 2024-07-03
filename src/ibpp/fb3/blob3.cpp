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
    if (mBlb != 0)
        throw LogicExceptionImpl("Blob::Open", _("Blob already opened."));
    if (mDatabase == 0)
        throw LogicExceptionImpl("Blob::Open", _("No Database is attached."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Blob::Open", _("No Transaction is attached."));
    if (!mIdAssigned)
        throw LogicExceptionImpl("Blob::Open", _("Blob Id is not assigned."));

    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    mBlb = atm->openBlob(mStatus, tra, &mId, 0, nullptr);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Open", _("IAttachment::openBlob failed."));
    mWriteMode = false;
}

void BlobImplFb3::Create()
{
    if (mBlb != nullptr)
        throw LogicExceptionImpl("Blob::Create", _("Blob already opened."));
    if (mDatabase == 0)
        throw LogicExceptionImpl("Blob::Create", _("No Database is attached."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Blob::Create", _("No Transaction is attached."));

    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    mBlb = atm->createBlob(mStatus, tra, &mId, 0, nullptr);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Create",
            _("IAttachment::crateBlob failed."));
    mIdAssigned = true;
    mWriteMode = true;
}

void BlobImplFb3::Close()
{
    // Not opened anyway
    if (mBlb == nullptr)
        return;

    mBlb->close(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Close", _("IBlob::close failed."));
    mBlb = nullptr;
}

void BlobImplFb3::Cancel()
{
    // Not opened anyway
    if (mBlb == nullptr)
        return;

    if (!mWriteMode)
        throw LogicExceptionImpl("Blob::Cancel", _("Can't cancel a Blob opened for read"));

    mBlb->cancel(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Cancel", _("isc_cancel_blob failed."));
    mBlb = nullptr;
    mIdAssigned = false;
}

int BlobImplFb3::Read(void* buffer, int size)
{
    if (mBlb == nullptr)
        throw LogicExceptionImpl("Blob::Read", _("The Blob is not opened"));
    if (mWriteMode)
        throw LogicExceptionImpl("Blob::Read", _("Can't read from Blob opened for write"));
    if (size < 1 || size > (64*1024-1))
        throw LogicExceptionImpl("Blob::Read", _("Invalid segment size (max 64Kb-1)"));

    unsigned bytesread;
    int StatusCode = mBlb->getSegment(mStatus, (unsigned)size, buffer, &bytesread);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Read", _("IBlob::getSegment failed."));
    if (StatusCode == IStatus::RESULT_NO_DATA)
        return 0;
    if ((StatusCode != IStatus::RESULT_OK) &&
        (StatusCode != IStatus::RESULT_SEGMENT))
        throw SQLExceptionImpl(mStatus, "Blob::Read", _("IBlob::getSegment unknown status code."));
    return (int)bytesread;
}

void BlobImplFb3::Write(const void* buffer, int size)
{
    if (mBlb == nullptr)
        throw LogicExceptionImpl("Blob::Write", _("The Blob is not opened"));
    if (!mWriteMode)
        throw LogicExceptionImpl("Blob::Write", _("Can't write to Blob opened for read"));
    if (size < 1 || size > (64*1024-1))
        throw LogicExceptionImpl("Blob::Write", _("Invalid segment size (max 64Kb-1)"));

    //(*getGDS().Call()->m_put_segment)(status.Self(), &mHandle,
    //  (unsigned short)size, (char*)buffer);
    mBlb->putSegment(mStatus, (unsigned)size, buffer);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Write", _("IBlob::putSegment failed."));
}

void BlobImplFb3::Info(int* Size, int* Largest, int* Segments)
{
    const unsigned char items[] =
    {
        isc_info_blob_total_length,
        isc_info_blob_max_segment,
        isc_info_blob_num_segments
    };

    if (mBlb == nullptr)
        throw LogicExceptionImpl("Blob::GetInfo", _("The Blob is not opened"));

    RB result(100);
    mBlb->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::GetInfo", _("IBlob::getInfo failed."));

    if (Size != 0)
        *Size = result.GetValue(isc_info_blob_total_length);
    if (Largest != 0)
        *Largest = result.GetValue(isc_info_blob_max_segment);
    if (Segments != 0)
        *Segments = result.GetValue(isc_info_blob_num_segments);
}

void BlobImplFb3::Save(const std::string& data)
{
    if (mBlb != nullptr)
        throw LogicExceptionImpl("Blob::Save", _("Blob already opened."));
    if (mDatabase == 0)
        throw LogicExceptionImpl("Blob::Save", _("No Database is attached."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Blob::Save", _("No Transaction is attached."));

	//(*getGDS().Call()->m_create_blob2)(status.Self(), mDatabase->GetHandlePtr(),
	//	mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    mBlb = atm->createBlob(mStatus, tra, &mId, 0, nullptr);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Save",
            _("isc_create_blob failed."));
    mIdAssigned = true;
    mWriteMode = true;

    size_t pos = 0;
    size_t len = data.size();
    while (len != 0)
    {
        size_t blklen = (len < 32*1024-1) ? len : 32*1024-1;
        mBlb->putSegment(mStatus, blklen, (void*)(data.data()+pos));
        if (mStatus->isDirty())
            throw SQLExceptionImpl(mStatus, "Blob::Save",
                    _("isc_put_segment failed."));
        pos += blklen;
        len -= blklen;
    }

    //(*getGDS().Call()->m_close_blob)(status.Self(), &mHandle);
    mBlb->close(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Save", _("IBlob::close failed."));
    mBlb = nullptr;
}

void BlobImplFb3::Load(std::string& data)
{
    if (mBlb != nullptr)
        throw LogicExceptionImpl("Blob::Load", _("Blob already opened."));
    if (mDatabase == 0)
        throw LogicExceptionImpl("Blob::Load", _("No Database is attached."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Blob::Load", _("No Transaction is attached."));
    if (! mIdAssigned)
        throw LogicExceptionImpl("Blob::Load", _("Blob Id is not assigned."));

    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    mBlb = atm->openBlob(mStatus, tra, &mId, 0, nullptr);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Load", _("IAttachment::openBlob failed."));
    mWriteMode = false;

    unsigned blklen = 32*1024-1;
    data.resize(blklen);

    size_t size = 0;
    size_t pos = 0;
    int StatusCode;
    do
    {
        unsigned bytesread;
        data.resize(size + blklen);
        StatusCode = mBlb->getSegment(mStatus, blklen, (void*)(data.data()+pos), &bytesread);
        if (mStatus->isDirty())
            throw SQLExceptionImpl(mStatus, "Blob::Load", _("IBlob::getSegment failed."));
        // end of blob
        if (StatusCode == IStatus::RESULT_NO_DATA)
            break;
        // the only possible "ok"-state is "segment". all others
        // are errors or unexpected
        if ((StatusCode != IStatus::RESULT_SEGMENT) &&
            (StatusCode != IStatus::RESULT_OK))
            throw SQLExceptionImpl(mStatus, "Blob::Read", _("IBlob::getSegment unknown status code."));;

        pos += bytesread;
        size += bytesread;
    }
    while (StatusCode != IStatus::RESULT_OK);
    data.resize(size);

    mBlb->close(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Blob::Load", _("IBlob::close failed."));
    mBlb = nullptr;
}

IBPP::Database BlobImplFb3::DatabasePtr() const
{
    if (mDatabase == nullptr)
        throw LogicExceptionImpl("Blob::DatabasePtr",
            _("No Database is attached."));
    return mDatabase;
}

IBPP::Transaction BlobImplFb3::TransactionPtr() const
{
    if (mTransaction == nullptr)
        throw LogicExceptionImpl("Blob::TransactionPtr",
            _("No Transaction is attached."));
    return mTransaction;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void BlobImplFb3::Init()
{
    mIdAssigned = false;
    mWriteMode = false;
    mBlb = nullptr;
    mDatabase = nullptr;
    mTransaction = nullptr;
}

void BlobImplFb3::SetId(ISC_QUAD* quad)
{
    if (mBlb != nullptr)
        throw LogicExceptionImpl("BlobImplFb3::SetId", _("Can't set Id on an opened BlobImplFb3."));
    if (quad == 0)
        throw LogicExceptionImpl("BlobImplFb3::SetId", _("Null Id reference detected."));

    memcpy(&mId, quad, sizeof(mId));
    mIdAssigned = true;
}

void BlobImplFb3::GetId(ISC_QUAD* quad)
{
    if (mBlb != nullptr)
        throw LogicExceptionImpl("BlobImplFb3::GetId", _("Can't get Id on an opened BlobImplFb3."));
    if (!mWriteMode)
        throw LogicExceptionImpl("BlobImplFb3::GetId", _("Can only get Id of a newly created Blob."));
    if (quad == 0)
        throw LogicExceptionImpl("BlobImplFb3::GetId", _("Null Id reference detected."));

    memcpy(quad, &mId, sizeof(mId));
}

void BlobImplFb3::AttachDatabaseImpl(DatabaseImplFb3* database)
{
    if (database == nullptr)
        throw LogicExceptionImpl("Blob::AttachDatabase",
            _("Can't attach a NULL Database object."));

    if (mDatabase != nullptr)
        mDatabase->DetachBlobImpl(this);
    mDatabase = database;
    mDatabase->AttachBlobImpl(this);
}

void BlobImplFb3::AttachTransactionImpl(TransactionImplFb3* transaction)
{
    if (transaction == nullptr)
        throw LogicExceptionImpl("Blob::AttachTransaction",
            _("Can't attach a NULL Transaction object."));

    if (mTransaction != nullptr)
        mTransaction->DetachBlobImpl(this);
    mTransaction = transaction;
    mTransaction->AttachBlobImpl(this);
}

void BlobImplFb3::DetachDatabaseImpl()
{
    if (mDatabase == nullptr)
        return;

    mDatabase->DetachBlobImpl(this);
    mDatabase = nullptr;
}

void BlobImplFb3::DetachTransactionImpl()
{
    if (mTransaction == nullptr)
        return;

    mTransaction->DetachBlobImpl(this);
    mTransaction = nullptr;
}

BlobImplFb3::BlobImplFb3(DatabaseImplFb3* database, TransactionImplFb3* transaction)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mStatus = new CheckStatusWrapper(master->getStatus());
    Init();
    AttachDatabaseImpl(database);
    if (transaction != nullptr)
        AttachTransactionImpl(transaction);
}

BlobImplFb3::~BlobImplFb3()
{
    try
    {
        if (mBlb != nullptr)
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

    delete mStatus;
}
