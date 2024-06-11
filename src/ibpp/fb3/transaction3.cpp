// IBPP, Transaction class implementation
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

#include <algorithm>

using namespace ibpp_internals;

//  (((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void TransactionImplFb3::AttachDatabase(IBPP::Database db,
    IBPP::TAM am, IBPP::TIL il, IBPP::TLR lr, IBPP::TFF flags)
{
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::AttachDatabase",
                _("Can't attach an unbound Database."));

    AttachDatabaseImpl(dynamic_cast<DatabaseImplFb3*>(db.intf()), am, il, lr, flags);
}

void TransactionImplFb3::DetachDatabase(IBPP::Database db)
{
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach an unbound Database."));

    DetachDatabaseImpl(dynamic_cast<DatabaseImplFb3*>(db.intf()));
}

void TransactionImplFb3::AddReservation(IBPP::Database db,
    const std::string& table, IBPP::TTR tr)
{
    if (mTra != 0)
        throw LogicExceptionImpl("Transaction::AddReservation",
                _("Can't add table reservation if Transaction started."));
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::AddReservation",
                _("Can't add table reservation on an unbound Database."));

    // Find the TPB associated with this database
    std::vector<DatabaseImplFb3*>::iterator pos =
        std::find(mDatabases.begin(), mDatabases.end(), dynamic_cast<DatabaseImplFb3*>(db.intf()));
    if (pos != mDatabases.end())
    {
        size_t index = pos - mDatabases.begin();
        TPBFb3* tpb = mTPBs[index];

        // Now add the reservations to the TPB
        switch (tr)
        {
            case IBPP::trSharedWrite :
                    tpb->InsertString(isc_tpb_lock_write, table);
                    tpb->InsertTag(isc_tpb_shared);
                    break;
            case IBPP::trSharedRead :
                    tpb->InsertString(isc_tpb_lock_read, table);
                    tpb->InsertTag(isc_tpb_shared);
                    break;
            case IBPP::trProtectedWrite :
                    tpb->InsertString(isc_tpb_lock_write, table);
                    tpb->InsertTag(isc_tpb_protected);
                    break;
            case IBPP::trProtectedRead :
                    tpb->InsertString(isc_tpb_lock_read, table);
                    tpb->InsertTag(isc_tpb_protected);
                    break;
            default :
                    throw LogicExceptionImpl("Transaction::AddReservation",
                        _("Illegal TTR value detected."));
        }
    }
    else throw LogicExceptionImpl("Transaction::AddReservation",
            _("The database connection you specified is not attached to this transaction."));
}

void TransactionImplFb3::Start()
{
    // Already started anyway?
    if (mTra != 0)
        return;

    if (mDatabases.empty())
        throw LogicExceptionImpl("Transaction::Start", _("No Database is attached."));

    Firebird::IMaster* master = FactoriesImplFb3::gMaster;
    Firebird::IDtc* dtc = master->getDtc();
    Firebird::IDtcStart* dtcStart = dtc->startBuilder(mStatus);
    if (mStatus->isDirty())
       throw SQLExceptionImpl(mStatus, "Transaction::Start", "IDtc::startBuilder failed.");

    unsigned i;
    Firebird::IAttachment* atm;
    for (i = 0; i < mDatabases.size(); i++)
    {
        atm = mDatabases[i]->GetFbIntf();
        if (atm == nullptr)
        {
            // All Databases must be connected to Start the transaction !
            throw LogicExceptionImpl("Transaction::Start",
                    _("All attached Database should have been connected."));
        }
        dtcStart->addAttachment(mStatus, atm);
        if (mStatus->isDirty())
           throw SQLExceptionImpl(mStatus, "Transaction::Start", "IDtcStart::addAttachment failed.");
    }

    mTra = dtcStart->start(mStatus);
    if (mStatus->isDirty())
    {
        // Should be, but better be sure...
        mTra = nullptr;
        mHandle = 0;
        throw SQLExceptionImpl(mStatus, "Transaction::Start", "IDtcStart::start failed.");
    }
    // FIXME: remove m_get_transaction_handle call
    IBS status;
    FactoriesImplFb3::m_get_transaction_handle(status.Self(), &mHandle, mTra);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Transaction::Start", _("fb_get_transaction_handle failed"));
}

void TransactionImplFb3::Commit()
{
    if (mTra == nullptr)
        throw LogicExceptionImpl("Transaction::Commit", _("Transaction is not started."));

    mTra->commit(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Transaction::Commit");
    // Should be, better be sure
    mHandle = 0;    
    mTra = nullptr;
}

void TransactionImplFb3::CommitRetain()
{
    if (mTra == nullptr)
        throw LogicExceptionImpl("Transaction::CommitRetain", _("Transaction is not started."));

    mTra->commitRetaining(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Transaction::CommitRetain");
    // Should be, better be sure
    mHandle = 0;
    mTra = nullptr;
}

void TransactionImplFb3::Rollback()
{
    // Transaction started?
    if (mTra == nullptr)
        return;   

    mTra->rollback(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Transaction::Rollback");
    // Should be, better be sure
    mHandle = 0;    
    mTra = nullptr;
}

void TransactionImplFb3::RollbackRetain()
{
    if (mTra == nullptr)
        throw LogicExceptionImpl("Transaction::RollbackRetain", _("Transaction is not started."));

    mTra->rollbackRetaining(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Transaction::RollbackRetain");
    // Should be, better be sure
    mHandle = 0;
    mTra = nullptr;
}

//  (((((((( OBJECT INTERNAL METHODS ))))))))

void TransactionImplFb3::Init()
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mStatus = new CheckStatusWrapper(master->getStatus());
    mHandle = 0;
    mTra = nullptr;
    mDatabases.clear();
    mTPBs.clear();
    mStatements.clear();
    mBlobs.clear();
    mArrays.clear();
}

void TransactionImplFb3::AttachStatementImpl(StatementImplFb3* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Transaction::AttachStatement",
                    _("Can't attach a 0 Statement object."));

    mStatements.push_back(st);
}

void TransactionImplFb3::DetachStatementImpl(StatementImplFb3* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Transaction::DetachStatement",
                _("Can't detach a 0 Statement object."));

    mStatements.erase(std::find(mStatements.begin(), mStatements.end(), st));
}

void TransactionImplFb3::AttachBlobImpl(BlobImplFb3* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Transaction::AttachBlob",
                    _("Can't attach a 0 BlobImpl object."));

    mBlobs.push_back(bb);
}

void TransactionImplFb3::DetachBlobImpl(BlobImplFb3* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Transaction::DetachBlob",
                _("Can't detach a 0 BlobImpl object."));

    mBlobs.erase(std::find(mBlobs.begin(), mBlobs.end(), bb));
}

void TransactionImplFb3::AttachArrayImpl(ArrayImplFb3* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Transaction::AttachArray",
                    _("Can't attach a 0 ArrayImpl object."));

    mArrays.push_back(ar);
}

void TransactionImplFb3::DetachArrayImpl(ArrayImplFb3* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Transaction::DetachArray",
                _("Can't detach a 0 ArrayImpl object."));

    mArrays.erase(std::find(mArrays.begin(), mArrays.end(), ar));
}

void TransactionImplFb3::AttachDatabaseImpl(DatabaseImplFb3* dbi,
    IBPP::TAM am, IBPP::TIL il, IBPP::TLR lr, IBPP::TFF flags)
{
    if (mHandle != 0)
        throw LogicExceptionImpl("Transaction::AttachDatabase",
                _("Can't attach a Database if Transaction started."));
    if (dbi == 0)
        throw LogicExceptionImpl("Transaction::AttachDatabase",
                _("Can't attach a null Database."));

    mDatabases.push_back(dbi);

    // Prepare a new TPB
    TPBFb3* tpb = new TPBFb3;
    if (am == IBPP::amRead) tpb->InsertTag(isc_tpb_read);
    else tpb->InsertTag(isc_tpb_write);

    switch (il)
    {
        case IBPP::ilConsistency :      tpb->InsertTag(isc_tpb_consistency); break;
        case IBPP::ilReadDirty :        tpb->InsertTag(isc_tpb_read_committed);
                                        tpb->InsertTag(isc_tpb_rec_version); break;
        case IBPP::ilReadCommitted :    tpb->InsertTag(isc_tpb_read_committed);
                                        tpb->InsertTag(isc_tpb_no_rec_version); break;
        default :                       tpb->InsertTag(isc_tpb_concurrency); break;
    }

    if (lr == IBPP::lrNoWait) tpb->InsertTag(isc_tpb_nowait);
    else tpb->InsertTag(isc_tpb_wait);

    if (flags & IBPP::tfIgnoreLimbo)    tpb->InsertTag(isc_tpb_ignore_limbo);
    if (flags & IBPP::tfAutoCommit)     tpb->InsertTag(isc_tpb_autocommit);
    if (flags & IBPP::tfNoAutoUndo)     tpb->InsertTag(isc_tpb_no_auto_undo);

    mTPBs.push_back(tpb);

    // Signals the Database object that it has been attached to the Transaction
    dbi->AttachTransactionImpl(this);
}

void TransactionImplFb3::DetachDatabaseImpl(DatabaseImplFb3* dbi)
{
    if (mHandle != 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach a Database if Transaction started."));
    if (dbi == 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach a null Database."));

    std::vector<DatabaseImplFb3*>::iterator pos =
        std::find(mDatabases.begin(), mDatabases.end(), dbi);
    if (pos != mDatabases.end())
    {
        size_t index = pos - mDatabases.begin();
        TPBFb3* tpb = mTPBs[index];
        mDatabases.erase(pos);
        mTPBs.erase(mTPBs.begin()+index);
        delete tpb;
    }

    // Signals the Database object that it has been detached from the Transaction
    dbi->DetachTransactionImpl(this);
}

TransactionImplFb3::TransactionImplFb3(DatabaseImplFb3* db,
    IBPP::TAM am, IBPP::TIL il, IBPP::TLR lr, IBPP::TFF flags)
{
    Init();
    AttachDatabaseImpl(db, am, il, lr, flags);
}

TransactionImplFb3::~TransactionImplFb3()
{
    // Rollback the transaction if it was Started
    try { if (Started()) Rollback(); }
        catch (...) { }

    // Let's detach cleanly all Blobs from this Transaction.
    // No Blob object can still maintain pointers to this
    // Transaction which is disappearing.
    //
    // We use a reverse traversal of the array to avoid loops.
    // The array shrinks on each loop (mBbCount decreases).
    // And during the deletion, there is a packing of the array through a
    // copy of elements from the end to the beginning of the array.
    try {
        while (mBlobs.size() > 0)
            mBlobs.back()->DetachTransactionImpl();
    } catch (...) { }

    // Let's detach cleanly all Arrays from this Transaction.
    // No Array object can still maintain pointers to this
    // Transaction which is disappearing.
    try {
        while (mArrays.size() > 0)
            mArrays.back()->DetachTransactionImpl();
    } catch (...) { }

    // Let's detach cleanly all Statements from this Transaction.
    // No Statement object can still maintain pointers to this
    // Transaction which is disappearing.
    try {
        while (mStatements.size() > 0)
            mStatements.back()->DetachTransactionImpl();
    } catch (...) { }

    // Very important : let's detach cleanly all Databases from this
    // Transaction. No Database object can still maintain pointers to this
    // Transaction which is disappearing.
    try {
        while (mDatabases.size() > 0)
        {
            size_t i = mDatabases.size()-1;
            DetachDatabaseImpl(mDatabases[i]);  // <-- remove link to database from mTPBs
                                            // array and destroy TPB object
                                            // Fixed : Maxim Abrashkin on 12 Jun 2002
            //mDatabases.back()->DetachTransaction(this);
        }
    } catch (...) { }

    delete mStatus;
}
