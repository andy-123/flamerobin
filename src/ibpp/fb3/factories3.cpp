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

#ifdef IBPP_UNIX
#include <dlfcn.h>
#endif

using namespace ibpp_internals;
using namespace IBPP;

Service FactoriesImplFb3::CreateService(const std::string& ServerName,
    const std::string& UserName, const std::string& UserPassword,
    const std::string& RoleName, const std::string& CharSet,
    const std::string& FBClient)
{
    return new ServiceImplFb3(ServerName, UserName,
        UserPassword, RoleName, CharSet);
}

Database FactoriesImplFb3::CreateDatabase(const std::string& ServerName,
    const std::string& DatabaseName, const std::string& UserName,
    const std::string& UserPassword, const std::string& RoleName,
    const std::string& CharSet, const std::string& CreateParams,
    const std::string& FBClient)
{
    return new DatabaseImplFb3(ServerName, DatabaseName, UserName,
        UserPassword, RoleName, CharSet, CreateParams);
}

Transaction FactoriesImplFb3::CreateTransaction(Database db, TAM am,
    TIL il, TLR lr, TFF flags)
{
    return new TransactionImplFb3(
        dynamic_cast<DatabaseImplFb3*>(db.intf()),
        am, il, lr, flags);
}

Statement FactoriesImplFb3::CreateStatement(Database db, Transaction tr)
{
    return new StatementImplFb3(
        dynamic_cast<DatabaseImplFb3*>(db.intf()),
        dynamic_cast<TransactionImplFb3*>(tr.intf()));
}

Blob FactoriesImplFb3::CreateBlob(Database db, Transaction tr)
{
     return new BlobImplFb3(
        dynamic_cast<DatabaseImplFb3*>(db.intf()),
        dynamic_cast<TransactionImplFb3*>(tr.intf()));
}

Array FactoriesImplFb3::CreateArray(Database db, Transaction tr)
{
    return new ArrayImplFb3(
        dynamic_cast<DatabaseImplFb3*>(db.intf()),
        dynamic_cast<TransactionImplFb3*>(tr.intf()));

}

Events FactoriesImplFb3::CreateEvents(Database db)
{
    return new EventsImplFb3(
        dynamic_cast<DatabaseImplFb3*>(db.intf()));
}

// ***** STATIC *****

bool FactoriesImplFb3::gIsInit = false;
bool FactoriesImplFb3::gAvailable = false;
IMaster* FactoriesImplFb3::gMaster = nullptr;
IUtil* FactoriesImplFb3::gUtil = nullptr;
proto_get_database_handle* FactoriesImplFb3::m_get_database_handle = nullptr;
proto_get_transaction_handle* FactoriesImplFb3::m_get_transaction_handle = nullptr;
// inernal IBPP-Utils (fb3+)
UtlFb3* FactoriesImplFb3::gUtlFb3 = nullptr;

bool FactoriesImplFb3::gInit(ibpp_HMODULE h)
{
    if (gIsInit)
        return gAvailable;

    // FB3 +
    proto_get_master_interface* m_get_master_interface;
    FB_LOADDYN_NOTHROW(h, get_master_interface);

    if (m_get_master_interface != nullptr)
    {
        gAvailable = true;
        gMaster = m_get_master_interface();
        gUtil = gMaster->getUtilInterface();
        FB_LOADDYN(h, get_database_handle);
        FB_LOADDYN(h, get_transaction_handle);
    }

    gIsInit = true;
    return gAvailable;
}
