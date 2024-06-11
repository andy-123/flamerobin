//   IBPP internal declarations
//
//  'Internal declarations' means everything used to implement ibpp. This
//   file and its contents is NOT needed by users of the library. All those
//   declarations are wrapped in a namespace : 'ibpp_internals'.

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
#ifndef __INTERNAL_IBPPFB3_H__
#define __INTERNAL_IBPPFB3_H__

#include "_ibpp.h"

// Firebird 3+ interfaces
#include "../firebird/include/firebird/Interface.h"

class DatabaseImplFb3;
class TransactionImplFb3;
class StatementImplFb3;
class BlobImplFb3;
class ArrayImplFb3;
class EventsImplFb3;

using namespace ibpp_internals;
using namespace IBPP;
using namespace Firebird;

//
//  Database Parameter Block (used to define a database)
//

class DPBFb3
{
    // Dynamically allocated DPB structure
    IXpbBuilder* mDPB;
    ThrowStatusWrapper* mStatus;
public:
    void InsertString(unsigned char tag, const std::string& str);
    //void Insert(char, int16_t);
    void InsertBool(unsigned char tag, bool data);
    //void Insert(char, char);

    void Clear();               
    const unsigned char* GetBuffer();
    unsigned GetBufferLength();

    DPBFb3();
    ~DPBFb3();
};

//
//  Transaction Parameter Block (used to define a transaction)
//

class TPBFb3
{
    // Dynamically allocated DPB structure
    IXpbBuilder* mTPB;
    ThrowStatusWrapper* mStatus;
public:
    void InsertTag(const unsigned char tag);
    void InsertString(unsigned char tag, const std::string& str);
    void Clear();
    const unsigned char* GetBuffer();
    unsigned GetBufferLength();

    TPBFb3();
    ~TPBFb3();
};

class ServiceImplFb3 : public IBPP::IService
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    isc_svc_handle mHandle;     // Firebird API Service Handle
    std::string mServerName;    // Server Name
    std::string mUserName;      // User Name
    std::string mUserPassword;  // User Password
    std::string mWaitMessage;   // Progress message returned by WaitMsg()
    std::string mRoleName;      // Role used for the duration of the connection
    std::string mCharSet;       // Character Set used for the connection

    int major_ver;
    int minor_ver;
    int rev_no;
    int build_no;


    isc_svc_handle* GetHandlePtr() { return &mHandle; }
    void SetServerName(const char*);
    void SetUserName(const char*);
    void SetUserPassword(const char*);
    void SetCharSet(const char*);
    void SetRoleName(const char*);


public:
    isc_svc_handle GetHandle() { return mHandle; }

    ServiceImplFb3(const std::string& ServerName, const std::string& UserName,
                const std::string& UserPassword, const std::string& RoleName,
                const std::string& CharSet
        );
    ~ServiceImplFb3();
    FBCLIENT getGDS() const { return gds; };

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void Connect();
    bool Connected() { return mHandle == 0 ? false : true; }
    void Disconnect();

    void GetVersion(std::string& version);
    bool versionIsHigherOrEqualTo(int versionMajor, int versionMinor);

    void AddUser(const IBPP::User&);
    void GetUser(IBPP::User&);
    void GetUsers(std::vector<IBPP::User>&);
    void ModifyUser(const IBPP::User&);
    void RemoveUser(const std::string& username);

    void SetPageBuffers(const std::string& dbfile, int buffers);
    void SetSweepInterval(const std::string& dbfile, int sweep);
    void SetSyncWrite(const std::string& dbfile, bool);
    void SetReadOnly(const std::string& dbfile, bool);
    void SetReserveSpace(const std::string& dbfile, bool);

    void Shutdown(const std::string& dbfile, IBPP::DSM flags,  int sectimeout);
    void Restart(const std::string& dbfile, IBPP::DSM flags);
    void Sweep(const std::string& dbfile);
    void Repair(const std::string& dbfile, IBPP::RPF flags);

    void StartBackup(
        const std::string& dbfile, const std::string& bkfile, const std::string& outfile = "",
        const int factor = 0, IBPP::BRF flags = IBPP::BRF(0),
        const std::string& cryptName = "", const std::string& keyHolder = "", const std::string& keyName = "",
        const std::string& skipData = "", const std::string& includeData = "", const int verboseInteval = 0,
        const int parallelWorkers = 0
    );
    void StartRestore(
        const std::string& bkfile, const std::string& dbfile,  const std::string& outfile = "",
        int pagesize = 0, int buffers = 0, IBPP::BRF flags = IBPP::BRF(0),
        const std::string& cryptName = "", const std::string& keyHolder = "", const std::string& keyName = "",
        const std::string& skipData = "", const std::string& includeData = "", const int verboseInteval = 0,
        const int parallelWorkers = 0
    );

    const char* WaitMsg();
    void Wait();
};

class DatabaseImplFb3 : public IBPP::IDatabase
{
    //  (((((((( OBJECT INTERNALS ))))))))

    FBCLIENT gdsM;	// Local GDS instance

    CheckStatusWrapper* mStatus;
    IAttachment* mAtm;
    isc_db_handle mHandle;      // InterBase API Session Handle
    std::string mServerName;    // Server name
    std::string mDatabaseName;  // Database name (path/file)
    std::string mUserName;      // User name
    std::string mUserPassword;  // User password
    std::string mRoleName;      // Role used for the duration of the connection
    std::string mCharSet;       // Character Set used for the connection
    std::string mCreateParams;  // Other parameters (creation only)

    int mDialect;                           // 1 if IB5, 1 or 3 if IB6/FB1
    std::vector<TransactionImplFb3*> mTransactions;// Table of Transaction*
    std::vector<StatementImplFb3*> mStatements;// Table of Statement*
    std::vector<BlobImplFb3*> mBlobs;          // Table of Blob*
    std::vector<ArrayImplFb3*> mArrays;        // Table of Array*
    std::vector<EventsImplFb3*> mEvents;       // Table of Events*

public:
    isc_db_handle* GetHandlePtr() { return &mHandle; }
    // array-functions has no interface - to support this
    // we need the old isc_db_handle
    isc_db_handle GetHandle();
    Firebird::IAttachment* GetFbIntf() { return mAtm; }

    void AttachTransactionImpl(TransactionImplFb3*);
    void DetachTransactionImpl(TransactionImplFb3*);
    void AttachStatementImpl(StatementImplFb3*);
    void DetachStatementImpl(StatementImplFb3*);
    void AttachBlobImpl(BlobImplFb3*);
    void DetachBlobImpl(BlobImplFb3*);
    void AttachArrayImpl(ArrayImplFb3*);
    void DetachArrayImpl(ArrayImplFb3*);
    void AttachEventsImpl(EventsImplFb3*);
    void DetachEventsImpl(EventsImplFb3*);

    DatabaseImplFb3(const std::string& ServerName, const std::string& DatabaseName,
                const std::string& UserName, const std::string& UserPassword,
                const std::string& RoleName, const std::string& CharSet,
                const std::string& CreateParams);
    ~DatabaseImplFb3();
    FBCLIENT getGDS() const { return gds; };

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    const char* ServerName() const      { return mServerName.c_str(); }
    const char* DatabaseName() const    { return mDatabaseName.c_str(); }
    const char* Username() const        { return mUserName.c_str(); }
    const char* UserPassword() const    { return mUserPassword.c_str(); }
    const char* RoleName() const        { return mRoleName.c_str(); }
    const char* CharSet() const         { return mCharSet.c_str(); }
    const char* CreateParams() const    { return mCreateParams.c_str(); }

    void Info(int* ODSMajor, int* ODSMinor,
        int* PageSize, int* Pages, int* Buffers, int* Sweep,
        bool* SyncWrites, bool* Reserve, bool* ReadOnly);
    void TransactionInfo(int* Oldest, int* OldestActive,
        int* OldestSnapshot, int* Next);
    void Statistics(int* Fetches, int* Marks, int* Reads, int* Writes, int* CurrentMemory);
    void Counts(int* Insert, int* Update, int* Delete,
        int* ReadIdx, int* ReadSeq);
    void DetailedCounts(IBPP::DatabaseCounts& counts);
    void Users(std::vector<std::string>& users);
    int Dialect() { return mDialect; }

    void Create(int dialect);
    void Connect();
    bool Connected() { return mAtm == nullptr ? false : true; }
    void Inactivate();
    void Disconnect();
    void Drop();
    IBPP::IDatabase* Clone();
};

class TransactionImplFb3 : public IBPP::ITransaction
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    CheckStatusWrapper* mStatus;
    Firebird::ITransaction* mTra;
    isc_tr_handle mHandle;          // Transaction InterBase

    std::vector<DatabaseImplFb3*> mDatabases;      // Table of IDatabase*
    std::vector<StatementImplFb3*> mStatements;    // Table of IStatement*
    std::vector<BlobImplFb3*> mBlobs;              // Table of IBlob*
    std::vector<ArrayImplFb3*> mArrays;            // Table of Array*
    std::vector<TPBFb3*> mTPBs;                    // Table of TPB

    void Init();            // A usage exclusif des constructeurs

public:
    isc_tr_handle* GetHandlePtr() { return &mHandle; }
    isc_tr_handle GetHandle() { return mHandle; }
    Firebird::ITransaction* GetFbIntf() { return mTra; }

    void AttachStatementImpl(StatementImplFb3*);
    void DetachStatementImpl(StatementImplFb3*);
    void AttachBlobImpl(BlobImplFb3*);
    void DetachBlobImpl(BlobImplFb3*);
    void AttachArrayImpl(ArrayImplFb3*);
    void DetachArrayImpl(ArrayImplFb3*);
    void AttachDatabaseImpl(DatabaseImplFb3* dbi, IBPP::TAM am = IBPP::amWrite,
            IBPP::TIL il = IBPP::ilConcurrency,
            IBPP::TLR lr = IBPP::lrWait, IBPP::TFF flags = IBPP::TFF(0));
    void DetachDatabaseImpl(DatabaseImplFb3* dbi);

    TransactionImplFb3(DatabaseImplFb3* db, IBPP::TAM am = IBPP::amWrite,
        IBPP::TIL il = IBPP::ilConcurrency,
        IBPP::TLR lr = IBPP::lrWait, IBPP::TFF flags = IBPP::TFF(0));
    ~TransactionImplFb3();

    FBCLIENT getGDS() const { return gds; };

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void AttachDatabase(IBPP::Database db, IBPP::TAM am = IBPP::amWrite,
            IBPP::TIL il = IBPP::ilConcurrency,
            IBPP::TLR lr = IBPP::lrWait, IBPP::TFF flags = IBPP::TFF(0));
    void DetachDatabase(IBPP::Database db);
    void AddReservation(IBPP::Database db,
            const std::string& table, IBPP::TTR tr);

    void Start();
    bool Started() { return mHandle == 0 ? false : true; }
    void Commit();
    void Rollback();
    void CommitRetain();
    void RollbackRetain();
};

class RowImplFb3 : public IBPP::IRow
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    XSQLDA* mDescrArea;             // XSQLDA descriptor itself
    std::vector<double> mNumerics;  // Temporary storage for Numerics
    std::vector<float> mFloats;     // Temporary storage for Floats
    std::vector<int64_t> mInt64s;   // Temporary storage for 64 bits
    std::vector<int32_t> mInt32s;   // Temporary storage for 32 bits
    std::vector<int16_t> mInt16s;   // Temporary storage for 16 bits
    std::vector<char> mBools;       // Temporary storage for Bools
    std::vector<std::string> mStrings;  // Temporary storage for Strings
    std::vector<bool> mUpdated;     // Which columns where updated (Set()) ?

    int mDialect;                   // Related database dialect
    DatabaseImplFb3* mDatabase;        // Related Database (important for Blobs, ...)
    TransactionImplFb3* mTransaction;  // Related Transaction (same remark)

    void SetValue(int, IITYPE, const void* value, int = 0);
    void* GetValue(int, IITYPE, void* = 0);

public:
    void Free();
    short AllocatedSize() { return mDescrArea->sqln; }
    void Resize(int n);
    void AllocVariables();
    bool MissingValues();       // Returns wether one of the mMissing[] is true
    XSQLDA* Self() { return mDescrArea; }

    RowImplFb3& operator=(const RowImplFb3& copied);
    RowImplFb3(const RowImplFb3& copied);
    RowImplFb3(int dialect, int size, DatabaseImplFb3* db, TransactionImplFb3* tr);
    ~RowImplFb3();

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void SetNull(int);
    void Set(int, bool);
    void Set(int, const char*);             // c-strings
    void Set(int, const void*, int);        // byte buffers
    void Set(int, const std::string&);
    void Set(int, int16_t);
    void Set(int, int32_t);
    void Set(int, int64_t);
    void Set(int, IBPP::ibpp_int128_t);
    void Set(int, float);
    void Set(int, double);
    void Set(int, IBPP::ibpp_dec16_t);
    void Set(int, IBPP::ibpp_dec34_t);
    void Set(int, const IBPP::Timestamp&);
    void Set(int, const IBPP::Date&);
    void Set(int, const IBPP::Time&);
    void Set(int, const IBPP::DBKey&);
    void Set(int, const IBPP::Blob&);
    void Set(int, const IBPP::Array&);

    bool IsNull(int);
    bool Get(int, bool&);
    bool Get(int, char*);       // c-strings, len unchecked
    bool Get(int, void*, int&); // byte buffers
    bool Get(int, std::string&);
    bool Get(int, int16_t&);
    bool Get(int, int32_t&);
    bool Get(int, int64_t&);
    bool Get(int, IBPP::ibpp_int128_t&);
    bool Get(int, float&);
    bool Get(int, double&);
    bool Get(int, IBPP::ibpp_dec16_t&);
    bool Get(int, IBPP::ibpp_dec34_t&);
    bool Get(int, IBPP::Timestamp&);
    bool Get(int, IBPP::Date&);
    bool Get(int, IBPP::Time&);
    bool Get(int, IBPP::DBKey&);
    bool Get(int, IBPP::Blob&);
    bool Get(int, IBPP::Array&);

    bool IsNull(const std::string&);
    bool Get(const std::string&, bool&);
    bool Get(const std::string&, char*);    // c-strings, len unchecked
    bool Get(const std::string&, void*, int&);  // byte buffers
    bool Get(const std::string&, std::string&);
    bool Get(const std::string&, int16_t&);
    bool Get(const std::string&, int32_t&);
    bool Get(const std::string&, int64_t&);
    void Get(const std::string&, IBPP::ibpp_int128_t&);
    bool Get(const std::string&, float&);
    bool Get(const std::string&, double&);
    bool Get(const std::string&, IBPP::Timestamp&);
    bool Get(const std::string&, IBPP::Date&);
    bool Get(const std::string&, IBPP::Time&);
    bool Get(const std::string&, IBPP::DBKey&);
    bool Get(const std::string&, IBPP::Blob&);
    bool Get(const std::string&, IBPP::Array&);

    int ColumnNum(const std::string&);
    const char* ColumnName(int);
    const char* ColumnAlias(int);
    const char* ColumnTable(int);
    IBPP::SDT ColumnType(int);
    int ColumnSQLType(int);
    int ColumnSubtype(int);
    int ColumnSize(int);
    int ColumnScale(int);
    int Columns();

    bool ColumnUpdated(int);
    bool Updated();

    IBPP::Database DatabasePtr() const;
    IBPP::Transaction TransactionPtr() const;

    IBPP::IRow* Clone();
};

class StatementImplFb3 : public IBPP::IStatement
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    friend class TransactionImplFb3;

    isc_stmt_handle mHandle;    // Statement Handle

    DatabaseImplFb3* mDatabase;        // Attached database
    TransactionImplFb3* mTransaction;  // Attached transaction
    RowImplFb3* mInRow;
    //bool* mInMissing;         // Quels paramètres n'ont pas été spécifiés
    RowImplFb3* mOutRow;
    bool mResultSetAvailable;   // Executed and result set is available
    bool mCursorOpened;         // dsql_set_cursor_name was called
    IBPP::STT mType;            // Type de requète
    std::string mSql;           // Last SQL statement prepared or executed

    // Internal Methods
    void CursorFree();

public:
    // Properties and Attributes Access Methods
    isc_stmt_handle GetHandle() { return mHandle; }

    void AttachDatabaseImpl(DatabaseImplFb3*);
    void DetachDatabaseImpl();
    void AttachTransactionImpl(TransactionImplFb3*);
    void DetachTransactionImpl();

    StatementImplFb3(DatabaseImplFb3*, TransactionImplFb3*);
    ~StatementImplFb3();
    FBCLIENT getGDS() const { return mDatabase->getGDS(); };

    //  (((((((( OBJECT INTERFACE ))))))))

public:

    void Prepare(const std::string& sql);
    void Execute(const std::string& sql);
    inline void Execute()   { Execute(std::string()); }
    void ExecuteImmediate(const std::string&);
    void CursorExecute(const std::string& cursor, const std::string& sql);
    inline void CursorExecute(const std::string& cursor)    { CursorExecute(cursor, std::string()); }
    bool Fetch();
    bool Fetch(IBPP::Row&);
    int AffectedRows();
    void Close();   // Free resources, attachments maintained
    std::string& Sql() { return mSql; }
    IBPP::STT Type() { return mType; }

    void SetNull(int);
    void Set(int, bool);
    void Set(int, const char*);             // c-strings
    void Set(int, const void*, int);        // byte buffers
    void Set(int, const std::string&);
    void Set(int, int16_t);
    void Set(int, int32_t);
    void Set(int, int64_t);
    void Set(int, IBPP::ibpp_int128_t);
    void Set(int, float);
    void Set(int, double);
    void Set(int, const IBPP::Timestamp&);
    void Set(int, const IBPP::Date&);
    void Set(int, const IBPP::Time&);
    void Set(int, const IBPP::DBKey&);
    void Set(int, const IBPP::Blob&);
    void Set(int, const IBPP::Array&);

    void SetNull(std::string);
    void Set(std::string, bool);
    void Set(std::string, const char*);             // c-strings
    void Set(std::string, const void*, int);        // byte buffers
    void Set(std::string, const std::string&);
    void Set(std::string, int16_t);
    void Set(std::string, int32_t);
    void Set(std::string, int64_t);
    void Set(std::string, IBPP::ibpp_int128_t);
    void Set(std::string, float);
    void Set(std::string, double);
    void Set(std::string, const IBPP::Timestamp&);
    void Set(std::string, const IBPP::Date&);
    void Set(std::string, const IBPP::Time&);
    void Set(std::string, const IBPP::DBKey&);
    void Set(std::string, const IBPP::Blob&);
    void Set(std::string, const IBPP::Array&);

private:
    std::string ParametersParser(std::string sql);
    std::vector<std::string> parametersByName_;
    std::vector<std::string> parametersDetailedByName_;
public:
    std::vector<std::string> ParametersByName();//Return a vector list, with the parameter(s) name, starting in 0
    std::vector<int> FindParamsByName(std::string name);  //Return a vector list, with the parameter(s) ID, starting in 1
    int ParameterNum(const std::string& name);

    bool IsNull(int);
    bool Get(int, bool*);
    bool Get(int, bool&);
    bool Get(int, char*);               // c-strings, len unchecked
    bool Get(int, void*, int&);         // byte buffers
    bool Get(int, std::string&);
    bool Get(int, int16_t*);
    bool Get(int, int16_t&);
    bool Get(int, int32_t*);
    bool Get(int, int32_t&);
    bool Get(int, int64_t*);
    bool Get(int, int64_t&);
    bool Get(int, IBPP::ibpp_int128_t&);
    bool Get(int, float*);
    bool Get(int, float&);
    bool Get(int, double*);
    bool Get(int, double&);
    bool Get(int, IBPP::ibpp_dec16_t&);
    bool Get(int, IBPP::ibpp_dec34_t&);
    bool Get(int, IBPP::Timestamp&);
    bool Get(int, IBPP::Date&);
    bool Get(int, IBPP::Time&);
    bool Get(int, IBPP::DBKey&);
    bool Get(int, IBPP::Blob&);
    bool Get(int, IBPP::Array&);

    bool IsNull(const std::string&);
    bool Get(const std::string&, bool*);
    bool Get(const std::string&, bool&);
    bool Get(const std::string&, char*);        // c-strings, len unchecked
    bool Get(const std::string&, void*, int&);  // byte buffers
    bool Get(const std::string&, std::string&);
    bool Get(const std::string&, int16_t*);
    bool Get(const std::string&, int16_t&);
    bool Get(const std::string&, int32_t*);
    bool Get(const std::string&, int32_t&);
    bool Get(const std::string&, int64_t*);
    bool Get(const std::string&, int64_t&);
    bool Get(const std::string&, float*);
    bool Get(const std::string&, float&);
    bool Get(const std::string&, double*);
    bool Get(const std::string&, double&);
    bool Get(const std::string&, IBPP::Timestamp&);
    bool Get(const std::string&, IBPP::Date&);
    bool Get(const std::string&, IBPP::Time&);
    bool Get(const std::string&, IBPP::DBKey&);
    bool Get(const std::string&, IBPP::Blob&);
    bool Get(const std::string&, IBPP::Array&);

    int ColumnNum(const std::string&);
    int ColumnNumAlias(const std::string&);
    const char* ColumnName(int);
    const char* ColumnAlias(int);
    const char* ColumnTable(int);
    IBPP::SDT ColumnType(int);
    int ColumnSQLType(int);
    int ColumnSubtype(int);
    int ColumnSize(int);
    int ColumnScale(int);
    int Columns();

    IBPP::SDT ParameterType(int);
    int ParameterSubtype(int);
    int ParameterSize(int);
    int ParameterScale(int);
    int Parameters();

    void Plan(std::string&);

    IBPP::Database DatabasePtr() const;
    IBPP::Transaction TransactionPtr() const;
};

class BlobImplFb3 : public IBPP::IBlob
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    friend class RowImplFb3;

    bool                    mIdAssigned;
    ISC_QUAD                mId;
    isc_blob_handle         mHandle;
    bool                    mWriteMode;
    DatabaseImplFb3*        mDatabase;      // Belongs to this database
    TransactionImplFb3*     mTransaction;   // Belongs to this transaction

    void Init();
    void SetId(ISC_QUAD*);
    void GetId(ISC_QUAD*);

public:
    void AttachDatabaseImpl(DatabaseImplFb3*);
    void DetachDatabaseImpl();
    void AttachTransactionImpl(TransactionImplFb3*);
    void DetachTransactionImpl();

    BlobImplFb3(const BlobImplFb3&);
    BlobImplFb3(DatabaseImplFb3*, TransactionImplFb3* = 0);
    ~BlobImplFb3();
    FBCLIENT getGDS() const { return mDatabase->getGDS(); };

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void Create();
    void Open();
    void Close();
    void Cancel();
    int Read(void*, int size);
    void Write(const void*, int size);
    void Info(int* Size, int* Largest, int* Segments);

    void Save(const std::string& data);
    void Load(std::string& data);

    IBPP::Database DatabasePtr() const;
    IBPP::Transaction TransactionPtr() const;
};

class ArrayImplFb3 : public IBPP::IArray
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    friend class RowImplFb3;

    bool                mIdAssigned;
    ISC_QUAD            mId;
    bool                mDescribed;
    ISC_ARRAY_DESC      mDesc;
    DatabaseImplFb3*    mDatabase;      // Database attachée
    TransactionImplFb3* mTransaction;   // Transaction attachée
    void*               mBuffer;        // Buffer for native data
    int                 mBufferSize;    // Size of this buffer in bytes
    int                 mElemCount;     // Count of elements in this array
    int                 mElemSize;      // Size of an element in the buffer

    void Init();
    void SetId(ISC_QUAD*);
    void GetId(ISC_QUAD*);
    void ResetId();
    void AllocArrayBuffer();

public:
    void AttachDatabaseImpl(DatabaseImplFb3*);
    void DetachDatabaseImpl();
    void AttachTransactionImpl(TransactionImplFb3*);
    void DetachTransactionImpl();

    ArrayImplFb3(const ArrayImplFb3&);
    ArrayImplFb3(DatabaseImplFb3*, TransactionImplFb3* = 0);
    ~ArrayImplFb3();
    FBCLIENT getGDS() const { return mDatabase->getGDS(); };

    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void Describe(const std::string& table, const std::string& column);
    void ReadTo(IBPP::ADT, void*, int);
    void WriteFrom(IBPP::ADT, const void*, int);
    IBPP::SDT ElementType();
    int ElementSize();
    int ElementScale();
    int Dimensions();
    void Bounds(int dim, int* low, int* high);
    void SetBounds(int dim, int low, int high);

    IBPP::Database DatabasePtr() const;
    IBPP::Transaction TransactionPtr() const;
};

//
//  EventBufferIterator: used in EventsImpl implementation.
//

template<class It>
struct EventBufferIteratorFb3
{
    It mIt;

public:
    EventBufferIteratorFb3& operator++()
        { mIt += 1 + static_cast<int>(*mIt) + 4; return *this; }

    bool operator == (const EventBufferIteratorFb3& i) const { return i.mIt == mIt; }
    bool operator != (const EventBufferIteratorFb3& i) const { return i.mIt != mIt; }

    std::string get_name() const
    {
        return std::string(mIt + 1, mIt + 1 + static_cast<int32_t>(*mIt));
    }

    uint32_t get_count() const
    {
        return (*gds.Call()->m_vax_integer)
            (const_cast<char*>(&*(mIt + 1 + static_cast<int>(*mIt))), 4);
    }

    // Those container like begin() and end() allow access to the underlying type
    It begin()  { return mIt; }
    It end()    { return mIt + 1 + static_cast<int>(*mIt) + 4; }

    EventBufferIteratorFb3() {}
    EventBufferIteratorFb3(It it) : mIt(it) {}
};

class EventsImplFb3 : public IBPP::IEvents
{
    static const size_t MAXEVENTNAMELEN;
    static void EventHandler(const char*, short, const char*);

    typedef std::vector<IBPP::EventInterface*> ObjRefs;
    ObjRefs mObjectReferences;

    typedef std::vector<char> Buffer;
    Buffer mEventBuffer;
    Buffer mResultsBuffer;

    DatabaseImplFb3* mDatabase;
    ISC_LONG mId;           // Firebird internal Id of these events
    bool mQueued;           // Has isc_que_events() been called?
    bool mTrapped;          // EventHandled() was called since last que_events()

    void FireActions();
    void Queue();
    void Cancel();

    EventsImplFb3& operator=(const EventsImplFb3&);
    EventsImplFb3(const EventsImplFb3&);

public:
    void AttachDatabaseImpl(DatabaseImplFb3*);
    void DetachDatabaseImpl();

    EventsImplFb3(DatabaseImplFb3* dbi);
    ~EventsImplFb3();
    FBCLIENT getGDS() const { return mDatabase->getGDS(); };


    //  (((((((( OBJECT INTERFACE ))))))))

public:
    void Add(const std::string&, IBPP::EventInterface*);
    void Drop(const std::string&);
    void List(std::vector<std::string>&);
    void Clear();               // Drop all events
    void Dispatch();            // Dispatch NON async events

    IBPP::Database DatabasePtr() const;
};

class FactoriesImplFb3 : public IFactories
{
public:
    FactoriesImplFb3() {};
    Service CreateService(const std::string& ServerName,
        const std::string& UserName, const std::string& UserPassword,
        const std::string& RoleName, const std::string& CharSet,
        const std::string& FBClient = "");
    Database CreateDatabase(const std::string& ServerName,
        const std::string& DatabaseName, const std::string& UserName,
        const std::string& UserPassword, const std::string& RoleName,
        const std::string& CharSet, const std::string& CreateParams,
        const std::string& FBClient = "");
    Transaction CreateTransaction(Database db, TAM am = amWrite,
        TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0));
    Statement CreateStatement(Database db, Transaction tr);
    Blob CreateBlob(Database db, Transaction tr);
    Array CreateArray(Database db, Transaction tr);
    Events CreateEvents(Database db);

private:
    static bool gIsInit;
    static bool gAvailable;
public:
    static proto_get_database_handle* m_get_database_handle;
    static proto_get_transaction_handle* m_get_transaction_handle;
    static IMaster* gMaster;
    static IUtil* gUtil;
    static bool gInit(ibpp_HMODULE h);
};

#endif