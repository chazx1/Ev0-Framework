//===== Copyright c 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IMATCHFRAMEWORK_H
#define IMATCHFRAMEWORK_H

#ifdef _WIN32
#pragma once
#endif


#include "../sdk.hpp"
#include "../misc/keyvalues.h"


#include "imatchasync.h"
#include "imatchtitle.h"
#include "imatchnetworkmsg.h"
#include "imatchextensions.h"
#include "imatchevents.h"
#include "imatchsystem.h"
#include "iplayermanager.h"
#include "iplayer.h"
#include "iservermanager.h"
#include "imatchvoice.h"
#include "isearchmanager.h"
#include "idatacenter.h"

	class IMatchFramework;
	class IMatchSession;

	class IMatchFramework
	{
	public:
		IMatchEventsSubscription* GetEventsSubscription()
		{
			typedef IMatchEventsSubscription*(__thiscall* tGetEventsSubscription)(void*);
			return CallVFunction<tGetEventsSubscription>(this, 11)(this);
		}

		IMatchSession * GetMatchSession() {
			typedef IMatchSession *(__thiscall* OriginalFn)(void*);
			return CallVFunction<OriginalFn>(this, 13)(this);
		}

		// Get the network msg encode/decode factory
		//virtual IMatchNetworkMsgController * GetMatchNetworkMsgController() = 0;

		// Get the match system
		virtual IMatchSystem * GetMatchSystem() {
			typedef  IMatchSystem*(__thiscall* OriginalFn)(void*);
			return CallVFunction<OriginalFn>(this, 15)(this);
		}

	};

#define IMATCHFRAMEWORK_VERSION_STRING "MATCHFRAMEWORK_001"



	class IMatchSession
	{
	public:
		// Get an internal pointer to session system-specific data
		virtual KeyValues * GetSessionSystemData() = 0;

		// Get an internal pointer to session settings
		virtual KeyValues * GetSessionSettings() = 0;

		// Update session settings, only changing keys and values need
		// to be passed and they will be updated
		virtual void UpdateSessionSettings(KeyValues *pSettings) = 0;

		// Issue a session command
		virtual void Command(KeyValues *pCommand) = 0;
	};

#endif // IMATCHFRAMEWORK_H
