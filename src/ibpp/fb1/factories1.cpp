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

using namespace ibpp_internals;
using namespace IBPP;

Service FactoriesImplFb1::CreateService(const std::string& ServerName,
    const std::string& UserName, const std::string& UserPassword,
    const std::string& RoleName, const std::string& CharSet,
    const std::string& FBClient)
{
    return new ServiceImplFb1(ServerName, UserName,
        UserPassword, RoleName, CharSet);
}

Database FactoriesImplFb1::CreateDatabase(const std::string& ServerName,
    const std::string& DatabaseName, const std::string& UserName,
    const std::string& UserPassword, const std::string& RoleName,
    const std::string& CharSet, const std::string& CreateParams,
    const std::string& FBClient)
{
    return new DatabaseImplFb1(ServerName, DatabaseName, UserName,
        UserPassword, RoleName, CharSet, CreateParams);
}

Transaction FactoriesImplFb1::CreateTransaction(Database db, TAM am,
    TIL il, TLR lr, TFF flags)
{
    return new TransactionImplFb1(
        dynamic_cast<DatabaseImplFb1*>(db.intf()),
        am, il, lr, flags);
}

Statement FactoriesImplFb1::CreateStatement(Database db, Transaction tr)
{
    return new StatementImplFb1(
        dynamic_cast<DatabaseImplFb1*>(db.intf()),
        dynamic_cast<TransactionImplFb1*>(tr.intf()));
}

Blob FactoriesImplFb1::CreateBlob(Database db, Transaction tr)
{
     return new BlobImplFb1(
        dynamic_cast<DatabaseImplFb1*>(db.intf()),
        dynamic_cast<TransactionImplFb1*>(tr.intf()));
}

Array FactoriesImplFb1::CreateArray(Database db, Transaction tr)
{
    return new ArrayImplFb1(
        dynamic_cast<DatabaseImplFb1*>(db.intf()),
        dynamic_cast<TransactionImplFb1*>(tr.intf()));

}

Events FactoriesImplFb1::CreateEvents(Database db)
{
    return new EventsImplFb1(
        dynamic_cast<DatabaseImplFb1*>(db.intf()));
}
