// Service class implementation

/* (C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

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
//FIXME entfernen
#include "fb1/ibppfb1.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

#ifdef IBPP_UNIX
#include <unistd.h>
#define Sleep(x) usleep(x)
#endif

#include <regex>
#include <iostream>

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void ServiceImplFb3::Connect()
{
    // Already connected
    if (mSvc != nullptr)
        return;

    // Attach to the Service Manager
    SPBFb3 spb(SPBFb3::attach);
    std::string connect;

    // Build a SPB based on	the properties
    spb.InsertTag(isc_spb_version);
    spb.InsertTag(isc_spb_current_version);
    if (!mUserName.empty())
    {
        spb.InsertString(isc_spb_user_name, mUserName);
        spb.InsertString(isc_spb_password, mUserPassword);
    }
    else
        spb.InsertString(isc_spb_trusted_auth, "");

    if (!mServerName.empty())
    {
        connect = mServerName;
        connect += ":";
    }

    connect += "service_mgr";

    Firebird::IProvider* prov = FactoriesImplFb3::gProv;
    mSvc = prov->attachServiceManager(mStatus, connect.c_str(), spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
    {
        // Should be, but better be sure...
        mSvc = nullptr;
        throw SQLExceptionImpl(mStatus, "Service::Connect", _("IProvider::attachServiceManager failed."));
    }

    std::string version;
    GetVersion(version);

    std::smatch m;

    if (std::regex_search(version, m, std::regex("\\d+.\\d+.\\d+.\\d+"))) {
        version = m[0];

        const std::regex re{ "((?:[^\\\\.]|\\\\.)+)(?:.|$)" };

        const std::vector<std::string> m_vecFields{ std::sregex_token_iterator(cbegin(version),
            cend(version), re, 1), std::sregex_token_iterator()
        };
        if (m_vecFields.size() == 4) {

            std::string str = m_vecFields[0];
            major_ver = atoi(str.c_str());

            str = m_vecFields[1];
            minor_ver = atoi(str.c_str());

            str = m_vecFields[2];
            rev_no = atoi(str.c_str());

            str = m_vecFields[3];
            build_no = atoi(str.c_str());
        }
    }
}

void ServiceImplFb3::Disconnect()
{
    // Already disconnected
    if (mSvc == 0)
        return;

    // Detach from the service manager
    mSvc->detach(mStatus);

    // Set mHandle to 0 now, just in case we need to throw, because Disconnect()
    // is called from Service destructor and we want to maintain a coherent state.
    mSvc = nullptr;
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Disconnect", _("IService::detach failed."));
}

void ServiceImplFb3::GetVersion(std::string& version)
{
    // Based on a patch provided by Torsten Martinsen (SourceForge 'bullestock')
    if (mSvc == 0)
        throw LogicExceptionImpl("Service::GetVersion", _("Service is not connected."));

    SPBFb3 spb(SPBFb3::query_receive);
    RB result(250);

    spb.InsertTag(isc_info_svc_server_version);

    mSvc->query(mStatus, 0, nullptr,
        spb.GetBufferLength(), spb.GetBuffer(),
        result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::GetVersion", _("isc_service_query failed"));

    result.GetString(isc_info_svc_server_version, version);
}

bool ServiceImplFb3::versionIsHigherOrEqualTo(int versionMajor, int versionMinor)
{
    return major_ver > versionMajor
        || (major_ver == versionMajor && minor_ver >= versionMinor);
}

void ServiceImplFb3::AddUser(const IBPP::User& user)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::AddUser", _("Service is not connected."));
    if (user.username.empty())
        throw LogicExceptionImpl("Service::AddUser", _("Username required."));
    if (user.password.empty())
        throw LogicExceptionImpl("Service::AddUser", _("Password required."));

    SPBFb3 spb(SPBFb3::service_start);
    spb.InsertTag(isc_action_svc_add_user);
    spb.InsertString(isc_spb_sec_username, user.username);
    spb.InsertString(isc_spb_sec_password, user.password);
    if (!user.firstname.empty())
        spb.InsertString(isc_spb_sec_firstname, user.firstname);
    if (!user.middlename.empty())
        spb.InsertString(isc_spb_sec_middlename, user.middlename);
    if (!user.lastname.empty())
        spb.InsertString(isc_spb_sec_lastname, user.lastname);
    if (user.userid != 0)
        spb.InsertQuad(isc_spb_sec_userid, (int32_t)user.userid);
    if (user.groupid != 0)
        spb.InsertQuad(isc_spb_sec_groupid, (int32_t)user.groupid);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::AddUser", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::ModifyUser(const IBPP::User& user)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::ModifyUser", _("Service is not connected."));
    if (user.username.empty())
        throw LogicExceptionImpl("Service::ModifyUser", _("Username required."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_modify_user);
    spb.InsertString(isc_spb_sec_username, user.username);
    if (!user.password.empty())
        spb.InsertString(isc_spb_sec_password, user.password);
    if (! user.firstname.empty())
        spb.InsertString(isc_spb_sec_firstname, user.firstname);
    if (!user.middlename.empty())
        spb.InsertString(isc_spb_sec_middlename, user.middlename);
    if (!user.lastname.empty())
        spb.InsertString(isc_spb_sec_lastname, user.lastname);
    if (user.userid != 0)
        spb.InsertQuad(isc_spb_sec_userid, (int32_t)user.userid);
    if (user.groupid != 0)
        spb.InsertQuad(isc_spb_sec_groupid, (int32_t)user.groupid);

    //(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::ModifyUser", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::RemoveUser(const std::string& username)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::RemoveUser", _("Service is not connected."));
    if (username.empty())
        throw LogicExceptionImpl("Service::RemoveUser", _("Username required."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_delete_user);
    spb.InsertString(isc_spb_sec_username, username);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::RemoveUser", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::GetUser(IBPP::User& user)
{
    if (mSvc == 0)
        throw LogicExceptionImpl("Service::GetUser", _("Service is not connected."));
    if (user.username.empty())
        throw LogicExceptionImpl("Service::GetUser", _("Username required."));

    SPBFb3 spb(SPBFb3::service_start);
    spb.InsertTag(isc_action_svc_display_user);
    spb.InsertString(isc_spb_sec_username, user.username);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::GetUser", _("isc_service_start failed"));

    RB result(8000);
    /*const unsigned char request[] =
    {
        isc_info_svc_get_users
    };*/
    SPBFb3 request(SPBFb3::query_receive);
    request.InsertTag(isc_info_svc_get_users);

	//(*getGDS().Call()->m_service_query)(status.Self(), &mHandle, 0, 0, 0,
	//	sizeof(request), request, result.Size(), result.Self());
    mSvc->query(mStatus, 0, nullptr,
        request.GetBufferLength(), request.GetBuffer(),
        result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::GetUser", _("IService::query failed"));

    char* p = (char*)result.GetBuffer();
    if (*p != isc_info_svc_get_users)
        throw SQLExceptionImpl(mStatus, "Service::GetUser", _("IService::query returned unexpected answer"));

    // Skips the 'isc_info_svc_get_users' and its total length
    p += 3;
    user.clear();
    while (*p != isc_info_end)
    {
        if (*p == isc_spb_sec_userid)
        {
            user.userid = (uint32_t)(*getGDS().Call()->m_vax_integer)(p+1, 4);
            p += 5;
        }
        else if (*p == isc_spb_sec_groupid)
        {
            user.groupid = (uint32_t)(*getGDS().Call()->m_vax_integer)(p+1, 4);
            p += 5;
        }
        else
        {
            unsigned short len = (unsigned short)(*getGDS().Call()->m_vax_integer)(p+1, 2);
            switch (*p)
            {
            case isc_spb_sec_username :
                // For each user, this is the first element returned
                if (len != 0) user.username.assign(p+3, len);
                break;
            case isc_spb_sec_password :
                if (len != 0) user.password.assign(p+3, len);
                break;
            case isc_spb_sec_firstname :
                if (len != 0) user.firstname.assign(p+3, len);
                break;
            case isc_spb_sec_middlename :
                if (len != 0) user.middlename.assign(p+3, len);
                break;
            case isc_spb_sec_lastname :
                if (len != 0) user.lastname.assign(p+3, len);
                break;
            }
            p += (3 + len);
        }
    }
}

void ServiceImplFb3::GetUsers(std::vector<IBPP::User>& users)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::GetUsers", _("Service is not connected."));

    SPBFb3 spb(SPBFb3::service_start);
    spb.InsertTag(isc_action_svc_display_user);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::GetUsers", _("isc_service_start failed"));

    RB result(0xFFFF);
    const unsigned char request[] =
    {
            isc_info_svc_get_users
    };
	//(*getGDS().Call()->m_service_query)(status.Self(), &mHandle, 0, 0, 0,
	//	sizeof(request), request, result.Size(), result.Self());
    mSvc->query(mStatus, 0, nullptr,
        sizeof(request), request,
        result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::GetUsers", _("isc_service_query failed"));

    users.clear();
    char* p = (char*)result.GetBuffer();
    if (*p != isc_info_svc_get_users)
        throw SQLExceptionImpl(mStatus, "Service::GetUsers", _("isc_service_query returned unexpected answer"));

    char* pEnd = p + (unsigned short)(*getGDS().Call()->m_vax_integer)(p+1, 2);
    // Skips the 'isc_info_svc_get_users' and its total length
    p += 3;
    IBPP::User user;
    while (p < pEnd && *p != isc_info_end)
    {
        if (*p == isc_spb_sec_userid)
        {
            user.userid = (uint32_t)(*getGDS().Call()->m_vax_integer)(p+1, 4);
            p += 5;
        }
        else if (*p == isc_spb_sec_groupid)
        {
            user.groupid = (uint32_t)(*getGDS().Call()->m_vax_integer)(p+1, 4);
            p += 5;
        }
        else
        {
            unsigned short len = (unsigned short)(*getGDS().Call()->m_vax_integer)(p+1, 2);
            switch (*p)
            {
            case isc_spb_sec_username :
                // For each user, this is the first element returned
                if (! user.username.empty()) users.push_back(user);	// Flush previous user
                user.clear();
                if (len != 0) user.username.assign(p+3, len);
                break;
            case isc_spb_sec_password :
                if (len != 0) user.password.assign(p+3, len);
                break;
            case isc_spb_sec_firstname :
                if (len != 0) user.firstname.assign(p+3, len);
                break;
            case isc_spb_sec_middlename :
                if (len != 0) user.middlename.assign(p+3, len);
                break;
            case isc_spb_sec_lastname :
                if (len != 0) user.lastname.assign(p+3, len);
                break;
            }
            p += (3 + len);
        }
    }
    // Flush last user
    if (!user.username.empty())
        users.push_back(user);
}

void ServiceImplFb3::SetPageBuffers(const std::string& dbfile, int buffers)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::SetPageBuffers", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::SetPageBuffers", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    IXpbBuilder* mSPB = spb.GetFbIntf();
    mSPB->clear(mStatus);
    mSPB->insertTag(mStatus, isc_action_svc_properties);
    //spb.InsertTag(isc_action_svc_properties);
    mSPB->insertString(mStatus, isc_spb_dbname, dbfile.c_str());
    //spb.InsertString(isc_spb_dbname, dbfile);
    mSPB->insertInt(mStatus, isc_spb_prp_page_buffers, buffers);
    //spb.InsertQuad(isc_spb_prp_page_buffers, buffers);

    if (dbfile.length() > 255)
        throw LogicExceptionImpl("Service::SetPageBuffers", _("Database file path is too long (max 255 characters)."));

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::SetPageBuffers", _("isc_service_start failed"));
    
    Wait();
}

void ServiceImplFb3::SetSweepInterval(const std::string& dbfile, int sweep)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::SetSweepInterval", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::SetSweepInterval", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);
    spb.InsertQuad(isc_spb_prp_sweep_interval, sweep);

    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::SetSweepInterval", _("IService::start failed"));

    Wait();
}

void ServiceImplFb3::SetSyncWrite(const std::string& dbfile, bool sync)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::SetSyncWrite", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::SetSyncWrite", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);
    if (sync)
        spb.InsertByte(isc_spb_prp_write_mode, (char)isc_spb_prp_wm_sync);
    else
        spb.InsertByte(isc_spb_prp_write_mode, (char)isc_spb_prp_wm_async);

    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::SetSyncWrite", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::SetReadOnly(const std::string& dbfile, bool readonly)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::SetReadOnly", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::SetReadOnly", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);
    if (readonly)
        spb.InsertByte(isc_spb_prp_access_mode, (char)isc_spb_prp_am_readonly);
    else
        spb.InsertByte(isc_spb_prp_access_mode, (char)isc_spb_prp_am_readwrite);

   x //(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::SetReadOnly", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::SetReserveSpace(const std::string& dbfile, bool reserve)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::SetReserveSpace", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::SetReserveSpace", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);
    if (reserve)
        spb.InsertByte(isc_spb_prp_reserve_space, (char)isc_spb_prp_res);
    else
        spb.InsertByte(isc_spb_prp_reserve_space, (char)isc_spb_prp_res_use_full);

	x//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::SetReserveSpace", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::Shutdown(const std::string& dbfile, IBPP::DSM flags, int sectimeout)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Shutdown", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Shutdown", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);

    // Shutdown mode
    //if (flags & IBPP::dsCache) spb.InsertQuad(isc_spb_prp, sectimeout)
    if (flags & IBPP::dsDenyTrans)
        spb.InsertQuad(isc_spb_prp_deny_new_transactions, sectimeout);
    if (flags & IBPP::dsDenyAttach)
        spb.InsertQuad(isc_spb_prp_deny_new_attachments, sectimeout);
    if (flags & IBPP::dsForce)
        spb.InsertQuad(isc_spb_prp_force_shutdown, sectimeout);

    // Database Mode
    if (flags & IBPP::dsNormal)
        spb.InsertByte(isc_spb_prp_shutdown_mode, isc_spb_prp_sm_normal);
    if (flags & IBPP::dsSingle)
        spb.InsertByte(isc_spb_prp_shutdown_mode, isc_spb_prp_sm_single);
    if (flags & IBPP::dsMulti)
        spb.InsertByte(isc_spb_prp_shutdown_mode, isc_spb_prp_sm_multi);
    if (flags & IBPP::dsFull)
        spb.InsertByte(isc_spb_prp_shutdown_mode, isc_spb_prp_sm_full);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Shutdown", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::Restart(const std::string& dbfile, IBPP::DSM /*flags*/)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Restart", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Restart", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_properties);
    spb.InsertString(isc_spb_dbname, dbfile);
    spb.InsertQuad(isc_spb_options, isc_spb_prp_db_online);

    //(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Restart", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::Sweep(const std::string& dbfile)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Sweep", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Sweep", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_repair);
    spb.InsertString(isc_spb_dbname, dbfile);
    spb.InsertQuad(isc_spb_options, isc_spb_rpr_sweep_db);

    //(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Sweep", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::Repair(const std::string& dbfile, IBPP::RPF flags)
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Repair", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Repair", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_repair);
    spb.InsertString(isc_spb_dbname, dbfile);

    unsigned int mask;
    if (flags & IBPP::rpValidateFull)
        mask = (isc_spb_rpr_full | isc_spb_rpr_validate_db);
    else if (flags & IBPP::rpValidatePages)
        mask = isc_spb_rpr_validate_db;
    else if (flags & IBPP::rpMendRecords)
        mask = isc_spb_rpr_mend_db;
    else throw LogicExceptionImpl("Service::Repair",
        _("One of rpMendRecords, rpValidatePages, rpValidateFull is required."));

    if (flags & IBPP::rpReadOnly)
        mask |= isc_spb_rpr_check_db;
    if (flags & IBPP::rpIgnoreChecksums)
        mask |= isc_spb_rpr_ignore_checksum;
    if (flags & IBPP::rpKillShadows)
        mask |= isc_spb_rpr_kill_shadows;

    spb.InsertQuad(isc_spb_options, mask);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Repair", _("isc_service_start failed"));

    Wait();
}

void ServiceImplFb3::StartBackup(
    const std::string& dbfile,	const std::string& bkfile, const std::string& /*outfile*/,
    const int factor, IBPP::BRF flags,
    const std::string& cryptName, const std::string& keyHolder, const std::string& keyName,
    const std::string& skipData, const std::string& includeData, const int verboseInteval,
    const int parallelWorkers
    )
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Backup", _("Service is not connected."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Backup", _("Main database file must be specified."));
    if (bkfile.empty())
        throw LogicExceptionImpl("Service::Backup", _("Backup file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_backup);
    spb.InsertString(isc_spb_dbname, dbfile);
    spb.InsertString(isc_spb_bkp_file, bkfile);

    if (versionIsHigherOrEqualTo(3, 0))
    {
        if ((flags & IBPP::brVerbose) && (verboseInteval == 0))
            spb.InsertTag(isc_spb_verbose);
        if (verboseInteval > 0)
            spb.InsertQuad(isc_spb_verbint, verboseInteval);
    }
    else if (flags & IBPP::brVerbose)
        spb.InsertTag(isc_spb_verbose);

    if (factor > 0)
        spb.InsertQuad(isc_spb_bkp_factor, factor);

    if (!skipData.empty() && versionIsHigherOrEqualTo(3, 0))
        spb.InsertString(isc_spb_bkp_skip_data, skipData);
    if (!includeData.empty() && versionIsHigherOrEqualTo(4, 0))
        spb.InsertString(isc_spb_bkp_include_data, includeData);

    if (parallelWorkers > 0 && versionIsHigherOrEqualTo(3, 0))
        spb.InsertQuad(isc_spb_bkp_parallel_workers, parallelWorkers);

    if (versionIsHigherOrEqualTo(4, 0))
    {
        if (!cryptName.empty())
            spb.InsertString(isc_spb_bkp_crypt, cryptName);
        if (!keyHolder.empty())
            spb.InsertString(isc_spb_bkp_keyholder, keyHolder);
        if (!keyName.empty())
            spb.InsertString(isc_spb_bkp_keyname, keyName);
    }

    unsigned int mask = 0;
    if (flags & IBPP::brConvertExtTables)
        mask |= isc_spb_bkp_convert;
    if (flags & IBPP::brExpand)
        mask |= isc_spb_bkp_expand;
    if (flags & IBPP::brNoGarbageCollect)
        mask |= isc_spb_bkp_no_garbage_collect;
    if (flags & IBPP::brIgnoreChecksums)
        mask |= isc_spb_bkp_ignore_checksums;
    if (flags & IBPP::brIgnoreLimbo)
        mask |= isc_spb_bkp_ignore_limbo;
    if (flags & IBPP::brNonTransportable)
        mask |= isc_spb_bkp_non_transportable;
    if (flags & IBPP::brOldDescriptions)
        mask |= isc_spb_bkp_old_descriptions;

    if (versionIsHigherOrEqualTo(2, 5))
    {
        if (flags & IBPP::brNoDBTriggers)
            mask |= isc_spb_bkp_no_triggers;
        if (flags & IBPP::brMetadataOnly)
            mask |= isc_spb_bkp_metadata_only;
    }

    if ((flags & IBPP::brZip) && versionIsHigherOrEqualTo(4, 0))
        mask |= isc_spb_bkp_zip;

    if (versionIsHigherOrEqualTo(2, 5))
    {
        std::string stFlags = "";

        if (flags & IBPP::brstatistics_time)
            stFlags += "T";
        if (flags & IBPP::brstatistics_delta)
            stFlags += "D";
        if (flags & IBPP::brstatistics_pagereads)
            stFlags += "R";
        if (flags & IBPP::brstatistics_pagewrites)
            stFlags += "W";

        if(!stFlags.empty())
            spb.InsertString(isc_spb_bkp_stat, stFlags);
    }

    if (mask != 0)
        spb.InsertQuad(isc_spb_options, mask);

	//(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Backup", _("isc_service_start failed"));
}

void ServiceImplFb3::StartRestore(
    const std::string& bkfile, const std::string& dbfile, const std::string& /*outfile*/,
    int pagesize, int buffers, IBPP::BRF flags,
    const std::string& cryptName, const std::string& keyHolder, const std::string& keyName,
    const std::string& skipData, const std::string& includeData, const int verboseInteval,
    const int parallelWorkers
    )
{
    if (mSvc == nullptr)
        throw LogicExceptionImpl("Service::Restore", _("Service is not connected."));
    if (bkfile.empty())
        throw LogicExceptionImpl("Service::Restore", _("Backup file must be specified."));
    if (dbfile.empty())
        throw LogicExceptionImpl("Service::Restore", _("Main database file must be specified."));

    SPBFb3 spb(SPBFb3::service_start);

    spb.InsertTag(isc_action_svc_restore);
    spb.InsertString(isc_spb_bkp_file, bkfile);
    spb.InsertString(isc_spb_dbname, dbfile);

    if (versionIsHigherOrEqualTo(3, 0))
    {
        if ((flags & IBPP::brVerbose) && (verboseInteval == 0))
            spb.InsertTag(isc_spb_verbose);
        if (verboseInteval > 0)
            spb.InsertQuad(isc_spb_verbint, verboseInteval);
    }
    else
        if (flags & IBPP::brVerbose)
            spb.InsertTag(isc_spb_verbose);

    if (pagesize > 0)
        spb.InsertQuad(isc_spb_res_page_size, pagesize);
    if (buffers > 0)
        spb.InsertQuad(isc_spb_res_buffers, buffers);

    if (parallelWorkers > 0 && versionIsHigherOrEqualTo(3, 0))
        spb.InsertQuad(isc_spb_bkp_parallel_workers, parallelWorkers);

    if (!skipData.empty() && versionIsHigherOrEqualTo(3, 0))
        spb.InsertString(isc_spb_res_skip_data, skipData);
    if (!includeData.empty() && versionIsHigherOrEqualTo(4, 0))
        spb.InsertString(isc_spb_res_include_data, includeData);

    if (versionIsHigherOrEqualTo(4, 0))
    {
        if (!cryptName.empty())
            spb.InsertString(isc_spb_res_crypt, cryptName);
        if (!keyHolder.empty())
            spb.InsertString(isc_spb_res_keyholder, keyHolder);
        if (!keyName.empty())
            spb.InsertString(isc_spb_res_keyname, keyName);
    }

    spb.InsertByte(isc_spb_res_access_mode, (flags & IBPP::brDatabase_readonly) ? isc_spb_res_am_readonly : isc_spb_res_am_readwrite);

    if (versionIsHigherOrEqualTo(4, 0)) 
    {
        if (flags & IBPP::brReplicaMode_none)
            spb.InsertByte(isc_spb_res_replica_mode, isc_spb_res_rm_none);
        if (flags & IBPP::brReplicaMode_readonly)
            spb.InsertByte(isc_spb_res_replica_mode, isc_spb_res_rm_readonly);
        if (flags & IBPP::brReplicaMode_readwrite)
            spb.InsertByte(isc_spb_res_replica_mode, isc_spb_res_rm_readwrite);
    }

    unsigned int mask = (flags & IBPP::brReplace) ? isc_spb_res_replace : isc_spb_res_create;	// Safe default mode

    if (flags & IBPP::brDeactivateIdx)
        mask |= isc_spb_res_deactivate_idx;
    if (flags & IBPP::brNoShadow)
        mask |= isc_spb_res_no_shadow;
    if (flags & IBPP::brNoValidity)
        mask |= isc_spb_res_no_validity;
    if (flags & IBPP::brPerTableCommit)
        mask |= isc_spb_res_one_at_a_time;
    if (flags & IBPP::brUseAllSpace)
        mask |= isc_spb_res_use_all_space;

    if (versionIsHigherOrEqualTo(2, 5))
    {
        if (flags & IBPP::brMetadataOnly)
            mask |= isc_spb_res_metadata_only;
        if (flags & IBPP::brFix_Fss_Data)
            spb.InsertString(isc_spb_res_fix_fss_data, mCharSet);
        if (flags & IBPP::brFix_Fss_Metadata)
            spb.InsertString(isc_spb_res_fix_fss_metadata, mCharSet);

        std::string stFlags = "";

        if (flags & IBPP::brstatistics_time)
            stFlags += "T";
        if (flags & IBPP::brstatistics_delta)       
            stFlags += "D";
        if (flags & IBPP::brstatistics_pagereads)   
            stFlags += "R";
        if (flags & IBPP::brstatistics_pagewrites)  
            stFlags += "W";

        if (!stFlags.empty())
            spb.InsertString(isc_spb_bkp_stat, stFlags);

    }

    if (mask != 0)
        spb.InsertQuad(isc_spb_options, mask);

    //(*getGDS().Call()->m_service_start)(status.Self(), &mHandle, 0, spb.Size(), spb.Self());
    mSvc->start(mStatus, spb.GetBufferLength(), spb.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "Service::Restore", _("isc_service_start failed"));
}

const char* ServiceImplFb3::WaitMsg()
{
    SPBFb3 req(SPBFb3::query_receive);
    RB result(1024);

    // Request one line of textual output
    req.InsertTag(isc_info_svc_line);

	// _service_query will only block until a line of result is available
	// (or until the end of the task if it does not report information)
	//(*getGDS().Call()->m_service_query)(status.Self(), &mHandle, 0, 0, 0,
	//	req.Size(),	req.Self(),	result.Size(), result.Self());
    mSvc->query(mStatus, 0, nullptr,
        req.GetBufferLength(), req.GetBuffer(),
        result.GetBufferLength(), result.GetBuffer());
    if (mStatus->isDirty())
        throw SQLExceptionImpl(mStatus, "ServiceImplFb3::Wait", _("isc_service_query failed"));

    // If message length is	zero bytes,	task is	finished
    if (result.GetString(isc_info_svc_line,	mWaitMessage) == 0)
        return 0;

    // Task	is not finished, but we	have something to report
    return mWaitMessage.c_str();
}

void ServiceImplFb3::Wait()
{
    SPBFb3 spb(SPBFb3::query_receive);
    RB result(1024);
    std::string msg;

    spb.InsertTag(isc_info_svc_line);
    for (;;)
    {
        // Sleeps 1 millisecond upfront. This will release the remaining
        // timeslot of the thread. Doing so will give a good chance for small
        // services tasks to finish before we check if they are still running.
        // The deal is to limit (in that particular case) the number of loops
        // polling _service_query that will happen.

        Sleep(1);

        // _service_query will only block until a line of result is available
        // (or until the end of the task if it does not report information)
        //(*getGDS().Call()->m_service_query)(status.Self(), &mHandle, 0, 0,	0,
        //    spb.Size(),	spb.Self(),	result.Size(), result.Self());
        mSvc->query(mStatus, 0, nullptr,
            spb.GetBufferLength(), spb.GetBuffer(),
            result.GetBufferLength(), result.GetBuffer());
        if (mStatus->isDirty())
            throw SQLExceptionImpl(mStatus, "ServiceImplFb3::Wait", _("isc_service_query failed"));

        // If message length is	zero bytes,	task is	finished
        if (result.GetString(isc_info_svc_line,	msg) ==	0)
            return;

        result.Reset();
    }
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void ServiceImplFb3::SetServerName(const char* newName)
{
    if (newName == 0)
        mServerName.erase();
    else
        mServerName = newName;
}

void ServiceImplFb3::SetUserName(const char* newName)
{
    if (newName == 0)
        mUserName.erase();
    else
        mUserName = newName;
}

void ServiceImplFb3::SetUserPassword(const char* newPassword)
{
    if (newPassword == 0)
        mUserPassword.erase();
    else
        mUserPassword = newPassword;
}

void ServiceImplFb3::SetCharSet(const char* newCharset)
{
    if (newCharset == 0)
        mCharSet.erase();
    else
        mCharSet = newCharset;
}

void ServiceImplFb3::SetRoleName(const char* newRoleName)
{
    if (newRoleName == 0)
        mRoleName.erase();
    else
        mRoleName = newRoleName;
}

ServiceImplFb3::ServiceImplFb3(const std::string& ServerName,
    const std::string& UserName, const std::string& UserPassword,
    const std::string& RoleName, const std::string& CharSet)
    : mSvc(nullptr), mServerName(ServerName),
    mUserName(UserName), mUserPassword(UserPassword),
    mRoleName(RoleName), mCharSet(CharSet)
{
    IMaster* master = FactoriesImplFb3::gMaster;
    mStatus = new CheckStatusWrapper(master->getStatus());
}

ServiceImplFb3::~ServiceImplFb3()
{
    try { if (Connected()) Disconnect(); }
        catch (...) { }
    delete mStatus;
}
