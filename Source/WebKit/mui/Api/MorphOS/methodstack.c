/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
 * Copyright 2009 Fabien Coeurjoly
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include "gui.h"

#define D(x)

STATIC struct SignalSemaphore semaphore;
STATIC struct Task *mstask;
STATIC struct MinList methodlist = { (APTR)&methodlist.mlh_Tail, NULL, { (APTR)&methodlist.mlh_Head } };
STATIC int is_quitting = FALSE;
STATIC int is_safe_to_quit = TRUE;

struct pushedmethod
{
	struct MinNode n;
	struct Message msg;
	ULONG size;
	APTR obj;
	ULONG sync;
	ULONG result;
	ULONG m[0];
};

struct pushedmessage
{
	struct Message msg;
	ULONG size;
	APTR obj;
	ULONG sync;
	ULONG result;
	ULONG m[0];
};

void WakeTimer(void)
{
	Signal(mstask, SIGBREAKF_CTRL_E);
}

void setIsQuitting(int value)
{
	is_quitting = value;
}

int isQuitting(void)
{
	return is_quitting;
}

void setIsSafeToQuit(int value)
{
	is_safe_to_quit = value;
}

int isSafeToQuit(void)
{
	return is_safe_to_quit;
}

void methodstack_init(void)
{
	InitSemaphore(&semaphore);
    mstask = FindTask(NULL);
}

void *getMainTask(void)
{
	return mstask;
}

void methodstack_cleanup(void)
{
	struct pushedmethod *pm, *next;

	methodstack_cleanup_flush();

	ITERATELISTSAFE(pm, next, &methodlist)
	{
		FreeMem(pm, pm->size);
	}

	mstask = NULL;
}

void methodstack_cleanup_flush(void)
{
	struct pushedmethod *pm;

	ObtainSemaphore(&semaphore);

	while ((pm = REMHEAD(&methodlist)))
	{
		if (pm->sync)
		{
			pm->result = -1;
			ReplyMsg(&pm->msg);
		}
		else
		{
			FreeMem(pm, pm->size);
		}
	}

	ReleaseSemaphore(&semaphore);
}

void methodstack_check(void)
{
	ULONG not_empty = TRUE;

	//D(kprintf("[MethodStack] methodstack_check()\n"));

	do
	{
		struct pushedmethod *pm;

		ObtainSemaphore(&semaphore);
		pm = REMHEAD(&methodlist);
		not_empty = !ISLISTEMPTY(&methodlist);
		ReleaseSemaphore(&semaphore);

		if (!pm)
			break;

		if (pm->sync)
		{
			if (pm->obj)
			{
				pm->result = DoMethodA(pm->obj, (Msg)&pm->m[0]);
			}
			else
			{
				pm->result = -1;
			}

			ReplyMsg(&pm->msg);
		}
		else
		{
			DoMethodA(pm->obj, (Msg)&pm->m[0]);
			FreeMem(pm, pm->size);
		}
	}
	while (not_empty);
}

/*
 * Pushes a method asynchronously just like MUI, except
 * we have more control and there's no stupid static limit.
 */
void methodstack_push(APTR obj, ULONG cnt, ...)
{
	struct pushedmethod *pm;
	ULONG size;
	va_list va;

	va_start(va, cnt);
	size = sizeof(*pm) + cnt * sizeof(ULONG);

	if ((pm = AllocMem(size, MEMF_ANY)))
	{
		ULONG i = 0;
		pm->obj = obj;
		pm->size = size;
		pm->sync = 0;

		while (cnt--)
		{
			pm->m[i] = va_arg(va, ULONG);
			i++;
		}

		if (SysBase->ThisTask == (APTR)mstask)
		{
			methodstack_check();
			DoMethodA(obj, (Msg)&pm->m[0]);
			FreeMem(pm, size);
		}
		else
		{
			ObtainSemaphore(&semaphore);
			AddTail((struct List *)&methodlist, (struct Node *)pm);
			ReleaseSemaphore(&semaphore);
		}
	}

	va_end(va);
}

/*
 * Pushes a method synchronously. Completes all the
 * waiting methods in the stack before ourself. Can
 * be used to synchronise previous pushmethods and
 * get greater efficiency by minimizing context switches.
 */


ULONG methodstack_push_sync(APTR obj, ULONG cnt, ...)
{
	struct pushedmethod *pm;
	ULONG res, size;
	va_list va;

	va_start(va, cnt);
	size = sizeof(*pm) + cnt * sizeof(ULONG);
	res = 0;

	if ((pm = AllocMem(size, MEMF_ANY)))
	{
		struct Process *thisproc = (APTR)SysBase->ThisTask;
		struct MsgPort *replyport = NULL;
		ULONG i = 0;

		while (cnt--)
		{
			pm->m[i] = va_arg(va, ULONG);
			i++;
		}

		if (thisproc == (APTR)mstask)
		{
			methodstack_check();
			res = DoMethodA(obj, (Msg)&pm->m[0]);

			FreeMem(pm, size);
			va_end(va);

			return res;
		}

		replyport = CreateMsgPort();

		pm->size = size;
		pm->obj = obj;
		pm->sync = TRUE;
		pm->msg.mn_ReplyPort = replyport;

		ObtainSemaphore(&semaphore);
		AddTail((struct List *)&methodlist, (struct Node *)pm);
		ReleaseSemaphore(&semaphore);

		Signal(mstask, SIGBREAKF_CTRL_F);
		WaitPort(replyport);
		GetMsg(replyport);

		res = pm->result;
		FreeMem(pm, pm->size);
	}

	va_end(va);

	return res;
}
