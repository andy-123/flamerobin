//   IBPP internal declarations - firebird api typedefs
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
#ifndef __INTERNAL_IBPPAPI_H__
#define __INTERNAL_IBPPAPI_H__

#include "ibpp.h"

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

#ifdef FIXME
Bin mir unsicher, ob diese Unit nötig ist ...
evtl. mal ganz entfernen und
* api-deklarationnen (c-style nach ibppfb1.h)
* interface-deklaration in ibppfb3.h importieren...
* evtl. hier nur die Makros zur verfügung stellen
* ok ... und die Prototypen ... dann passt es eigentlich so wie es ist
#endif

// From Firebird installation
#include "../firebird/include/ibase.h"
#ifdef FIXME
sollte mal entfernt werden -> nach ibppfb3.h wandern
#endif
// Firebird 3+ interfaces
#include "../firebird/include/firebird/Interface.h"

#ifdef IBPP_WINDOWS
/*#define IB_ENTRYPOINT(X) \
			if ((m_##X = (proto_##X*)GetProcAddress(mHandle, "isc_"#X)) == 0) \
                throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point isc_"#X" not found"))*/
#define FB_LOADDYN(H, X) \
            if ((m_##X = (proto_##X*)GetProcAddress(H, "fb_"#X)) == 0) \
                throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point fb_"#X" not found"))
#define FB_LOADDYN_NOTHROW(H, X) \
    if ((m_##X = (proto_##X*)GetProcAddress(H, "fb_"#X)) == 0) \
        m_##X = NULL;
#endif
#ifdef IBPP_UNIX
/*#ifdef IBPP_LATE_BIND
#define IB_ENTRYPOINT(X) \
    if ((m_##X = (proto_##X*)dlsym(mHandle,"isc_"#X)) == 0) \
        throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point isc_"#X" not found"))*/
#define FB_LOADDYN(H, X) \
    if ((m_##X = (proto_##X*)dlsym(H,"fb_"#X)) == 0) \
        throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point fb_"#X" not found"))
/*#else
#define IB_ENTRYPOINT(X) m_##X = (proto_##X*)isc_##X
#define FB_ENTRYPOINT(X) m_##X = (proto_##X*)fb_##X
#endif*/
#define FB_LOADDYN_NOTHROW(H, X) \
    if ((m_##X = (proto_##X*)dlsym(H,"fb_"#X)) == 0) \
        m_##X = NULL;
#endif

namespace ibpp_internals
{

//
//  FB3+ / get master-interface (fb_get_master_interface)
//
typedef
    Firebird::IMaster* ISC_EXPORT proto_get_master_interface();

typedef
    ISC_STATUS ISC_EXPORT proto_get_database_handle(ISC_STATUS*,
        isc_db_handle*, void*);

typedef
    ISC_STATUS ISC_EXPORT proto_get_transaction_handle(ISC_STATUS*,
        isc_tr_handle*, void*);
}

#endif