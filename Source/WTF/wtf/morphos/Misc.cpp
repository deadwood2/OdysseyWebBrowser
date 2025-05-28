#include "config.h"
#include <exec/types.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/system.h>
#include <proto/exec.h>
#include "Platform.h"

extern "C" void Fail(unsigned char *)
{
	// Libjpeg fail message...
	// TODO
}

extern "C" char *realpath(const char *file_name, char *resolved_name)
{
	BPTR l = Lock(file_name, ACCESS_READ);
	if (l)
	{
		if (NameFromLock(l, resolved_name, PATH_MAX))
		{
			UnLock(l);
			return resolved_name;
		}
		UnLock(l);
	}

	return nullptr;
}

namespace WTF {

bool HasAltivec::m_hasAltivec;

HasAltivec::HasAltivec()
{
	LONG altivec = 0;
	NewGetSystemAttrsA(&altivec, sizeof(altivec), SYSTEMINFOTYPE_PPC_ALTIVEC, NULL);
	m_hasAltivec = altivec == 1;
}

bool HasAltivec::hasAltivec()
{
	static HasAltivec __hs;
	return m_hasAltivec;
}

}
