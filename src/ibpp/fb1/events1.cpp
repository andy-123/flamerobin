//	Subject : IBPP, internal EventsImplFb1 class implementation
//
//	COMMENTS
//
//	SPECIAL WARNING COMMENT (by Olivier Mascia, 2000 Nov 12)
//	The way this source file handles events is not publicly documented, in
//	the ibase.h header file or in the IB 6.0 documentation. This documentation
//	suggests to use the API isc_event_block to construct vectors of events.
//	Unfortunately, this API takes a variable number of parameters to specify
//	the list of event names. In addition, the documentation warn on not using
//	more than 15 names. This is a sad limitation, partly because the maximum
//	number of parameters safely processed in such an API is very compiler
//	dependant and also because isc_event_counts() is designed to return counts
//	through the IB status vector which is a vector of 20 32-bits integers !
//	From reverse engineering of the isc_event_block() API in
//	source file jrd/alt.c (as available on fourceforge.net/project/InterBase as
//	of 2000 Nov 12), it looks like the internal format of those EPB is simple.
//	An EPB starts by a byte with value 1. A version identifier of some sort.
//	Then a small cluster is used for each event name. The cluster starts with
//	a byte for the length of the event name (no final '\0'). Followed by the N
//	characters of the name itself (no final '\0'). The cluster ends with 4 bytes
//	preset to 0.
//
//	SPECIAL CREDIT (July 2004) : this is a complete re-implementation of this
//	class, directly based on work by Val Samko.
//	The whole event handling has then be completely redesigned, based on the old
//	EPB class to bring up the current IBPP::Events implementation.

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
#include "fb1/ibppfb1.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

const size_t EventsImplFb1::MAXEVENTNAMELEN = 127;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void EventsImplFb1::Add(const std::string& eventname, IBPP::EventInterface* objref)
{
	if (eventname.size() == 0)
		throw LogicExceptionImpl("Events::Add", _("Zero length event names not permitted"));
	if (eventname.size() > MAXEVENTNAMELEN)
		throw LogicExceptionImpl("Events::Add", _("Event name is too long"));
	if ((mEventBuffer.size() + eventname.length() + 5) > 32766)	// max signed 16 bits integer minus one
		throw LogicExceptionImpl("Events::Add",
			_("Can't add this event, the events list would overflow IB/FB limitation"));

	Cancel();

	// 1) Alloc or grow the buffers
	size_t prev_buffer_size = mEventBuffer.size();
	size_t needed = ((prev_buffer_size==0) ? 1 : 0) + eventname.length() + 5;
	// Initial alloc will require one more byte, we need 4 more bytes for
	// the count itself, and one byte for the string length prefix

	mEventBuffer.resize(mEventBuffer.size() + needed);
	mResultsBuffer.resize(mResultsBuffer.size() + needed);
	if (prev_buffer_size == 0)
		mEventBuffer[0] = mResultsBuffer[0] = 1; // First byte is a 'one'. Documentation ??

	// 2) Update the buffers (append)
	{
		Buffer::iterator it = mEventBuffer.begin() +
				((prev_buffer_size==0) ? 1 : prev_buffer_size); // Byte after current content
		*(it++) = static_cast<char>(eventname.length());
		it = std::copy(eventname.begin(), eventname.end(), it);
		// We initialize the counts to (uint32_t)(-1) to initialize properly, see FireActions()
		*(it++) = -1; *(it++) = -1; *(it++) = -1; *it = -1;
	}

	// copying new event to the results buffer to keep event_buffer_ and results_buffer_ consistant,
	// otherwise we might get a problem in `FireActions`
	// Val Samko, val@digiways.com
	std::copy(mEventBuffer.begin() + prev_buffer_size,
		mEventBuffer.end(), mResultsBuffer.begin() + prev_buffer_size);

	// 3) Alloc or grow the objref array and update the objref array (append)
	mObjectReferences.push_back(objref);

	Queue();
}

void EventsImplFb1::Drop(const std::string& eventname)
{
	if (eventname.size() == 0)
		throw LogicExceptionImpl("EventsImplFb1::Drop", _("Zero length event names not permitted"));
	if (eventname.size() > MAXEVENTNAMELEN)
		throw LogicExceptionImpl("EventsImplFb1::Drop", _("Event name is too long"));

	if (mEventBuffer.size() <= 1) return;	// Nothing to do, but not an error

	Cancel();

	// 1) Find the event in the buffers
	typedef EventBufferIteratorFb1<Buffer::iterator> EventIterator;
	EventIterator eit(mEventBuffer.begin()+1);
	EventIterator rit(mResultsBuffer.begin()+1);

	for (ObjRefs::iterator oit = mObjectReferences.begin();
			oit != mObjectReferences.end();
				++oit, ++eit, ++rit)
	{
		if (eventname != eit.get_name()) continue;

		// 2) Event found, remove it
		mEventBuffer.erase(eit.begin(), eit.end());
		mResultsBuffer.erase(rit.begin(), rit.end());
		mObjectReferences.erase(oit);
		break;
	}

	Queue();
}

void EventsImplFb1::List(std::vector<std::string>& events)
{
	events.clear();

	if (mEventBuffer.size() <= 1) return;	// Nothing to do, but not an error

	typedef EventBufferIteratorFb1<Buffer::iterator> EventIterator;
	EventIterator eit(mEventBuffer.begin()+1);

	for (ObjRefs::iterator oit = mObjectReferences.begin();
			oit != mObjectReferences.end();
				++oit, ++eit)
	{
		events.push_back(eit.get_name());
	}
}

void EventsImplFb1::Clear()
{
	Cancel();

	mObjectReferences.clear();
	mEventBuffer.clear();
	mResultsBuffer.clear();
}

void EventsImplFb1::Dispatch()
{
	// If no events registered, nothing to do of course.
	if (mEventBuffer.size() == 0) return;

	// Let's fire the events actions for all the events which triggered, if any, and requeue.
	FireActions();
	Queue();
}

IBPP::Database EventsImplFb1::DatabasePtr() const
{
	if (mDatabase == 0) throw LogicExceptionImpl("Events::DatabasePtr",
			_("No Database is attached."));
	return mDatabase;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void EventsImplFb1::Queue()
{
	if (! mQueued)
	{
		if (mDatabase->GetHandle() == 0)
			throw LogicExceptionImpl("EventsImplFb1::Queue",
				  _("Database is not connected"));

		IBS vector;
		mTrapped = false;
		mQueued = true;
		(*getGDS().Call()->m_que_events)(vector.Self(), mDatabase->GetHandlePtr(), &mId,
			short(mEventBuffer.size()), &mEventBuffer[0],
				(isc_callback)EventHandler, (char*)this);

		if (vector.Errors())
		{
			mId = 0;	// Should be, but better be safe
			mQueued = false;
			throw SQLExceptionImpl(vector, "EventsImplFb1::Queue",
				_("isc_que_events failed"));
		}
	}
}

void EventsImplFb1::Cancel()
{
	if (mQueued)
	{
		if (mDatabase->GetHandle() == 0) throw LogicExceptionImpl("EventsImplFb1::Cancel",
			_("Database is not connected"));

		IBS vector;

		// A call to cancel_events will call *once* the handler routine, even
		// though no events had fired. This is why we first set mEventsQueued
		// to false, so that we can be sure to dismiss those unwanted callbacks
		// subsequent to the execution of isc_cancel_events().
		mTrapped = false;
		mQueued = false;
		(*getGDS().Call()->m_cancel_events)(vector.Self(), mDatabase->GetHandlePtr(), &mId);

	    if (vector.Errors())
		{
			mQueued = true;	// Need to restore this as cancel failed
	    	throw SQLExceptionImpl(vector, "EventsImplFb1::Cancel",
	    		_("isc_cancel_events failed"));
		}

		mId = 0;	// Should be, but better be safe
	}
}

void EventsImplFb1::FireActions()
{
	if (mTrapped)
	{
		typedef EventBufferIteratorFb1<Buffer::iterator> EventIterator;
		EventIterator eit(mEventBuffer.begin()+1);
		EventIterator rit(mResultsBuffer.begin()+1);

		for (ObjRefs::iterator oit = mObjectReferences.begin();
			 oit != mObjectReferences.end();
				 ++oit, ++eit, ++rit)
		{
			if (eit == EventIterator(mEventBuffer.end())
				  || rit == EventIterator(mResultsBuffer.end()))
				throw LogicExceptionImpl("EventsImplFb1::FireActions", _("Internal buffer size error"));
			uint32_t vnew = rit.get_count();
			uint32_t vold = eit.get_count();
			if (vnew > vold)
			{
				// Fire the action
				try
				{
					(*oit)->ibppEventHandler(this, eit.get_name(), (int)(vnew - vold));
				}
				catch (...)
				{
					std::copy(rit.begin(), rit.end(), eit.begin());
					throw;
				}
				std::copy(rit.begin(), rit.end(), eit.begin());
			}
			// This handles initialization too, where vold == (uint32_t)(-1)
			// Thanks to M. Hieke for this idea and related initialization to (-1)
			if (vnew != vold)
 				std::copy(rit.begin(), rit.end(), eit.begin());
		}
	}
}

// This function must keep this prototype to stay compatible with
// what isc_que_events() expects

void EventsImplFb1::EventHandler(const char* object, short size, const char* tmpbuffer)
{
	// >>>>> This method is a STATIC member !! <<<<<
	// Consider this method as a kind of "interrupt handler". It should do as
	// few work as possible as quickly as possible and then return.
	// Never forget: this is called by the Firebird client code, on *some*
	// thread which might not be (and won't probably be) any of your application
	// thread. This function is to be considered as an "interrupt-handler" of a
	// hardware driver.

	// There can be spurious calls to EventHandler from FB internal. We must
	// dismiss those calls.
	if (object == 0 || size == 0 || tmpbuffer == 0) return;

	EventsImplFb1* evi = (EventsImplFb1*)object;	// Ugly, but wanted, c-style cast

	if (evi->mQueued)
	{
		try
		{
			char* rb = &evi->mResultsBuffer[0];
			if (evi->mEventBuffer.size() < (unsigned)size) size = (short)evi->mEventBuffer.size();
			for (int i = 0; i < size; i++)
				rb[i] = tmpbuffer[i];
			evi->mTrapped = true;
			evi->mQueued = false;
		}
		catch (...) { }
	}
}

void EventsImplFb1::AttachDatabaseImpl(DatabaseImplFb1* database)
{
	if (database == 0) throw LogicExceptionImpl("EventsImplFb1::AttachDatabase",
			_("Can't attach a null Database object."));

	if (mDatabase != 0) mDatabase->DetachEventsImpl(this);
	mDatabase = database;
	mDatabase->AttachEventsImpl(this);
}

void EventsImplFb1::DetachDatabaseImpl()
{
	if (mDatabase == 0) return;

	mDatabase->DetachEventsImpl(this);
	mDatabase = 0;
}

EventsImplFb1::EventsImplFb1(DatabaseImplFb1* database)
{
	mDatabase = 0;
	mId = 0;
	mQueued = mTrapped = false;
	AttachDatabaseImpl(database);
}

EventsImplFb1::~EventsImplFb1()
{
	try { Clear(); }
		catch (...) { }

	try { if (mDatabase != 0) mDatabase->DetachEventsImpl(this); }
		catch (...) { }
}