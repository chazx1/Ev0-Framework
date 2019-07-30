#ifndef _IDATACENTER_H_
#define _IDATACENTER_H_



#include "imatchsystem.h"
	class IDatacenter;
	class IDatacenter
	{
	public:
		//
		// GetStats
		//	retrieves the last received datacenter stats
		//
		virtual KeyValues * GetStats() = 0;
	};


#endif // _IDATACENTER_H_
