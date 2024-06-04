//   IBPP internal declarations
//
//  'Internal declarations' means everything used to implement ibpp. This
//   file and its contents is NOT needed by users of the library. All those
//   declarations are wrapped in a namespace : 'ibpp_internals'.

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
#ifndef __INTERNAL_IBPP_H__
#define __INTERNAL_IBPP_H__

#include "ibpp.h"
#include "_ibppapi.h"

#if defined(_MSC_VER) 
#define HAS_HDRSTOP
#endif

#if (defined(__GNUC__) && defined(IBPP_WINDOWS))
//  Setting flags for ibase.h -- using GCC/Cygwin/MinGW on Win32
#ifndef _MSC_VER
#define _MSC_VER 1299
#endif
#ifndef _WIN32
#define _WIN32   1
#endif
#endif

// From Firebird installation
#include "../firebird/include/ibase.h"
#ifdef FIXME
sollte mal entfernt werden -> nach ibppfb3.h wandern
#endif
// Firebird 3+ interfaces
#include "../firebird/include/firebird/Interface.h"

#if (defined(__GNUC__) && defined(IBPP_WINDOWS))
//  UNSETTING flags used above for ibase.h -- Huge conflicts with libstdc++ !
#undef _MSC_VER
#undef _WIN32
#endif

#ifdef IBPP_WINDOWS
#include <windows.h>
#endif

#include <limits>
#include <string>
#include <vector>
#include <sstream>
#include <cstdarg>
#include <cstring>


#ifdef _DEBUG
#define ASSERTION(x)    {if (!(x)) {throw LogicExceptionImpl("ASSERTION", \
                            "'"#x"' is not verified at %s, line %d", \
                                __FILE__, __LINE__);}}
#else
#define ASSERTION(x)    /* x */
#endif

namespace ibpp_internals
{

enum flush_debug_stream_type {fds};

#ifdef _DEBUG

#if defined (_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning ( push )
#pragma warning ( disable: 4250 )
#endif

struct DebugStream : public std::stringstream
{
    // next two operators fix some g++ and vc++ related problems
    std::ostream& operator<< (const char* p)
        { static_cast<std::stringstream&>(*this)<< p; return *this; }

    std::ostream& operator<< (const std::string& p)
        { static_cast<std::stringstream&>(*this)<< p; return *this; }

    DebugStream& operator=(const DebugStream&) {return *this;}
    DebugStream(const DebugStream&) {}
    DebugStream() {}
};
std::ostream& operator<< (std::ostream& a, flush_debug_stream_type);

#if defined (_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning ( pop )
#endif

#else

struct DebugStream
{
    template<class T> DebugStream& operator<< (const T&) { return *this; }
    // for manipulators
    DebugStream& operator<< (std::ostream&(*)(std::ostream&)) { return *this; }
};

#endif  // _DEBUG

//  Native data types
typedef enum {ivArray, ivBlob, ivDate, ivTime, ivTimestamp, ivString,
            ivInt16, ivInt32, ivInt64, ivInt128, ivFloat, ivDouble,
            ivBool, ivDBKey, ivByte, ivDec34, ivDec16} IITYPE;

//
//  Those are the Interbase C API prototypes that we use
//  Taken 'asis' from IBASE.H, prefix 'isc_' replaced with 'proto_',
//  and 'typedef' preprended...
//

typedef ISC_STATUS ISC_EXPORT proto_create_database (ISC_STATUS *,
                        short,
                        char *,
                        isc_db_handle *,
                        short,
                        char *,
                        short);

typedef ISC_STATUS ISC_EXPORT proto_attach_database (ISC_STATUS *,
                        short,
                        char *,
                        isc_db_handle *,
                        short,
                        char *);

typedef ISC_STATUS  ISC_EXPORT proto_detach_database (ISC_STATUS *,
                        isc_db_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_drop_database (ISC_STATUS *,
                      isc_db_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_database_info (ISC_STATUS *,
                      isc_db_handle *,
                      short,
                      char *,
                      short,
                      char *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_execute_immediate (ISC_STATUS *,
                           isc_db_handle *,
                           isc_tr_handle *,
                           unsigned short,
                           char *,
                           unsigned short,
                           XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_open_blob2 (ISC_STATUS *,
                       isc_db_handle *,
                       isc_tr_handle *,
                       isc_blob_handle *,
                       ISC_QUAD *,
                       short,
                       char *);

typedef ISC_STATUS  ISC_EXPORT proto_create_blob2 (ISC_STATUS *,
                    isc_db_handle *,
                    isc_tr_handle *,
                    isc_blob_handle *,
                    ISC_QUAD *,
                    short,
                    char *);

typedef ISC_STATUS  ISC_EXPORT proto_close_blob (ISC_STATUS *,
                       isc_blob_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_cancel_blob (ISC_STATUS *,
                        isc_blob_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_get_segment (ISC_STATUS *,
                        isc_blob_handle *,
                        unsigned short *,
                        unsigned short,
                        char *);

typedef ISC_STATUS  ISC_EXPORT proto_put_segment (ISC_STATUS *,
                    isc_blob_handle *,
                    unsigned short,
                    char *);

typedef ISC_STATUS  ISC_EXPORT proto_blob_info (ISC_STATUS *,
                      isc_blob_handle *,
                      short,
                      char *,
                      short,
                      char *);

typedef ISC_STATUS  ISC_EXPORT proto_array_lookup_bounds (ISC_STATUS *,
                        isc_db_handle *,
                        isc_tr_handle *,
                        char *,
                        char *,
                        ISC_ARRAY_DESC *);

typedef ISC_STATUS  ISC_EXPORT proto_array_get_slice (ISC_STATUS *,
                        isc_db_handle *,
                        isc_tr_handle *,
                        ISC_QUAD *,
                        ISC_ARRAY_DESC *,
                        void *,
                        ISC_LONG *);

typedef ISC_STATUS  ISC_EXPORT proto_array_put_slice (ISC_STATUS *,
                        isc_db_handle *,
                        isc_tr_handle *,
                        ISC_QUAD *,
                        ISC_ARRAY_DESC *,
                        void *,
                        ISC_LONG *);

typedef ISC_LONG    ISC_EXPORT proto_vax_integer (char *,
                    short);

typedef ISC_LONG    ISC_EXPORT proto_sqlcode (ISC_STATUS *);

typedef void        ISC_EXPORT proto_sql_interprete (short,
                       char *,
                       short);

#if defined(FB_API_VER) && FB_API_VER >= 20
typedef ISC_STATUS  ISC_EXPORT proto_interpret (char *,
                                      unsigned int,
                                      const ISC_STATUS * *);
#else
typedef ISC_STATUS  ISC_EXPORT proto_interprete (char *,
                       ISC_STATUS * *);

#endif

typedef ISC_STATUS  ISC_EXPORT proto_que_events (ISC_STATUS *,
                       isc_db_handle *,
                       ISC_LONG *,
                       short,
                       char *,
                       isc_callback,
                       void *);

typedef ISC_STATUS  ISC_EXPORT proto_cancel_events (ISC_STATUS *,
                      isc_db_handle *,
                      ISC_LONG *);

typedef ISC_STATUS  ISC_EXPORT proto_start_multiple (ISC_STATUS *,
                       isc_tr_handle *,
                       short,
                       void *);

typedef ISC_STATUS  ISC_EXPORT proto_commit_transaction (ISC_STATUS *,
                           isc_tr_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_commit_retaining (ISC_STATUS *,
                         isc_tr_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_rollback_transaction (ISC_STATUS *,
                         isc_tr_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_rollback_retaining (ISC_STATUS *,
                         isc_tr_handle *);


typedef ISC_STATUS  ISC_EXPORT proto_dsql_allocate_statement (ISC_STATUS *,
                            isc_db_handle *,
                            isc_stmt_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_describe (ISC_STATUS *,
                      isc_stmt_handle *,
                      unsigned short,
                      XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_describe_bind (ISC_STATUS *,
                           isc_stmt_handle *,
                           unsigned short,
                           XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_execute (ISC_STATUS *,
                     isc_tr_handle *,
                     isc_stmt_handle *,
                     unsigned short,
                     XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_execute2 (ISC_STATUS *,
                      isc_tr_handle *,
                      isc_stmt_handle *,
                      unsigned short,
                      XSQLDA *,
                      XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_fetch (ISC_STATUS *,
                       isc_stmt_handle *,
                       unsigned short,
                       XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_free_statement (ISC_STATUS *,
                        isc_stmt_handle *,
                        unsigned short);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_prepare (ISC_STATUS *,
                     isc_tr_handle *,
                     isc_stmt_handle *,
                     unsigned short,
                     char *,
                     unsigned short,
                     XSQLDA *);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_set_cursor_name (ISC_STATUS *,
                         isc_stmt_handle *,
                         char *,
                         unsigned short);

typedef ISC_STATUS  ISC_EXPORT proto_dsql_sql_info (ISC_STATUS *,
                      isc_stmt_handle *,
                      short,
                      char *,
                      short,
                      char *);

typedef void        ISC_EXPORT proto_decode_date (ISC_QUAD *,
                    void *);

typedef void        ISC_EXPORT proto_encode_date (void *,
                    ISC_QUAD *);

typedef int         ISC_EXPORT proto_add_user (ISC_STATUS *, USER_SEC_DATA *);
typedef int         ISC_EXPORT proto_delete_user (ISC_STATUS *, USER_SEC_DATA *);
typedef int         ISC_EXPORT proto_modify_user (ISC_STATUS *, USER_SEC_DATA *);


typedef ISC_STATUS  ISC_EXPORT proto_service_attach (ISC_STATUS *,
                       unsigned short,
                       char *,
                       isc_svc_handle *,
                       unsigned short,
                       char *);

typedef ISC_STATUS  ISC_EXPORT proto_service_detach (ISC_STATUS *,
                       isc_svc_handle *);

typedef ISC_STATUS  ISC_EXPORT proto_service_query (ISC_STATUS *,
                      isc_svc_handle *,
                                      isc_resv_handle *,
                      unsigned short,
                      char *,
                      unsigned short,
                      char *,
                      unsigned short,
                      char *);

typedef ISC_STATUS ISC_EXPORT proto_service_start (ISC_STATUS *,
                         isc_svc_handle *,
                                 isc_resv_handle *,
                         unsigned short,
                         char*);

typedef void        ISC_EXPORT proto_decode_sql_date (ISC_DATE *,
                    void *);

typedef void        ISC_EXPORT proto_decode_sql_time (ISC_TIME *,
                    void *);

typedef void        ISC_EXPORT proto_decode_timestamp (ISC_TIMESTAMP *,
                    void *);

typedef void        ISC_EXPORT proto_encode_sql_date (void *,
                    ISC_DATE *);

typedef void        ISC_EXPORT proto_encode_sql_time (void *,
                    ISC_TIME *);

typedef void        ISC_EXPORT proto_encode_timestamp (void *,
                    ISC_TIMESTAMP *);

//
//  Internal binding structure to the FBCLIENT DLL
//

class IFactories;

#ifdef IBPP_WINDOWS
typedef HMODULE ibpp_HMODULE;
#endif
#ifdef IBPP_UNIX
typedef void* ibpp_HMODULE;
#endif

struct FBCLIENT
{
    // Attributes
    bool mReady;
    std::string mfbdll;

    // The FBCLIENT.DLL HMODULE
    ibpp_HMODULE mHandle;

#ifdef IBPP_WINDOWS
    std::string mSearchPaths;   // Optional additional search paths
#endif

    #ifdef FIXME
    nach FactoriesImplFb1::Init verschieben
    #endif

    IFactories* mFactories;

    FBCLIENT* Call();

    // FBCLIENT Entry Points
    proto_create_database*          m_create_database;
    proto_attach_database*          m_attach_database;
    proto_detach_database*          m_detach_database;
    proto_drop_database*            m_drop_database;
    proto_database_info*            m_database_info;
    proto_dsql_execute_immediate*   m_dsql_execute_immediate;
    proto_open_blob2*               m_open_blob2;
    proto_create_blob2*             m_create_blob2;
    proto_close_blob*               m_close_blob;
    proto_cancel_blob*              m_cancel_blob;
    proto_get_segment*              m_get_segment;
    proto_put_segment*              m_put_segment;
    proto_blob_info*                m_blob_info;
    proto_array_lookup_bounds*      m_array_lookup_bounds;
    proto_array_get_slice*          m_array_get_slice;
    proto_array_put_slice*          m_array_put_slice;

    proto_vax_integer*              m_vax_integer;
    proto_sqlcode*                  m_sqlcode;
    proto_sql_interprete*           m_sql_interprete;
    #if defined(FB_API_VER) && FB_API_VER >= 20
    proto_interpret*                m_interpret;
    #else
    proto_interprete*               m_interprete;
    #endif

    proto_que_events*               m_que_events;
    proto_cancel_events*            m_cancel_events;
    proto_start_multiple*           m_start_multiple;
    proto_commit_transaction*       m_commit_transaction;
    proto_commit_retaining*         m_commit_retaining;
    proto_rollback_transaction*     m_rollback_transaction;
    proto_rollback_retaining*       m_rollback_retaining;
    proto_dsql_allocate_statement*  m_dsql_allocate_statement;
    proto_dsql_describe*            m_dsql_describe;
    proto_dsql_describe_bind*       m_dsql_describe_bind;
    proto_dsql_prepare*             m_dsql_prepare;
    proto_dsql_execute*             m_dsql_execute;
    proto_dsql_execute2*            m_dsql_execute2;
    proto_dsql_fetch*               m_dsql_fetch;
    proto_dsql_free_statement*      m_dsql_free_statement;
    proto_dsql_set_cursor_name*     m_dsql_set_cursor_name;
    proto_dsql_sql_info*            m_dsql_sql_info;
    //proto_decode_date*                m_decode_date;
    //proto_encode_date*                m_encode_date;
    //proto_add_user*                   m_add_user;
    //proto_delete_user*                m_delete_user;
    //proto_modify_user*                m_modify_user;

    proto_service_attach*           m_service_attach;
    proto_service_detach*           m_service_detach;
    proto_service_start*            m_service_start;
    proto_service_query*            m_service_query;
    //proto_decode_sql_date*            m_decode_sql_date;
    //proto_decode_sql_time*            m_decode_sql_time;
    //proto_decode_timestamp*           m_decode_timestamp;
    //proto_encode_sql_date*            m_encode_sql_date;
    //proto_encode_sql_time*            m_encode_sql_time;
    //proto_encode_timestamp*           m_encode_timestamp;

    // Constructor (No need for a specific destructor)
    FBCLIENT()
    {
        mReady = false;
#ifdef IBPP_WINDOWS
        mHandle = 0;
#endif
        mFactories = nullptr;
    }
};

extern FBCLIENT gds;

//
//  Service Parameter Block (used to define a service)
//

class SPB
{
    static const int BUFFERINCR;

    char* mBuffer;              // Dynamically allocated SPB structure
    int mSize;                  // Its used size in bytes
    int mAlloc;                 // Its allocated size in bytes

    void Grow(int needed);      // Alloc or grow the mBuffer

public:
    void Insert(char);          // Insert a single byte code
    void InsertString(char, int, const char*);  // Insert a string, len can be defined as 1 or 2 bytes
    void InsertByte(char type, char data);
    void InsertQuad(char type, int32_t data);
    void Reset();           // Clears the SPB
    char* Self() { return mBuffer; }
    short Size() { return (short)mSize; }

    SPB() : mBuffer(0), mSize(0), mAlloc(0) { }
    ~SPB() { Reset(); }
};

//
//  Transaction Parameter Block (used to define a transaction)
//

class TPB
{
    static const int BUFFERINCR;

    char* mBuffer;                  // Dynamically allocated TPB structure
    int mSize;                      // Its used size in bytes
    int mAlloc;                     // Its allocated size

    void Grow(int needed);          // Alloc or re-alloc the mBuffer

public:
    void Insert(char);              // Insert a flag item
    void Insert(const std::string& data); // Insert a string (typically table name)
    void Reset();               // Clears the TPB
    char* Self() { return mBuffer; }
    int Size() { return mSize; }

    TPB() : mBuffer(0), mSize(0), mAlloc(0) { }
    ~TPB() { Reset(); }
};

//
//  Used to receive (and process) a results buffer in various API calls
//

class RB
{
    char* mBuffer;
    int mSize;

    char* FindToken(char token);
    char* FindToken(char token, char subtoken);

public:
    void Reset();
    int GetValue(char token);
    int GetCountValue(char token);
    void GetDetailedCounts(IBPP::DatabaseCounts& counts, char token);
    int GetValue(char token, char subtoken);
    bool GetBool(char token);
    int GetString(char token, std::string& data);

    char* Self() { return mBuffer; }
    short Size() { return (short)mSize; }

    unsigned char* GetBuffer() { return (unsigned char*)mBuffer; }
    unsigned GetBufferLength() { return (unsigned)mSize; }

    RB();
    RB(int Size);
    ~RB();
};

//
//  Used to receive status info from API calls
//

class IBS
{
    mutable ISC_STATUS mVector[20];
    mutable std::string mMessage;

public:
    ISC_STATUS* Self() { return mVector; }
    bool Errors() { return (mVector[0] == 1 && mVector[1] > 0) ? true : false; }
    const char* ErrorMessage() const;
    int SqlCode() const;
    int EngineCode() const { return (mVector[0] == 1) ? (int)mVector[1] : 0; }
    void Reset();

    IBS();
    IBS(IBS&);  // Copy Constructor
    ~IBS();
};

///////////////////////////////////////////////////////////////////////////////
//
//  Implementation of the "hidden" classes associated with their public
//  counterparts. Their private data and methods can freely change without
//  breaking the compatibility of the DLL. If they receive new public methods,
//  and those methods are reflected in the public class, then the compatibility
//  is broken.
//
///////////////////////////////////////////////////////////////////////////////

//
// Hidden implementation of Exception classes.
//

/*
                         std::exception
                                |
                         IBPP::Exception
                       /                 \
                      /                   \
  IBPP::LogicException    ExceptionBase    IBPP::SQLException
        |        \         /   |     \     /
        |   LogicExceptionImpl |   SQLExceptionImpl
        |                      |
    IBPP::WrongType            |
               \               |
              IBPP::WrongTypeImpl
*/

class ExceptionBase
{
    //  (((((((( OBJECT INTERNALS ))))))))

protected:
    std::string mContext;           // Exception context ("IDatabase::Drop")
    std::string mWhat;              // Full formatted message

    void buildErrorMessage(const char* message);
    void raise(const std::string& context, const char* message, va_list argptr);

public:

    ExceptionBase() throw();
    ExceptionBase(const ExceptionBase& copied) throw();
    ExceptionBase& operator=(const ExceptionBase& copied) throw();
    ExceptionBase(const std::string& context, const char* message = 0, ...) throw();

    virtual ~ExceptionBase() throw();

    //  (((((((( OBJECT INTERFACE ))))))))

    virtual const char* Origin() const throw();
    virtual const char* what() const throw();
};

class LogicExceptionImpl : public IBPP::LogicException, public ExceptionBase
{
    //  (((((((( OBJECT INTERNALS ))))))))

public:

    LogicExceptionImpl() throw();
    LogicExceptionImpl(const LogicExceptionImpl& copied) throw();
    LogicExceptionImpl& operator=(const LogicExceptionImpl& copied) throw();
    LogicExceptionImpl(const std::string& context, const char* message = 0, ...) throw();

    virtual ~LogicExceptionImpl() throw ();

    //  (((((((( OBJECT INTERFACE ))))))))
    //
    //  The object public interface is partly implemented by inheriting from
    //  the ExceptionBase class.

public:
    virtual const char* Origin() const throw();
    virtual const char* what() const throw();
};

class SQLExceptionImpl : public IBPP::SQLException, public ExceptionBase
{
    //  (((((((( OBJECT INTERNALS ))))))))

private:
    int mSqlCode;
    int mEngineCode;

public:
    #ifdef FIXME
    SQL-Exception evtl. aufteilen in FB1/FB3
    #endif

    SQLExceptionImpl() throw();
    SQLExceptionImpl(const SQLExceptionImpl& copied) throw();
    SQLExceptionImpl& operator=(const SQLExceptionImpl& copied) throw();
    SQLExceptionImpl(const IBS& status, const std::string& context,
                        const char* message = 0, ...) throw();
    SQLExceptionImpl(Firebird::IStatus* mStatus, const std::string& context,
                        const char* message = 0, ...) throw();

    virtual ~SQLExceptionImpl() throw ();

    //  (((((((( OBJECT INTERFACE ))))))))
    //
    //  The object public interface is partly implemented by inheriting from
    //  the ExceptionBase class.

public:
    virtual const char* Origin() const throw();
    virtual const char* what() const throw();
    virtual int SqlCode() const throw();
    virtual int EngineCode() const throw();
};

class WrongTypeImpl : public IBPP::WrongType, public ExceptionBase
{
    //  (((((((( OBJECT INTERNALS ))))))))

public:

    WrongTypeImpl() throw();
    WrongTypeImpl(const WrongTypeImpl& copied) throw();
    WrongTypeImpl& operator=(const WrongTypeImpl& copied) throw();
    WrongTypeImpl(const std::string& context, int sqlType, IITYPE varType,
                    const char* message = 0, ...) throw();

    virtual ~WrongTypeImpl() throw ();

    //  (((((((( OBJECT INTERFACE ))))))))
    //
    //  The object public interface is partly implemented by inheriting from
    //  the ExceptionBase class.

public:
    virtual const char* Origin() const throw();
    virtual const char* what() const throw();
};

void encodeDate(ISC_DATE& isc_dt, const IBPP::Date& dt);
void decodeDate(IBPP::Date& dt, const ISC_DATE& isc_dt);

void encodeTime(ISC_TIME& isc_tm, const IBPP::Time& tm);
void decodeTime(IBPP::Time& tm, const ISC_TIME& isc_tm);

void decodeTimeTz(IBPP::Time& tm, const ISC_TIME_TZ& isc_tm);

void encodeTimestamp(ISC_TIMESTAMP& isc_ts, const IBPP::Timestamp& ts);
void decodeTimestamp(IBPP::Timestamp& ts, const ISC_TIMESTAMP& isc_ts);

void decodeTimestampTz(IBPP::Timestamp& ts, const ISC_TIMESTAMP_TZ& isc_ts);

struct consts   // See _ibpp.cpp for initializations of these constants
{
    static const double dscales[19];
    static const int Dec31_1899;
    static const int16_t min16;
    static const int16_t max16;
    static const int32_t min32;
    static const int32_t max32;
};

/* FB3+: get the IMaster-interface.
 * This is needed to convert times with time zone correctly (FB4+).
 * Implemented in fbinterfaces.cpp */
const int FB_MAX_TIME_ZONE_NAME_LENGTH = 32;
class fbIntfClass
{
    private:
        static fbIntfClass gmFbIntf;
        bool init(std::string& errmsg);
    public:
        Firebird::IMaster* mMaster;
        Firebird::ThrowStatusWrapper* mStatus;
        Firebird::IUtil* mUtil;

        fbIntfClass();
        ~fbIntfClass();

        static fbIntfClass* getInstance();
};

class IFactories
{
public:
    virtual IBPP::Service CreateService(const std::string& ServerName,
        const std::string& UserName, const std::string& UserPassword,
        const std::string& RoleName, const std::string& CharSet,
        const std::string& FBClient = "") = 0;
    virtual IBPP::Database CreateDatabase(const std::string& ServerName,
        const std::string& DatabaseName, const std::string& UserName,
        const std::string& UserPassword, const std::string& RoleName,
        const std::string& CharSet, const std::string& CreateParams,
        const std::string& FBClient = "") = 0;
    virtual IBPP::Transaction CreateTransaction(IBPP::Database db, IBPP::TAM am = IBPP::amWrite,
        IBPP::TIL il = IBPP::ilConcurrency, IBPP::TLR lr = IBPP::lrWait, IBPP::TFF flags = IBPP::TFF(0)) = 0;
    virtual IBPP::Statement CreateStatement(IBPP::Database db, IBPP::Transaction tr) = 0;
    virtual IBPP::Blob CreateBlob(IBPP::Database db, IBPP::Transaction tr) = 0;
    virtual IBPP::Array CreateArray(IBPP::Database db, IBPP::Transaction tr) = 0;
    virtual IBPP::Events CreateEvents(IBPP::Database db) = 0;
};

}   // namespace ibpp_internal

#endif // __INTERNAL_IBPP_H__
