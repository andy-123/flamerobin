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
#include "fb1/ibppfb1.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <algorithm>

using namespace ibpp_internals;

//  (((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void TransactionImplFb1::AttachDatabase(IBPP::Database db,
    IBPP::TAM am, IBPP::TIL il, IBPP::TLR lr, IBPP::TFF flags)
{
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::AttachDatabase",
                _("Can't attach an unbound Database."));

    AttachDatabaseImpl(dynamic_cast<DatabaseImplFb1*>(db.intf()), am, il, lr, flags);
}

void TransactionImplFb1::DetachDatabase(IBPP::Database db)
{
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach an unbound Database."));

    DetachDatabaseImpl(dynamic_cast<DatabaseImplFb1*>(db.intf()));
}

void TransactionImplFb1::AddReservation(IBPP::Database db,
    const std::string& table, IBPP::TTR tr)
{
    if (mHandle != 0)
        throw LogicExceptionImpl("Transaction::AddReservation",
                _("Can't add table reservation if Transaction started."));
    if (db.intf() == 0)
        throw LogicExceptionImpl("Transaction::AddReservation",
                _("Can't add table reservation on an unbound Database."));

    // Find the TPB associated with this database
    std::vector<DatabaseImplFb1*>::iterator pos =
        std::find(mDatabases.begin(), mDatabases.end(), dynamic_cast<DatabaseImplFb1*>(db.intf()));
    if (pos != mDatabases.end())
    {
        size_t index = pos - mDatabases.begin();
        TPBFb1* tpb = mTPBs[index];

        // Now add the reservations to the TPB
        switch (tr)
        {
            case IBPP::trSharedWrite :
                    tpb->Insert(isc_tpb_lock_write);
                    tpb->Insert(table);
                    tpb->Insert(isc_tpb_shared);
                    break;
            case IBPP::trSharedRead :
                    tpb->Insert(isc_tpb_lock_read);
                    tpb->Insert(table);
                    tpb->Insert(isc_tpb_shared);
                    break;
            case IBPP::trProtectedWrite :
                    tpb->Insert(isc_tpb_lock_write);
                    tpb->Insert(table);
                    tpb->Insert(isc_tpb_protected);
                    break;
            case IBPP::trProtectedRead :
                    tpb->Insert(isc_tpb_lock_read);
                    tpb->Insert(table);
                    tpb->Insert(isc_tpb_protected);
                    break;
            default :
                    throw LogicExceptionImpl("Transaction::AddReservation",
                        _("Illegal TTR value detected."));
        }
    }
    else throw LogicExceptionImpl("Transaction::AddReservation",
            _("The database connection you specified is not attached to this transaction."));
}

void TransactionImplFb1::Start()
{
    if (mHandle != 0) return;   // Already started anyway

    if (mDatabases.empty())
        throw LogicExceptionImpl("Transaction::Start", _("No Database is attached."));

    struct ISC_TEB
    {
        ISC_LONG* db_ptr;
        ISC_LONG tpb_len;
        char* tpb_ptr;
    } * teb = new ISC_TEB[mDatabases.size()];

    unsigned i;
    for (i = 0; i < mDatabases.size(); i++)
    {
        if (mDatabases[i]->GetHandle() == 0)
        {
            // All Databases must be connected to Start the transaction !
            delete [] teb;
            throw LogicExceptionImpl("Transaction::Start",
                    _("All attached Database should have been connected."));
        }
        teb[i].db_ptr = (ISC_LONG*) mDatabases[i]->GetHandlePtr();
        teb[i].tpb_len = mTPBs[i]->Size();
        teb[i].tpb_ptr = mTPBs[i]->Self();
    }

    IBS status;
    (*getGDS().Call()->m_start_multiple)(status.Self(), &mHandle, (short)mDatabases.size(), teb);
    delete [] teb;
    if (status.Errors())
    {
        mHandle = 0;    // Should be, but better be sure...
        throw SQLExceptionImpl(status, "Transaction::Start");
    }
}

void TransactionImplFb1::Commit()
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Transaction::Commit", _("Transaction is not started."));

    IBS status;

    (*getGDS().Call()->m_commit_transaction)(status.Self(), &mHandle);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Transaction::Commit");
    mHandle = 0;    // Should be, better be sure

    /*
    size_t i;
    for (i = mStatements.size(); i != 0; i--)
        mStatements[i-1]->CursorFree();
    */
}

void TransactionImplFb1::CommitRetain()
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Transaction::CommitRetain", _("Transaction is not started."));

    IBS status;

    (*getGDS().Call()->m_commit_retaining)(status.Self(), &mHandle);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Transaction::CommitRetain");
}

void TransactionImplFb1::Rollback()
{
    if (mHandle == 0) return;   // Transaction not started anyway

    IBS status;

    (*getGDS().Call()->m_rollback_transaction)(status.Self(), &mHandle);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Transaction::Rollback");
    mHandle = 0;    // Should be, better be sure

    /*
    size_t i;
    for (i = mStatements.size(); i != 0; i--)
        mStatements[i-1]->CursorFree();
    */
}

void TransactionImplFb1::RollbackRetain()
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Transaction::RollbackRetain", _("Transaction is not started."));

    IBS status;

    (*getGDS().Call()->m_rollback_retaining)(status.Self(), &mHandle);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Transaction::RollbackRetain");
}

//  (((((((( OBJECT INTERNAL METHODS ))))))))

void TransactionImplFb1::Init()
{
    mHandle = 0;
    mDatabases.clear();
    mTPBs.clear();
    mStatements.clear();
    mBlobs.clear();
    mArrays.clear();
}

void TransactionImplFb1::AttachStatementImpl(StatementImplFb1* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Transaction::AttachStatement",
                    _("Can't attach a 0 Statement object."));

    mStatements.push_back(st);
}

void TransactionImplFb1::DetachStatementImpl(StatementImplFb1* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Transaction::DetachStatement",
                _("Can't detach a 0 Statement object."));

    mStatements.erase(std::find(mStatements.begin(), mStatements.end(), st));
}

void TransactionImplFb1::AttachBlobImpl(BlobImplFb1* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Transaction::AttachBlob",
                    _("Can't attach a 0 BlobImpl object."));

    mBlobs.push_back(bb);
}

void TransactionImplFb1::DetachBlobImpl(BlobImplFb1* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Transaction::DetachBlob",
                _("Can't detach a 0 BlobImpl object."));

    mBlobs.erase(std::find(mBlobs.begin(), mBlobs.end(), bb));
}

void TransactionImplFb1::AttachArrayImpl(ArrayImplFb1* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Transaction::AttachArray",
                    _("Can't attach a 0 ArrayImpl object."));

    mArrays.push_back(ar);
}

void TransactionImplFb1::DetachArrayImpl(ArrayImplFb1* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Transaction::DetachArray",
                _("Can't detach a 0 ArrayImpl object."));

    mArrays.erase(std::find(mArrays.begin(), mArrays.end(), ar));
}

void TransactionImplFb1::AttachDatabaseImpl(DatabaseImplFb1* dbi,
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
    TPBFb1* tpb = new TPBFb1;
    if (am == IBPP::amRead) tpb->Insert(isc_tpb_read);
    else tpb->Insert(isc_tpb_write);

    switch (il)
    {
        case IBPP::ilConsistency :      tpb->Insert(isc_tpb_consistency); break;
        case IBPP::ilReadDirty :        tpb->Insert(isc_tpb_read_committed);
                                        tpb->Insert(isc_tpb_rec_version); break;
        case IBPP::ilReadCommitted :    tpb->Insert(isc_tpb_read_committed);
                                        tpb->Insert(isc_tpb_no_rec_version); break;
        default :                       tpb->Insert(isc_tpb_concurrency); break;
    }

    if (lr == IBPP::lrNoWait) tpb->Insert(isc_tpb_nowait);
    else tpb->Insert(isc_tpb_wait);

    if (flags & IBPP::tfIgnoreLimbo)    tpb->Insert(isc_tpb_ignore_limbo);
    if (flags & IBPP::tfAutoCommit)     tpb->Insert(isc_tpb_autocommit);
    if (flags & IBPP::tfNoAutoUndo)     tpb->Insert(isc_tpb_no_auto_undo);

    mTPBs.push_back(tpb);

    // Signals the Database object that it has been attached to the Transaction
    dbi->AttachTransactionImpl(this);
}

void TransactionImplFb1::DetachDatabaseImpl(DatabaseImplFb1* dbi)
{
    if (mHandle != 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach a Database if Transaction started."));
    if (dbi == 0)
        throw LogicExceptionImpl("Transaction::DetachDatabase",
                _("Can't detach a null Database."));

    std::vector<DatabaseImplFb1*>::iterator pos =
        std::find(mDatabases.begin(), mDatabases.end(), dbi);
    if (pos != mDatabases.end())
    {
        size_t index = pos - mDatabases.begin();
        TPBFb1* tpb = mTPBs[index];
        mDatabases.erase(pos);
        mTPBs.erase(mTPBs.begin()+index);
        delete tpb;
    }

    // Signals the Database object that it has been detached from the Transaction
    dbi->DetachTransactionImpl(this);
}

TransactionImplFb1::TransactionImplFb1(DatabaseImplFb1* db,
    IBPP::TAM am, IBPP::TIL il, IBPP::TLR lr, IBPP::TFF flags)
{
    Init();
    AttachDatabaseImpl(db, am, il, lr, flags);
}

TransactionImplFb1::~TransactionImplFb1()
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
}