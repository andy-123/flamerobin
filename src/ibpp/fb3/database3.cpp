// Database class implementation
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
using namespace Firebird;

//  (((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void DatabaseImplFb3::Create(int dialect)
{
    if (mAtm != nullptr)
        throw LogicExceptionImpl("Database::Create", _("Database is already connected."));
    if (mDatabaseName.empty())
        throw LogicExceptionImpl("Database::Create", _("Unspecified database name."));
    if (dialect != 1 && dialect != 3)
        throw LogicExceptionImpl("Database::Create", _("Only dialects 1 and 3 are supported."));

    // Build the SQL Create Statement
    std::string create;
    create.assign("CREATE DATABASE '");
    if (!mServerName.empty()) create.append(mServerName).append(":");
    create.append(mDatabaseName).append("' ");

    if (!mUserName.empty())
        create.append("USER '").append(mUserName).append("' ");
    if (!mUserPassword.empty())
        create.append("PASSWORD '").append(mUserPassword).append("' ");
    if (!mCreateParams.empty()) create.append(mCreateParams);

    // Call ExecuteImmediate to create the database
    IUtil* utl = FactoriesImplFb3::gUtil;
    mAtm = utl->executeCreateDatabase(mStatus, create.length(), create.c_str(), dialect, nullptr);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Create", _("IUtil::executeCreateDatabase failed"));

    Disconnect();
}

void DatabaseImplFb3::Connect()
{
    // Already connected?
    if (mAtm != nullptr)
        return;   

    if (mDatabaseName.empty())
        throw LogicExceptionImpl("Database::Connect", _("Unspecified database name."));

    // Build a DPB based on the properties
    DPBFb3 dpb;
    if (!mUserName.empty())
    {
        dpb.InsertString(isc_dpb_user_name, mUserName.c_str());
        dpb.InsertString(isc_dpb_password, mUserPassword.c_str());
    }
    else
        dpb.InsertBool(isc_dpb_trusted_auth, true);
    if (!mRoleName.empty())
        dpb.InsertString(isc_dpb_sql_role_name, mRoleName.c_str());
    if (!mCharSet.empty())
        dpb.InsertString(isc_dpb_lc_ctype, mCharSet.c_str());

    std::string connect;
    if (!mServerName.empty())
        connect.assign(mServerName).append(":");
    connect.append(mDatabaseName);

    IMaster* master = FactoriesImplFb3::gMaster;
    IProvider* prov = master->getDispatcher();
    mAtm = prov->attachDatabase(mStatus, connect.c_str(),
                                dpb.GetBufferLength(), dpb.GetBuffer());
    if (mStatus->isDirty())
    {
        mAtm = nullptr;
        mHandle = 0;     // Should be, but better be sure...
        throw SQLExceptionImpl(mStatus, "Database::Connect", _("isc_attach_database failed"));
    }
    // FIXME: remove m_get_database_handle call
    IBS status;
    FactoriesImplFb3::m_get_database_handle(status.Self(), &mHandle, mAtm);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Database::Connect", _("fb_get_database_handle failed"));

    // Now, get ODS version information and dialect.
    // If ODS major is lower of equal to 9, we reject the connection.
    // If ODS major is 10 or higher, this is at least an InterBase 6.x Server
    // OR FireBird 1.x Server.

    const unsigned char items[] =
    {
        isc_info_ods_version,
        isc_info_db_sql_dialect,
        isc_info_end
    };

    RB result(100);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
    {
        mAtm->detach(mStatus);
        mAtm = nullptr; // Should be, but better be sure...
        mHandle = 0;
        throw SQLExceptionImpl(mStatus, "Database::Connect", _("IAttachment::getInfo failed."));
    }

    int ODS = result.GetValue(isc_info_ods_version);
    if (ODS <= 9)
    {
        mAtm->detach(mStatus);
        mAtm = nullptr; // Should be, but better be sure...
        mHandle = 0;
        throw LogicExceptionImpl("Database::Connect",
            _("Unsupported Server : wrong ODS version (%d), at least '10' required."), ODS);
    }

    mDialect = result.GetValue(isc_info_db_sql_dialect);
    if (mDialect != 1 && mDialect != 3)
    {
        mAtm->detach(mStatus);
        mAtm = nullptr; // Should be, but better be sure...
        mHandle = 0;
        throw LogicExceptionImpl("Database::Connect", _("Dialect 1 or 3 required"));
    }
}

void DatabaseImplFb3::Inactivate()
{
    if (mAtm == nullptr)
        return;   // Not connected anyway

    // Rollback any started transaction...
    for (unsigned i = 0; i < mTransactions.size(); i++)
    {
        if (mTransactions[i]->Started())
            mTransactions[i]->Rollback();
    }

    // Cancel all pending event traps
    for (unsigned i = 0; i < mEvents.size(); i++)
        mEvents[i]->Clear();

    // Let's detach from all Blobs
    while (mBlobs.size() > 0)
        mBlobs.back()->DetachDatabaseImpl();

    // Let's detach from all Arrays
    while (mArrays.size() > 0)
        mArrays.back()->DetachDatabaseImpl();

    // Let's detach from all Statements
    while (mStatements.size() > 0)
        mStatements.back()->DetachDatabaseImpl();

    // Let's detach from all Transactions
    while (mTransactions.size() > 0)
        mTransactions.back()->DetachDatabaseImpl(this);

    // Let's detach from all Events
    while (mEvents.size() > 0)
        mEvents.back()->DetachDatabaseImpl();
}

void DatabaseImplFb3::Disconnect()
{
    if (mAtm == nullptr)
        return;   // Not connected anyway

    // Put the connection to rest
    Inactivate();

    // Detach from the server
    mAtm->detach(mStatus);

    // Should we throw, set mHandle to 0 first, because Disconnect() may
    // be called from Database destructor (keeps the object coherent).
    mHandle = 0;
    mAtm = nullptr;
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Disconnect", _("IAttachment::detach failed."));
}

void DatabaseImplFb3::Drop()
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::Drop", _("Database must be connected."));

    // Put the connection to a rest
    Inactivate();

    mAtm->dropDatabase(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Drop", _("IAttachment::dropDatabase failed"));

    mHandle = 0;
    mAtm = nullptr;
}

IBPP::IDatabase * DatabaseImplFb3::Clone()
{
    // By definition the clone of an IBPP Database is a new Database.
    DatabaseImplFb3* clone = new DatabaseImplFb3(mServerName, mDatabaseName,
        mUserName, mUserPassword, mRoleName,
        mCharSet, mCreateParams);
    return clone;
}

void DatabaseImplFb3::Info(int* ODSMajor, int* ODSMinor,
    int* PageSize, int* Pages, int* Buffers, int* Sweep,
    bool* Sync, bool* Reserve, bool* ReadOnly)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::Info", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_ods_version,
        isc_info_ods_minor_version,
        isc_info_page_size,
        isc_info_allocation,
        isc_info_num_buffers,
        isc_info_sweep_interval,
        isc_info_forced_writes,
        isc_info_no_reserve,
        isc_info_db_read_only,
        isc_info_end
    };

    RB result(256);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Info", _("isc_database_info failed"));

    if (ODSMajor != 0) *ODSMajor = result.GetValue(isc_info_ods_version);
    if (ODSMinor != 0) *ODSMinor = result.GetValue(isc_info_ods_minor_version);
    if (PageSize != 0) *PageSize = result.GetValue(isc_info_page_size);
    if (Pages != 0) *Pages = result.GetValue(isc_info_allocation);
    if (Buffers != 0) *Buffers = result.GetValue(isc_info_num_buffers);
    if (Sweep != 0) *Sweep = result.GetValue(isc_info_sweep_interval);
    if (Sync != 0)
        *Sync = result.GetValue(isc_info_forced_writes) == 1 ? true : false;
    if (Reserve != 0)
        *Reserve = result.GetValue(isc_info_no_reserve) == 1 ? false : true;
    if (ReadOnly != 0)
        *ReadOnly = result.GetValue(isc_info_db_read_only) == 1 ? true : false;
}

void DatabaseImplFb3::TransactionInfo(int* Oldest, int* OldestActive,
    int* OldestSnapshot, int* Next)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::TransactionInfo", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_oldest_transaction,
        isc_info_oldest_active,
        isc_info_oldest_snapshot,
        isc_info_next_transaction,
        isc_info_end
    };

    RB result(256);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::TransactionInfo", _("isc_database_info failed"));

    if (Oldest != 0)
        *Oldest = result.GetValue(isc_info_oldest_transaction);
    if (OldestActive != 0)
        *OldestActive = result.GetValue(isc_info_oldest_active);
    if (OldestSnapshot != 0)
        *OldestSnapshot = result.GetValue(isc_info_oldest_snapshot);
    if (Next != 0)
        *Next = result.GetValue(isc_info_next_transaction);
}

void DatabaseImplFb3::Statistics(int* Fetches, int* Marks, int* Reads, int* Writes, int* CurrentMemory)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::Statistics", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_fetches,
        isc_info_marks,
        isc_info_reads,
        isc_info_writes,
        isc_info_current_memory,
        isc_info_end
    };

    RB result(256);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Statistics", _("isc_database_info failed"));

    if (Fetches != 0) *Fetches = result.GetValue(isc_info_fetches);
    if (Marks != 0) *Marks = result.GetValue(isc_info_marks);
    if (Reads != 0) *Reads = result.GetValue(isc_info_reads);
    if (Writes != 0) *Writes = result.GetValue(isc_info_writes);
    if (CurrentMemory != 0) *CurrentMemory = result.GetValue(isc_info_current_memory);
}

void DatabaseImplFb3::Counts(int* Insert, int* Update, int* Delete,
    int* ReadIdx, int* ReadSeq)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::Counts", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_insert_count,
        isc_info_update_count,
        isc_info_delete_count,
        isc_info_read_idx_count,
        isc_info_read_seq_count,
        isc_info_end
    };

    RB result(1024);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Counts", _("isc_database_info failed"));

    if (Insert != 0) *Insert = result.GetCountValue(isc_info_insert_count);
    if (Update != 0) *Update = result.GetCountValue(isc_info_update_count);
    if (Delete != 0) *Delete = result.GetCountValue(isc_info_delete_count);
    if (ReadIdx != 0) *ReadIdx = result.GetCountValue(isc_info_read_idx_count);
    if (ReadSeq != 0) *ReadSeq = result.GetCountValue(isc_info_read_seq_count);
}

void DatabaseImplFb3::DetailedCounts(IBPP::DatabaseCounts& counts)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::DetailedCounts", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_insert_count,
        isc_info_update_count,
        isc_info_delete_count,
        isc_info_read_idx_count,
        isc_info_read_seq_count,
        isc_info_end
    };

    RB result(1024);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::DetailedCounts", _("isc_database_info failed"));

    result.GetDetailedCounts(counts, isc_info_insert_count);
    result.GetDetailedCounts(counts, isc_info_update_count);
    result.GetDetailedCounts(counts, isc_info_delete_count);
    result.GetDetailedCounts(counts, isc_info_read_idx_count);
    result.GetDetailedCounts(counts, isc_info_read_seq_count);
}

void DatabaseImplFb3::Users(std::vector<std::string>& users)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Database::Users", _("Database is not connected."));

    const unsigned char items[] =
    {
        isc_info_user_names,
        isc_info_end
    };

    RB result(8000);
    mAtm->getInfo(mStatus, sizeof(items), items, result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Database::Users", _("isc_database_info failed"));

    users.clear();
    char* p = result.Self();
    while (*p == isc_info_user_names)
    {
        p += 3;     // Get to the length byte (there are two undocumented bytes which we skip)
        int len = (int)(*p);
        ++p;        // Get to the first char of username
        if (len != 0) users.push_back(std::string().append(p, len));
        p += len;   // Skip username
    }
    return;
}

//  (((((((( OBJECT INTERNAL METHODS ))))))))

void DatabaseImplFb3::AttachTransactionImpl(TransactionImplFb3* tr)
{
    if (tr == 0)
        throw LogicExceptionImpl("Database::AttachTransaction",
                    _("Transaction object is null."));

    mTransactions.push_back(tr);
}

void DatabaseImplFb3::DetachTransactionImpl(TransactionImplFb3* tr)
{
    if (tr == 0)
        throw LogicExceptionImpl("Database::DetachTransaction",
                _("ITransaction object is null."));

    mTransactions.erase(std::find(mTransactions.begin(), mTransactions.end(), tr));
}

void DatabaseImplFb3::AttachStatementImpl(StatementImplFb3* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Database::AttachStatement",
                    _("Can't attach a null Statement object."));

    mStatements.push_back(st);
}

void DatabaseImplFb3::DetachStatementImpl(StatementImplFb3* st)
{
    if (st == 0)
        throw LogicExceptionImpl("Database::DetachStatement",
                _("Can't detach a null Statement object."));

    mStatements.erase(std::find(mStatements.begin(), mStatements.end(), st));
}

void DatabaseImplFb3::AttachBlobImpl(BlobImplFb3* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Database::AttachBlob",
                    _("Can't attach a null Blob object."));

    mBlobs.push_back(bb);
}

void DatabaseImplFb3::DetachBlobImpl(BlobImplFb3* bb)
{
    if (bb == 0)
        throw LogicExceptionImpl("Database::DetachBlob",
                _("Can't detach a null Blob object."));

    mBlobs.erase(std::find(mBlobs.begin(), mBlobs.end(), bb));
}

void DatabaseImplFb3::AttachArrayImpl(ArrayImplFb3* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Database::AttachArray",
                    _("Can't attach a null Array object."));

    mArrays.push_back(ar);
}

void DatabaseImplFb3::DetachArrayImpl(ArrayImplFb3* ar)
{
    if (ar == 0)
        throw LogicExceptionImpl("Database::DetachArray",
                _("Can't detach a null Array object."));

    mArrays.erase(std::find(mArrays.begin(), mArrays.end(), ar));
}

void DatabaseImplFb3::AttachEventsImpl(EventsImplFb3* ev)
{
    if (ev == 0)
        throw LogicExceptionImpl("Database::AttachEventsImpl",
                    _("Can't attach a null Events object."));

    mEvents.push_back(ev);
}

void DatabaseImplFb3::DetachEventsImpl(EventsImplFb3* ev)
{
    if (ev == 0)
        throw LogicExceptionImpl("Database::DetachEventsImpl",
                _("Can't detach a null Events object."));

    mEvents.erase(std::find(mEvents.begin(), mEvents.end(), ev));
}

isc_db_handle DatabaseImplFb3::GetHandle()
{
    if (mAtm == nullptr)
        return 0;

    isc_db_handle h;
    IBS status;
    FactoriesImplFb3::m_get_database_handle(status.Self(), &h, mAtm);
    if (status.Errors())
        throw SQLExceptionImpl(status, "Database::GetHandle", _("fb_get_database_handle failed"));

    return h;
}

DatabaseImplFb3::DatabaseImplFb3(const std::string& ServerName, const std::string& DatabaseName,
                           const std::string& UserName, const std::string& UserPassword,
                           const std::string& RoleName, const std::string& CharSet,
                           const std::string& CreateParams) :

    mAtm(nullptr), mHandle(0),
    mServerName(ServerName), mDatabaseName(DatabaseName),
    mUserName(UserName), mUserPassword(UserPassword), mRoleName(RoleName),
    mCharSet(CharSet), mCreateParams(CreateParams),
    mDialect(3)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mStatus = new CheckStatusWrapper(master->getStatus());
}

DatabaseImplFb3::~DatabaseImplFb3()
{
    try { if (Connected()) Disconnect(); }
        catch(...) { }

    delete mStatus;
}
