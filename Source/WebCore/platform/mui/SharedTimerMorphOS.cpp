/*
 * Copyright (C) 2009-2013 Fabien Coeurjoly.
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedTimer.h"

#include <wtf/CurrentTime.h>
#include <wtf/Assertions.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "BALBase.h"

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/dos.h>   
#include <proto/intuition.h>
#include <clib/macros.h>
#include <clib/debug_protos.h>
#include <dos/dostags.h>

#include <unistd.h>

#define D(x)

namespace WebCore {

void (*sharedTimerFiredFunction)() = NULL;

// Single timer, shared to implement all the timers managed by the Timer class.
// Not intended to be used directly; use the Timer class instead.

struct TimerHandle
{
	struct Task *CallerTask;
	struct Task *TimerTask;
	BYTE TaskSigBit;

	BYTE StartSigBit;
	BYTE StopSigBit;
	BYTE TermSigBit;

	ULONG Interval;
};

static TimerHandle *timer_handle;

void fireTimerIfNeeded()
{
  D(kprintf("[SharedTimer] fireTimerIfNeeded()\n"));
  if(sharedTimerFiredFunction)
  {
	  sharedTimerFiredFunction();
  }
}

static void TimerFunc(void)
{
#if !OS(AROS)
	struct ExecBase *SysBase;
#endif
	struct Task *CurrentTask;
	struct TimerHandle *Handle;
	struct MsgPort * TimerMP;
	ULONG TimerSignal;
	struct timerequest TimerIO;

#if !OS(AROS)
	SysBase = *((struct ExecBase **) 4);
#endif

	CurrentTask = FindTask(NULL);

	Handle = (struct TimerHandle *) CurrentTask->tc_UserData;

	Handle->TermSigBit = -1;

	TimerMP = CreateMsgPort();

	D(kprintf("[SharedTimer] Entering Timer thread\n"));

	if(TimerMP)
	{
		TimerSignal = 1 << TimerMP->mp_SigBit;

		TimerIO.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
		TimerIO.tr_node.io_Message.mn_ReplyPort    = TimerMP;
		TimerIO.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);

		if (!OpenDevice("timer.device", UNIT_MICROHZ, &TimerIO.tr_node, 0))
		{
			BOOL TimerPending = FALSE;

			Handle->StopSigBit = -1;

			if (((Handle->StartSigBit = AllocSignal(-1)) != -1) &&
			    ((Handle->StopSigBit  = AllocSignal(-1)) != -1) &&
			    ((Handle->TermSigBit  = AllocSignal(-1)) != -1))
			{
				ULONG StartSignal, StopSignal, TermSignal;
				ULONG Signals;

				/* Signal the parent we are ready */
				D(kprintf("[SharedTimer Thread] Signal main task that Timer thread is ready to accept orders\n"));
				Signal(Handle->CallerTask, 1 << Handle->TaskSigBit);

				StartSignal = 1 << Handle->StartSigBit;
				StopSignal  = 1 << Handle->StopSigBit;
				TermSignal  = 1 << Handle->TermSigBit;

				do
				{
					D(kprintf("[SharedTimer Thread] Wait for start or end signal\n"));
					Signals = Wait(StartSignal | TermSignal);
restart:
					if (Signals & StartSignal)
					{
						D(kprintf("[SharedTimer Thread] Received start signal\n"));

						D(kprintf("[SharedTimer Thread] Adding timerequest for %d µs\n", Handle->Interval));

						TimerIO.tr_node.io_Command = TR_ADDREQUEST;
						TimerIO.tr_time.tv_secs    = Handle->Interval / 1000000;
						TimerIO.tr_time.tv_micro   = Handle->Interval % 1000000;

						SendIO(&TimerIO.tr_node);

						TimerPending = TRUE;

						while(1)
						{
							Signals = Wait(TimerSignal | StartSignal | StopSignal | TermSignal);
							if (Signals & TermSignal)
							{
								D(kprintf("[SharedTimer Thread] Termination signal received\n"));
								break;
							}
							else if(Signals & StopSignal)
							{
								D(kprintf("[SharedTimer Thread] Stop signal with pending request received\n"));
								AbortIO(&TimerIO.tr_node);
								WaitIO(&TimerIO.tr_node);
                                TimerPending = FALSE;

								if (sharedTimerFiredFunction)
								{
									D(kprintf("[SharedTimer Thread] Signaling main task 0x%p\n", Handle->CallerTask));
									Signal(Handle->CallerTask, SIGBREAKF_CTRL_E);
								}

								// avoid spamming too much
								TimerIO.tr_node.io_Command = TR_ADDREQUEST;
								TimerIO.tr_time.tv_secs    = 0;
								TimerIO.tr_time.tv_micro   = 2500;
								DoIO(&TimerIO.tr_node);
								break;
							}
							else if(Signals & StartSignal)
							{
								D(kprintf("[SharedTimer Thread] Start signal with pending request received, restart\n"));
								AbortIO(&TimerIO.tr_node);
								WaitIO(&TimerIO.tr_node);
                                TimerPending = FALSE;

								// avoid spamming too much
								//if(Handle->Interval < 5000)
								{
									TimerIO.tr_node.io_Command = TR_ADDREQUEST;
									TimerIO.tr_time.tv_secs    = 0;
									TimerIO.tr_time.tv_micro   = 2500;
									DoIO(&TimerIO.tr_node);
								}

								goto restart;
							}
							else if((Signals & TimerSignal) && CheckIO(&TimerIO.tr_node))
							{
								D(kprintf("[SharedTimer Thread] Timersignal and IO completed\n"));
                                TimerPending = FALSE;
									
								if (sharedTimerFiredFunction)
								{
									D(kprintf("[SharedTimer Thread] Signaling main task 0x%p\n", Handle->CallerTask));
									Signal(Handle->CallerTask, SIGBREAKF_CTRL_E);
								}	 
								break;
							}
						}
					}
				} while (!(Signals & TermSignal));
			} /* AllocSignal */

			D(kprintf("[SharedTimer Thread] Freeing signals\n"));

			FreeSignal(Handle->StartSigBit);
			FreeSignal(Handle->StopSigBit);
			FreeSignal(Handle->TermSigBit);

			if (TimerPending)
			{
				AbortIO(&TimerIO.tr_node);
				WaitIO(&TimerIO.tr_node);
			}

			CloseDevice(&TimerIO.tr_node);
		}

		DeleteMsgPort(TimerMP);
	}

	D(kprintf("[SharedTimer Thread] Signaling end of thread to main task\n"));
	Forbid();
	Signal(Handle->CallerTask, 1 << Handle->TaskSigBit);
}

struct TimerHandle * TimerCreate(void)
{
	struct TimerHandle *Handle;

	Handle = (struct TimerHandle *) AllocVec(sizeof(struct TimerHandle), MEMF_CLEAR | MEMF_PUBLIC);
	if (Handle)
	{
		Handle->CallerTask = FindTask(NULL);
		Handle->TaskSigBit = AllocSignal(-1);

		if (Handle->TaskSigBit != -1)
		{
#if !OS(AROS)
			Handle->TimerTask = NewCreateTask(TASKTAG_CODETYPE,  CODETYPE_PPC,
										TASKTAG_PC,        (ULONG) &TimerFunc,
										TASKTAG_NAME,      (ULONG) "[OWB] Timer",
                                        TASKTAG_STACKSIZE, 4096,
										TASKTAG_PRI,       0,
										TASKTAG_USERDATA,  (ULONG) Handle,
                                        TAG_DONE);
#else
            struct TagItem tags[] =
            {
                    { TASKTAG_PC,       (IPTR) &TimerFunc },
                    { TASKTAG_NAME,     (IPTR) "[OWB] Timer" },
                    { TASKTAG_STACKSIZE,4096 },
                    { TASKTAG_PRI,      0 },
                    { TASKTAG_USERDATA, (IPTR) Handle },
                    { TAG_DONE, TAG_DONE }
            };
            Handle->TimerTask = NewCreateTaskA(tags);
#endif
			if (Handle->TimerTask)
			{
				Wait(1 << Handle->TaskSigBit);

				/* If subtask's init succeeded */
				if (Handle->TermSigBit != -1)
				{
					return Handle;
				}
			}

			FreeSignal(Handle->TaskSigBit);
		}

		FreeVec(Handle);
	}

	return NULL;
}

void TimerStart(struct TimerHandle *Handle, ULONG Interval)
{
	if (Handle)
	{
		Handle->Interval = Interval;

		Signal(Handle->TimerTask, 1 << Handle->StartSigBit);
	}
}


void TimerStop(struct TimerHandle *Handle)
{
	if (Handle)
	{
		Signal(Handle->TimerTask, 1 << Handle->StopSigBit);
		//Wait(1 << Handle->TaskSigBit);
	}
}

void TimerDelete(struct TimerHandle *Handle)
{
	if (Handle)
	{
		D(kprintf("[SharedTimer] Signaling Timer thread to quit\n"));
		Signal(Handle->TimerTask, 1 << Handle->TermSigBit);

		D(kprintf("[SharedTimer] Waiting for Timer thread end\n"));
		Wait(1 << Handle->TaskSigBit);

		FreeSignal(Handle->TaskSigBit);

		FreeVec(Handle);
	}
}

static void cleanup(void)
{
	D(kprintf("[SharedTimer] Cleanup\n"));
	TimerDelete(timer_handle);
	D(kprintf("[SharedTimer] Done\n"));
}

void setSharedTimerFiredFunction(void (*f)()) 
{
	//kprintf("[SharedTimer] setSharedTimerFiredFunction(%p)\n", f);
	if ( NULL == sharedTimerFiredFunction )
	{
        sharedTimerFiredFunction = f;

		timer_handle = TimerCreate();

		if(timer_handle)
		{
			atexit(cleanup);
			return;
		}
		fprintf(stderr, "couldn't create timer.\n");
    }
}

void setSharedTimerFireInterval(double interval) 
{
    assert(sharedTimerFiredFunction);

    //kprintf("[SharedTimer] setSharedTimerFireInterval(%f)\n", interval);
    unsigned long intervalInUS = static_cast<unsigned long>(interval * 1000 * 1000);
    if (intervalInUS < 1000) // min. time 1000 usec.
        intervalInUS = 1000;

// check if we could call it anyway
//  stopSharedTimer();

    TimerStart(timer_handle, intervalInUS);
}

void stopSharedTimer() 
{
	//kprintf("[SharedTimer] stopSharedTimer()\n");
	TimerStop(timer_handle);
}

void invalidateSharedTimer()
{
}

}
