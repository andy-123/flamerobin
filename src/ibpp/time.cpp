//	IBPP, Time class implementation
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

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <ctime>

using namespace ibpp_internals;

const std::string IBPP::Time::TZ_GMT_FALLBACK = "GMT*";

void IBPP::Time::Now()
{
	time_t systime = time(0);
	tm* loctime = localtime(&systime);
	SetTime(IBPP::Time::tmNone, loctime->tm_hour, loctime->tm_min, loctime->tm_sec, 0, IBPP::Time::TZ_NONE, NULL);
}

void IBPP::Time::SetTime(TimezoneMode tzMode, int tm, int timezone)
{
    if (tm < 0 || tm > 863999999)
        throw LogicExceptionImpl("Time::SetTime", _("Invalid time value"));

    mTime = tm;
    mTimezoneMode = tzMode;
    SetTimezone(timezone);
}

void IBPP::Time::SetTime(TimezoneMode tzMode, int hour, int minute, int second, int tenthousandths, int timezone, char* fbTimezoneName)
{
    if (hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
            second < 0 || second > 59 ||
                tenthousandths < 0 || tenthousandths > 9999)
                    throw LogicExceptionImpl("Time::SetTime",
                        _("Invalid hour, minute, second values"));

    // Check, if fbclient fails to load icu. In this case
    // TimeZoneName "GMT*" ist returned.
    if ((tzMode == tmTimezone) && (fbTimezoneName == TZ_GMT_FALLBACK))
        tzMode = tmTimezoneGmtFallback;

    int tm;
    IBPP::itot(&tm, hour, minute, second, tenthousandths);
    SetTime(tzMode, tm, timezone);
}

void IBPP::Time::SetTimezone(int tz)
{
    mTimezone = tz;
}

int IBPP::Time::GetTime() const
{
    return mTime;
}

void IBPP::Time::GetTime(int& hour, int& minute, int& second) const
{
	IBPP::ttoi(GetTime(), &hour, &minute, &second, 0);
}

void IBPP::Time::GetTime(int& hour, int& minute, int& second, int& tenthousandths) const
{
	IBPP::ttoi(GetTime(), &hour, &minute, &second, &tenthousandths);
}

int IBPP::Time::Hours() const
{
	int hours;
	IBPP::ttoi(mTime, &hours, 0, 0, 0);
	return hours;
}

int IBPP::Time::Minutes() const
{
	int minutes;
	IBPP::ttoi(mTime, 0, &minutes, 0, 0);
	return minutes;
}

int IBPP::Time::Seconds() const
{
	int seconds;
	IBPP::ttoi(mTime, 0, 0, &seconds, 0);
	return seconds;
}

int IBPP::Time::SubSeconds() const	// Actually tenthousandths of seconds
{
	int tenthousandths;
	IBPP::ttoi(mTime, 0, 0, 0, &tenthousandths);
	return tenthousandths;
}

IBPP::Time::Time(TimezoneMode tzMode, int hour, int minute, int second, int tenthousandths, int timezone, char* fbTimezoneName)
{
	SetTime(tzMode, hour, minute, second, tenthousandths, timezone, fbTimezoneName);
}

IBPP::Time::Time(const IBPP::Time& copied)
{
	mTime = copied.mTime;
	mTimezoneMode = copied.mTimezoneMode;
	mTimezone = copied.mTimezone;
}

IBPP::Time& IBPP::Time::operator=(const IBPP::Timestamp& assigned)
{
	mTime = assigned.GetTime();
	mTimezone = assigned.GetTimezone();
	return *this;
}

IBPP::Time& IBPP::Time::operator=(const IBPP::Time& assigned)
{
	mTime = assigned.mTime;
	mTimezone = assigned.GetTimezone();
	return *this;
}

//	Time calculations. Internal format is the number of seconds elapsed since
//	midnight. Splits such a time in its hours, minutes, seconds components.

void IBPP::ttoi(int itime, int *h, int *m, int *s, int* t)
{
	int hh, mm, ss, tt;

	hh = (int) (itime / 36000000);	itime = itime - hh * 36000000;
	mm = (int) (itime / 600000);	itime = itime - mm * 600000;
	ss = (int) (itime / 10000);
	tt = (int) (itime - ss * 10000);

	if (h != 0) *h = hh;
	if (m != 0) *m = mm;
	if (s != 0) *s = ss;
	if (t != 0) *t = tt;

	return;
}

//	Get the internal time format, given hour, minute, second.

void IBPP::itot (int *ptime, int hour, int minute, int second, int tenthousandths)
{
	*ptime = hour * 36000000 + minute * 600000 + second * 10000 + tenthousandths;
	return;
}
