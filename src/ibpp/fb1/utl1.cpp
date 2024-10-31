///////////////////////////////////////////////////////////////////////////////
//
//	Subject : IBPP, Row class implementation
//
///////////////////////////////////////////////////////////////////////////////
//
//	(C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)
//
//	The contents of this file are subject to the IBPP License (the "License");
//	you may not use this file except in compliance with the License.  You may
//	obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
//	file which must have been distributed along with this file.
//
//	This software, distributed under the License, is distributed on an "AS IS"
//	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
//	License for the specific language governing rights and limitations
//	under the License.
//

#include "_ibpp.h"

namespace ibpp_internals::utlfb1
{

//
//	The following functions are helper conversions functions between IBPP
//	Date, Time, Timestamp and ISC_DATE, ISC_TIME and ISC_TIMESTAMP.
//	(They must be maintained if the encoding used by Firebird evolve.)
//	These helper functions are used from row.cpp and from array.cpp.
//

void encodeDate(ISC_DATE& isc_dt, const IBPP::Date& dt)
{
	// There simply has a shift of 15019 between the native Firebird
	// date model and the IBPP model.
	isc_dt = (ISC_DATE)(dt.GetDate() + 15019);
}

void decodeDate(IBPP::Date& dt, const ISC_DATE& isc_dt)
{
	// There simply has a shift of 15019 between the native Firebird
	// date model and the IBPP model.
	dt.SetDate((int)isc_dt - 15019);
}

void encodeTime(ISC_TIME& isc_tm, const IBPP::Time& tm)
{
	isc_tm = (ISC_TIME)tm.GetTime();
}

void decodeTime(IBPP::Time& tm, const ISC_TIME& isc_tm)
{
	tm.SetTime(IBPP::Time::tmNone, (int)isc_tm, IBPP::Time::TZ_NONE);
}

void decodeTimeTz(IBPP::Time& tm, const ISC_TIME_TZ& isc_tm)
{
    unsigned h, m, s, frac;
    char tzBuf[FB_MAX_TIME_ZONE_NAME_LENGTH];
    IBPP::Time::TimezoneMode tzMode = IBPP::Time::tmTimezone;

    fbIntfClass* fbIntf = fbIntfClass::getInstance();
    fbIntf->mUtil->decodeTimeTz(fbIntf->mStatus, &isc_tm, &h, &m, &s, &frac,
                                sizeof(tzBuf), tzBuf);

    tm.SetTime(tzMode, h, m, s, frac, isc_tm.time_zone, tzBuf);
}

void encodeTimestamp(ISC_TIMESTAMP& isc_ts, const IBPP::Timestamp& ts)
{
	encodeDate(isc_ts.timestamp_date, ts);
	encodeTime(isc_ts.timestamp_time, ts);
}

void decodeTimestamp(IBPP::Timestamp& ts, const ISC_TIMESTAMP& isc_ts)
{
	decodeDate(ts, isc_ts.timestamp_date);
	decodeTime(ts, isc_ts.timestamp_time);
}

void decodeTimestampTz(IBPP::Timestamp& ts, const ISC_TIMESTAMP_TZ& isc_ts)
{
    unsigned h, n, s, frac;
    unsigned y, m, d;
    char tzBuf[FB_MAX_TIME_ZONE_NAME_LENGTH];

    fbIntfClass* fbIntf = fbIntfClass::getInstance();
    fbIntf->mUtil->decodeTimeStampTz(fbIntf->mStatus, &isc_ts, &y, &m, &d, &h, &n, &s, &frac,
                                     sizeof(tzBuf), tzBuf);

    ts.Clear();
    ts.SetDate(y, m, d);
    ts.SetTime(IBPP::Time::tmTimezone, h, n, s, frac, isc_ts.time_zone, tzBuf);
}

}
