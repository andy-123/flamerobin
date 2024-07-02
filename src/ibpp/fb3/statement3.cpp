//	Subject : IBPP, Service class implementation
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

#include <cctype>
#include <functional>
#include <array>

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <iostream>
#include <algorithm>

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void StatementImplFb3::CheckIsInit(const char* callername)
{
    if (mStm == nullptr)
        throw LogicExceptionImpl(callername, _("No statement has been prepared."));
}

void StatementImplFb3::Prepare(const std::string& sql)
{
    if (mDatabase == 0)
        throw LogicExceptionImpl("Statement::Prepare", _("An IDatabase must be attached."));
    if (mDatabase->GetFbIntf() == 0)
        throw LogicExceptionImpl("Statement::Prepare", _("IDatabase must be connected."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Statement::Prepare", _("An ITransaction must be attached."));
    if (mTransaction->GetFbIntf() == 0)
        throw LogicExceptionImpl("Statement::Prepare", _("ITransaction must be started."));
    if (sql.empty())
        throw LogicExceptionImpl("Statement::Prepare", _("SQL statement can't be 0."));

    // Saves the SQL sentence, only for reporting reasons in case of errors
    mSql = ParametersParser(sql);

    // Free all resources currently attached to this Statement, then allocate
    // a new statement descriptor.
    Close();

    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    mStm = atm->prepare(mStatus, tra, 0, mSql.c_str(),
                        mDatabase->Dialect(),
                        Firebird::IStatement::PREPARE_PREFETCH_METADATA);
    if (mStatus->isDirty())
    {
        Close();
        std::string context = "Statement::Prepare( ";
        context.append(mSql).append(" )");
        throw SQLExceptionImpl(mStatus, context.c_str(),
                               _("isc_dsql_prepare failed"));
    }

    // Read what kind of statement was prepared
    unsigned stmType = mStm->getType(mStatus);
    if (mStatus->isDirty())
    {
        Close();
        throw SQLExceptionImpl(mStatus, "Statement::Prepare",
                               _("isc_dsql_sql_info failed"));
    }
    switch (stmType)
    {
        case isc_info_sql_stmt_select : mType = IBPP::stSelect; break;
        case isc_info_sql_stmt_insert : mType = IBPP::stInsert; break;
        case isc_info_sql_stmt_update : mType = IBPP::stUpdate; break;
        case isc_info_sql_stmt_delete : mType = IBPP::stDelete; break;
        case isc_info_sql_stmt_ddl :    mType = IBPP::stDDL; break;
        case isc_info_sql_stmt_exec_procedure : mType = IBPP::stExecProcedure; break;
        case isc_info_sql_stmt_start_trans: mType = IBPP::stStartTransaction; break;
        case isc_info_sql_stmt_commit:  mType = IBPP::stCommitTransaction; break;
        case isc_info_sql_stmt_rollback:    mType = IBPP::stRollbackTransaction; break;
        case isc_info_sql_stmt_select_for_upd : mType = IBPP::stSelectUpdate; break;
        case isc_info_sql_stmt_set_generator : mType = IBPP::stSetGenerator; break;
        case isc_info_sql_stmt_savepoint : mType = IBPP::stSavePoint; break;
        default : mType = IBPP::stUnsupported;
    }
    if (mType == IBPP::stUnknown || mType == IBPP::stUnsupported)
    {
        Close();
        throw LogicExceptionImpl("Statement::Prepare",
                                 _("Unknown or unsupported statement type"));
    }

    Firebird::IMessageMetadata* metaOut = nullptr;
    metaOut = mStm->getOutputMetadata(mStatus);
    if (mStatus->isDirty())
    {
        Close();
        throw SQLExceptionImpl(mStatus, "Statement::Prepare",
                               _("IStatement::getInputMetadata failed"));
    }
    mOutRow = new RowImplFb3(mDatabase->Dialect(), mDatabase, mTransaction);
    mOutRow->SetFbIntfMeta(metaOut);

    if (mOutRow->Columns() == 0)
    {
        // Get rid of the output descriptor, if it wasn't required (no output)
        mOutRow->Release();
        mOutRow = nullptr;
    }

    Firebird::IMessageMetadata* metaIn = nullptr;
    metaIn = mStm->getInputMetadata(mStatus);
    if (mStatus->isDirty())
    {
        Close();
        throw SQLExceptionImpl(mStatus, "Statement::Prepare",
                               _("IStatement::getOutputMetadata failed"));
    }

    mInRow = new RowImplFb3(mDatabase->Dialect(), mDatabase, mTransaction);
    mInRow->SetFbIntfMeta(metaIn);
    if (mInRow->Columns() == 0)
    {
        mInRow->Release();
        mInRow = nullptr;
    }
}

void StatementImplFb3::Plan(std::string& plan)
{
    if (mStm == nullptr)
        throw LogicExceptionImpl("Statement::Plan", _("No statement has been prepared."));
    if (mDatabase == 0)
        throw LogicExceptionImpl("Statement::Plan", _("A Database must be attached."));
    if (mDatabase->GetFbIntf() == 0)
        throw LogicExceptionImpl("Statement::Plan", _("Database must be connected."));

    const char* cplan = mStm->getPlan(mStatus, FB_FALSE);
    if (cplan != nullptr)
    {
        plan = cplan;
        if (plan[0] == '\n')
            plan.erase(0, 1);
    }
    else
        plan.erase();
}

void StatementImplFb3::Execute(const std::string& sql)
{
    if (!sql.empty())
        Prepare(sql);

    if (mStm == nullptr)
        throw LogicExceptionImpl("Statement::Execute",
                                 _("No statement has been prepared."));

    // Check that a value has been set for each input parameter
    if ((mInRow != nullptr) && (mInRow->MissingValues()))
        throw LogicExceptionImpl("Statement::Execute",
                                 _("All parameters must be specified."));

    // Free a previous 'cursor' if any
    CursorFree();

    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    char* metaInBuf = nullptr;
    char* metaOutBuf = nullptr;
    Firebird::IMessageMetadata* metaIn = nullptr;
    Firebird::IMessageMetadata* metaOut = nullptr;
    if (mInRow != nullptr)
    {
        metaIn = mInRow->GetFbIntfMeta();
        metaInBuf = mInRow->GetBuffer();
    }
    if (mOutRow != nullptr)
    {
        metaOut = mOutRow->GetFbIntfMeta();
        metaOutBuf = mOutRow->GetBuffer();
    }

    if (mType == IBPP::stSelect)
    {
        // Could return a result set (none, single or multi rows)
        mRes = mStm->openCursor(mStatus, tra, metaIn, metaInBuf,
                                metaOut, 0);
        if (mStatus->isDirty())
        {
            //Close();	Commented because Execute error should not free the statement
            std::string context = "Statement::Execute( ";
            context.append(mSql).append(" )");
            throw SQLExceptionImpl(mStatus, context.c_str(),
                                   _("isc_dsql_execute failed"));
        }
    }
    else
    {
        // Can return at a single or mutiple rows
        mStm->execute(mStatus, tra, metaIn, metaInBuf,
                      metaOut, metaOutBuf);
        if (mStatus->isDirty())
        {
            //Close();	Commented because Execute error should not free the statement
            std::string context = "Statement::Execute( ";
            context.append(mSql).append(" )");
            throw SQLExceptionImpl(mStatus, context.c_str(),
                _("isc_dsql_execute2 failed"));
        }
    }
}

void StatementImplFb3::CursorExecute(const std::string& cursor, const std::string& sql)
{
    if (cursor.empty())
        throw LogicExceptionImpl("Statement::CursorExecute", _("Cursor name can't be 0."));

    if (!sql.empty())
        Prepare(sql);

    if (mStm == nullptr)
        throw LogicExceptionImpl("Statement::CursorExecute", _("No statement has been prepared."));
    if (mType != IBPP::stSelectUpdate)
        throw LogicExceptionImpl("Statement::CursorExecute", _("Statement must be a SELECT FOR UPDATE."));
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::CursorExecute", _("Statement would return no rows."));

    // Check that a value has been set for each input parameter
    if (mInRow != 0 && mInRow->MissingValues())
        throw LogicExceptionImpl("Statement::CursorExecute",
            _("All parameters must be specified."));

    CursorFree(); // Free a previous 'cursor' if any

    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    char* metaInBuf = nullptr;
    Firebird::IMessageMetadata* metaIn = nullptr;
    Firebird::IMessageMetadata* metaOut = nullptr;
    if (mInRow != nullptr)
    {
        metaIn = mInRow->GetFbIntfMeta();
        metaInBuf = mInRow->GetBuffer();
    }
    if (mOutRow != nullptr)
        metaOut = mOutRow->GetFbIntfMeta();
    mRes = mStm->openCursor(mStatus, tra, metaIn, metaInBuf,
                            metaOut, 0);
    if (mStatus->isDirty())
    {
        //Close();	Commented because Execute error should not free the statement
        std::string context = "Statement::CursorExecute( ";
        context.append(mSql).append(" )");
        throw SQLExceptionImpl(mStatus, context.c_str(),
            _("isc_dsql_execute failed"));
    }

    mStm->setCursorName(mStatus, const_cast<char*>(cursor.c_str()));
    if (mStatus->isDirty())
    {
        //Close();	Commented because Execute error should not free the statement
        throw SQLExceptionImpl(mStatus, "Statement::CursorExecute",
            _("isc_dsql_set_cursor_name failed"));
    }
}

void StatementImplFb3::ExecuteImmediate(const std::string& sql)
{
    if (mDatabase == 0)
        throw LogicExceptionImpl("Statement::ExecuteImmediate", _("An IDatabase must be attached."));
    if (mDatabase->GetFbIntf() == 0)
        throw LogicExceptionImpl("Statement::ExecuteImmediate", _("IDatabase must be connected."));
    if (mTransaction == 0)
        throw LogicExceptionImpl("Statement::ExecuteImmediate", _("An ITransaction must be attached."));
    if (mTransaction->GetFbIntf() == 0)
        throw LogicExceptionImpl("Statement::ExecuteImmediate", _("ITransaction must be started."));
    if (sql.empty())
        throw LogicExceptionImpl("Statement::ExecuteImmediate", _("SQL statement can't be 0."));

    Firebird::IAttachment* atm = mDatabase->GetFbIntf();
    Firebird::ITransaction* tra = mTransaction->GetFbIntf();
    Close();
    atm->execute(mStatus, tra, 0, sql.c_str(), mDatabase->Dialect(),
        nullptr, nullptr, nullptr, nullptr);
    if (mStatus->isDirty())
    {
        std::string context = "Statement::ExecuteImmediate( ";
        context.append(sql).append(" )");
        throw SQLExceptionImpl(mStatus, context.c_str(),
            _("isc_dsql_execute_immediate failed"));
    }
}

int StatementImplFb3::AffectedRows()
{
    if (mStm == nullptr)
        throw LogicExceptionImpl("Statement::AffectedRows", _("No statement has been prepared."));

    ISC_UINT64 count = mStm->getAffectedRecords(mStatus);
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Statement::AffectedRows",
                               _("IStatement::getAffectedRecords failed."));
    return (int)count;
}

bool StatementImplFb3::Fetch()
{
    if (mOutRow == nullptr)
        throw LogicExceptionImpl("Statement::Fetch",
                                 _("No statement has been executed or no result set available."));

    ISC_STATUS fetchStatus = mRes->fetchNext(mStatus, mOutRow->GetBuffer());
    if (mStatus->isDirty())
    {
        Close();
        throw SQLExceptionImpl(mStatus, "Statement::Fetch",
                               _("isc_dsql_fetch failed."));
    }
    if (fetchStatus == Firebird::IStatus::RESULT_NO_DATA)
    {
        CursorFree();   
        return false;
    }

    return true;
}

bool StatementImplFb3::Fetch(IBPP::Row& row)
{
    if (mOutRow == nullptr)
        throw LogicExceptionImpl("Statement::Fetch(row)",
            _("No statement has been executed or no result set available."));

    RowImplFb3* rowimpl = new RowImplFb3(*mOutRow);
    row = rowimpl;

    ISC_STATUS fetchStatus = mRes->fetchNext(mStatus, rowimpl->GetBuffer());
    if (fetchStatus == Firebird::IStatus::RESULT_NO_DATA)
    {
        CursorFree();
        row.clear();
        return false;
    }
    if (mStatus->isDirty())
    {
        //Close();
        throw SQLExceptionImpl(mStatus, "Statement::Fetch",
                               _("isc_dsql_fetch failed."));
    }
    if (mStatus->isDirty())
    {
        //Close();
        row.clear();
        throw SQLExceptionImpl(mStatus, "Statement::Fetch(row)",
            _("isc_dsql_fetch failed."));
    }

    return true;
}

void StatementImplFb3::Close()
{
    // Free all statement resources.
    // Used before preparing a new statement or from destructor.
    CursorFree();

    if (mInRow != 0)
    {
        mInRow->Release();
        mInRow = 0;
    }
    if (mOutRow != 0)
    {
        mOutRow->Release();
        mOutRow = 0;
    }

    mType = IBPP::stUnknown;

    if (mStm != nullptr)
    {
        mStm->free(mStatus);
        mStm = nullptr;
        if (mStatus->isDirty())
            throw SQLExceptionImpl(mStatus,
                "Statement::Close(DSQL_drop)",
                _("isc_dsql_free_statement failed."));
    }
}

void StatementImplFb3::SetNull(int param)
{
    CheckIsInit("Statement::SetNull");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::SetNull", _("The statement does not take parameters."));

    mInRow->SetNull(param);
}

void StatementImplFb3::Set(int param, bool value)
{
    CheckIsInit("Statement::Set[bool]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[bool]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, const char* cstring)
{
    CheckIsInit("Statement::Set[char*]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[char*]", _("The statement does not take parameters."));

    mInRow->Set(param, cstring);
}

void StatementImplFb3::Set(int param, const void* bindata, int len)
{
    CheckIsInit("Statement::Set[void*]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[void*]", _("The statement does not take parameters."));

    mInRow->Set(param, bindata, len);
}

void StatementImplFb3::Set(int param, const std::string& s)
{
    CheckIsInit("Statement::Set[string]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[string]", _("The statement does not take parameters."));

    mInRow->Set(param, s);
}

void StatementImplFb3::Set(int param, int16_t value)
{
    CheckIsInit("Statement::Set[int16_t]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[int16_t]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, int32_t value)
{
    CheckIsInit("Statement::Set[int32_t]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[int32_t]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, int64_t value)
{
    CheckIsInit("Statement::Set[int64_t]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[int64_t]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, float value)
{
    CheckIsInit("Statement::Set[float]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[float]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, double value)
{
    CheckIsInit("Statement::Set[double]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[double]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, const IBPP::Timestamp& value)
{
    CheckIsInit("Statement::Set[Timestamp]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[Timestamp]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, const IBPP::Date& value)
{
    CheckIsInit("Statement::Set[Date]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[Date]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, const IBPP::Time& value)
{
    CheckIsInit("Statement::Set[Time]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[Time]", _("The statement does not take parameters."));

    mInRow->Set(param, value);
}

void StatementImplFb3::Set(int param, const IBPP::Blob& blob)
{
    CheckIsInit("Statement::Set[Blob]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[Blob]", _("The statement does not take parameters."));

    mInRow->Set(param, blob);
}

void StatementImplFb3::Set(int param, const IBPP::Array& array)
{
    CheckIsInit("Statement::Set[Array]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[Array]", _("The statement does not take parameters."));

    mInRow->Set(param, array);
}

void StatementImplFb3::Set(int param, const IBPP::DBKey& key)
{
    CheckIsInit("Statement::Set[DBKey]");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Set[DBKey]", _("The statement does not take parameters."));

    mInRow->Set(param, key);
}

int StatementImplFb3::ParameterNum(const std::string& name)
{
    if (name.empty())
        throw LogicExceptionImpl("Statement::ColumnNum", _("Parameter name <empty> not found."));
    std::vector<int> params = this->FindParamsByName(name);
    if (params.size()>0)
        return params.at(0);  //Already returns ID+1
    throw LogicExceptionImpl("Statement::ColumnNum", _("Could not find matching parameter."));
}

std::vector<int> StatementImplFb3::FindParamsByName(std::string name)
{
    if (name.empty())
        throw LogicExceptionImpl("Statement::FindParamsByName", _("Parameter name <empty> not found."));

    std::vector<int> params;
    unsigned int i;
    for(i=0;i < parametersDetailedByName_.size() ; i++)
    {
        if (parametersDetailedByName_.at(i) == name)
            params.push_back(i+1);
    }
    return params;
}

std::vector<std::string> StatementImplFb3::ParametersByName()
{
    std::vector<std::string> vt;
    vt.reserve(parametersByName_.size());
    for (unsigned int i=0;i < parametersByName_.size() ; i++)
    {
        vt.push_back(parametersByName_.at(i));
    }
    return vt;
}

std::string StatementImplFb3::ParametersParser(std::string sql)
{
    unsigned int i;
    bool isDMLcheck = false;

    parametersByName_.clear();
    parametersDetailedByName_.clear();

    //Improve this part, verify the SQL, if is really necessary to process the parameters this time
    //Can be done in the begin

    //Here we verify, the job done recently was time lost?
    std::string isDML(sql);

    isDML.erase(isDML.begin(), std::find_if(isDML.begin(), isDML.end(), [](int c) { return !std::isspace(c); })); //lTrim

    std::transform(isDML.begin(), isDML.end(), isDML.begin(), [](char c) { return (char)std::toupper(c); }); //UpperCase (only bothered about ASCII text, cast is okay)

    std::string isDML4 = isDML.substr(0, 4);  std::string isDML6 = isDML.substr(0, 6);  std::string isDML7 = isDML.substr(0, 7);

    std::array<std::string, 6> dml =
    {
        "INSERT",
        "UPDATE",
        "DELETE",
        "SELECT",
        "EXECUTE",
        "WITH"
    };
    std::array<std::string, 6> check =
    {
        isDML6,
        isDML6,
        isDML6,
        isDML6,
        isDML7,
        isDML4
    };
    for (i = 0; i<dml.size(); i++) {
        if (check.at(i) == dml.at(i))
        {
            if (dml.at(i) == "EXECUTE")//is execute procedure or execute block? else, is DML for sure
            {
                auto x = dml.at(i).size();
                while (std::isspace(isDML.at(x)) && x<isDML.size())
                    x++;
                char p = isDML.at(x);
                //std::cout << "Char:"<<p << std::endl;
                if (p != 'P')//Procedure: execute procedure $sp(:params), OK to replace, else, use original, break loop
                    break;     //I don't want to replace :parameters inside an execute block..
                               //TODO:
                               //It is possible to insert parameters in the begin of the block, example:
                               //execute block (x double precision = ?, y double precision = ?)
                               //But it will need more work to do it
                               //Source: http://www.firebirdsql.org/refdocs/langrefupd20-execblock.html

            }
            isDMLcheck = true;
            break;
        }

    }
    if (!isDMLcheck) {
        //Probably is DDL... don't replace parameters then

        //std::cout << sql << std::endl;
        return sql;
    }






    //ctor
    bool comment = false, blockComment = false, palavra = false, quote = false, doubleQuote = false;
    std::ostringstream temp, sProcessedSQL;
    std::string debugProcessedSQL="";

    for(i= 0; i < sql.length(); i++){
        char nextChar=0;
        char previousChar=0;
        if (i < sql.length()-1)
            nextChar=sql.at(i+1);
        if (i > 0)
            previousChar=sql.at(i-1);

        if ((sql.at(i)=='\n') || (sql.at(i)=='\r')) {
            comment = false;
            //sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='-' && nextChar=='-' && !quote && !doubleQuote  && !blockComment) {
            comment = true;
        }else if (sql.at(i)=='/' && nextChar=='*' && !quote && !doubleQuote  && !blockComment) {
            blockComment = true;
        }else if (previousChar=='*' && sql.at(i)=='/' && blockComment) {
            blockComment = false;
            continue;
        }else if (sql.at(i)=='\'' && !doubleQuote && !quote && !comment && !blockComment) {
            quote = true;
            sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\'' && quote && !comment && !blockComment) {
            quote = false;
            //sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\"' && !quote && !doubleQuote && !comment && !blockComment) {
            doubleQuote = true;
            sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\"' && doubleQuote) {
            doubleQuote = false;

        } else if (quote || doubleQuote)
        {
            sProcessedSQL << sql.at(i);
        }
        if (!(comment || blockComment || quote || doubleQuote ) || palavra || sql.at(i)=='\n' || sql.at(i)=='\r')
        {
            comment = false;  //New line?
            if (sql.at(i)=='?'){

                if (FindParamsByName("?").size() == 0)//if first time:
                    parametersByName_.push_back("?");
                parametersDetailedByName_.push_back("?");
            }
            if (sql.at(i)==':'){
                palavra = true;
                //comeco = i;
            }else if (palavra) {
                if (std::isalnum(sql.at(i)) || sql.at(i)=='_' || sql.at(i)=='$')
                { //A-Z 0-9 _
                    temp << (char)std::toupper(sql.at(i));
                    if (i==sql.length()-1)  //se o I esta no fim do SQL...
                        goto fimPalavra;
                }else{
                    fimPalavra :
                    palavra = false;

                    //cout << "sqletro encontrado: \""<< temp.str()<<"\""<<endl;
                    sProcessedSQL << "?"<<"/*:"<<temp.str()<<"*/";
                    if (!(std::isalnum(sql.at(i)) || sql.at(i)=='_' || sql.at(i)=='$'))
                        sProcessedSQL << sql.at(i);
                    std::vector<int> tmp = FindParamsByName(temp.str());
                    if (tmp.size() == 0)//If first time
                        parametersByName_.push_back(temp.str());
                    parametersDetailedByName_.push_back(temp.str());
                    temp.str(std::string());temp.clear(); //Limpar StringStream
                }
            }
            else
            {
                sProcessedSQL << sql.at(i);
            }
        }
        debugProcessedSQL = sProcessedSQL.str();
    }
  std::cout << "sProcessedSQL: " << sProcessedSQL.str() << std::endl;

  return sProcessedSQL.str();
}

void StatementImplFb3::SetNull(std::string param)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0)
    {
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it)
    {
        SetNull(*it);
    }
}
void StatementImplFb3::Set(std::string param, int64_t value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImplFb3::Set(std::string param, int32_t value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImplFb3::Set(std::string param, int16_t value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImplFb3::Set(std::string param, float value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImplFb3::Set(std::string param, double value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImplFb3::Set(std::string param, const void* bindata, int len)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, bindata, len);
    }
}
void StatementImplFb3::Set(std::string param, const std::string& s)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, s);
    }
}
void StatementImplFb3::Set(std::string param, const char* cstring)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, cstring);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::Timestamp& value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::Time& value)
{
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::Date& value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::DBKey& key) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, key);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::Blob& blob) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, blob);
    }
}
void StatementImplFb3::Set(std::string param, const IBPP::Array& array) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, array);
    }
}
void StatementImplFb3::Set(std::string param, bool value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

bool StatementImplFb3::IsNull(int column)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::IsNull", _("The row is not initialized."));

    return mOutRow->IsNull(column);
}

bool StatementImplFb3::Get(int column, bool* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, bool& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, char* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, void* bindata, int& userlen)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, bindata, userlen);
}

bool StatementImplFb3::Get(int column, std::string& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, int16_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, int16_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, int32_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, int32_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, int64_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, int64_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, IBPP::ibpp_int128_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, float* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, float& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, double* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(column, *retvalue);
}

bool StatementImplFb3::Get(int column, double& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, IBPP::ibpp_dec16_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, IBPP::ibpp_dec34_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, retvalue);
}

bool StatementImplFb3::Get(int column, IBPP::Timestamp& timestamp)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, timestamp);
}

bool StatementImplFb3::Get(int column, IBPP::Date& date)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, date);
}

bool StatementImplFb3::Get(int column, IBPP::Time& time)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, time);
}

bool StatementImplFb3::Get(int column, IBPP::Blob& blob)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, blob);
}

bool StatementImplFb3::Get(int column, IBPP::DBKey& key)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, key);
}

bool StatementImplFb3::Get(int column, IBPP::Array& array)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(column, array);
}

bool StatementImplFb3::IsNull(const std::string& name)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::IsNull", _("The row is not initialized."));

    return mOutRow->IsNull(name);
}

bool StatementImplFb3::Get(const std::string& name, bool* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, bool& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, char* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get[char*]", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, void* retvalue, int& count)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get[void*,int]", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue, count);
}

bool StatementImplFb3::Get(const std::string& name, std::string& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::GetString", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int16_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int16_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int32_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int32_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int64_t* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, int64_t& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, float* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, float& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, double* retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
    if (retvalue == 0)
        throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

    return mOutRow->Get(name, *retvalue);
}

bool StatementImplFb3::Get(const std::string& name, double& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, IBPP::Timestamp& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, IBPP::Date& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, IBPP::Time& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string&name, IBPP::Blob& retblob)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retblob);
}

bool StatementImplFb3::Get(const std::string& name, IBPP::DBKey& retvalue)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retvalue);
}

bool StatementImplFb3::Get(const std::string& name, IBPP::Array& retarray)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

    return mOutRow->Get(name, retarray);
}

int StatementImplFb3::Columns()
{
    if (mOutRow == 0)
        return 0;

    return mOutRow->Columns();
}

int StatementImplFb3::ColumnNum(const std::string& name)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnNum", _("The row is not initialized."));

    return mOutRow->ColumnNum(name);
}

const char* StatementImplFb3::ColumnName(int varnum)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

    return mOutRow->ColumnName(varnum);
}

const char* StatementImplFb3::ColumnAlias(int varnum)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

    return mOutRow->ColumnAlias(varnum);
}

const char* StatementImplFb3::ColumnTable(int varnum)
{
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

    return mOutRow->ColumnTable(varnum);
}

int StatementImplFb3::ColumnSQLType(int varnum)
{
    CheckIsInit("Statement::ColumnSQLType");
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnSQLType", _("The statement does not return results."));

    return mOutRow->ColumnSQLType(varnum);
}

IBPP::SDT StatementImplFb3::ColumnType(int varnum)
{
    CheckIsInit("Statement::ColumnType");
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnType", _("The statement does not return results."));

    return mOutRow->ColumnType(varnum);
}

int StatementImplFb3::ColumnSubtype(int varnum)
{
    CheckIsInit("Statement::ColumnSubtype");
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnSubtype", _("The statement does not return results."));

    return mOutRow->ColumnSubtype(varnum);
}

int StatementImplFb3::ColumnSize(int varnum)
{
    CheckIsInit("Statement::ColumnSize");
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnSize", _("The row is not initialized."));

    return mOutRow->ColumnSize(varnum);
}

int StatementImplFb3::ColumnScale(int varnum)
{
    CheckIsInit("Statement::ColumnScale");
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnScale", _("The row is not initialized."));

    return mOutRow->ColumnScale(varnum);
}

int StatementImplFb3::Parameters()
{
    CheckIsInit("Statement::Parameters");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::Parameters", _("The statement uses no parameters."));

    return mInRow->Columns();
}

IBPP::SDT StatementImplFb3::ParameterType(int varnum)
{
    CheckIsInit("Statement::ParameterType");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::ParameterType", _("The statement uses no parameters."));

    return mInRow->ColumnType(varnum);
}

int StatementImplFb3::ParameterSubtype(int varnum)
{
    CheckIsInit("Statement::ParameterSubtype");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::ParameterSubtype", _("The statement uses no parameters."));

    return mInRow->ColumnSubtype(varnum);
}

int StatementImplFb3::ParameterSize(int varnum)
{
    CheckIsInit("Statement::ParameterSize");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::ParameterSize", _("The statement uses no parameters."));

    return mInRow->ColumnSize(varnum);
}

int StatementImplFb3::ParameterScale(int varnum)
{
    CheckIsInit("Statement::ParameterScale");
    if (mInRow == 0)
        throw LogicExceptionImpl("Statement::ParameterScale", _("The statement uses no parameters."));

    return mInRow->ColumnScale(varnum);
}

IBPP::Database StatementImplFb3::DatabasePtr() const
{
    return mDatabase;
}

IBPP::Transaction StatementImplFb3::TransactionPtr() const
{
    return mTransaction;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void StatementImplFb3::AttachDatabaseImpl(DatabaseImplFb3* database)
{
    if (database == 0)
        throw LogicExceptionImpl("Statement::AttachDatabase",
            _("Can't attach a 0 IDatabase object."));

    if (mDatabase != 0) mDatabase->DetachStatementImpl(this);
    mDatabase = database;
    mDatabase->AttachStatementImpl(this);
}

void StatementImplFb3::DetachDatabaseImpl()
{
    if (mDatabase == 0) return;

    Close();
    mDatabase->DetachStatementImpl(this);
    mDatabase = 0;
}

void StatementImplFb3::AttachTransactionImpl(TransactionImplFb3* transaction)
{
    if (transaction == 0)
        throw LogicExceptionImpl("Statement::AttachTransaction",
            _("Can't attach a 0 ITransaction object."));

    if (mTransaction != 0) mTransaction->DetachStatementImpl(this);
    mTransaction = transaction;
    mTransaction->AttachStatementImpl(this);
}

void StatementImplFb3::DetachTransactionImpl()
{
    if (mTransaction == 0) return;

    Close();
    mTransaction->DetachStatementImpl(this);
    mTransaction = 0;
}

void StatementImplFb3::CursorFree()
{
    if (mRes == nullptr)
        return;

    mRes->close(mStatus);
    mRes = nullptr;
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "StatementImplFb3::CursorFree(DSQL_close)",
                               _("IResultSet::close failed."));
}

StatementImplFb3::StatementImplFb3(DatabaseImplFb3* database, TransactionImplFb3* transaction)
    : mStm(nullptr), mRes(nullptr), mDatabase(0), mTransaction(0),
    mInRow(0), mOutRow(0),
    mType(IBPP::stUnknown)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mStatus = new CheckStatusWrapper(master->getStatus());

    AttachDatabaseImpl(database);
    if (transaction != 0) AttachTransactionImpl(transaction);
}

StatementImplFb3::~StatementImplFb3()
{
    try { Close(); }
        catch (...) { }
    try { if (mTransaction != 0) mTransaction->DetachStatementImpl(this); }
        catch (...) { }
    try { if (mDatabase != 0) mDatabase->DetachStatementImpl(this); }
        catch (...) { }

    delete mStatus;
}

