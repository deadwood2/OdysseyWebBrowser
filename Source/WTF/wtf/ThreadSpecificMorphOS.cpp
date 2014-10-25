/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Jian Li <jianli@chromium.org>
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Thread local storage is implemented by using either pthread API or Windows
 * native API. There is subtle semantic discrepancy for the cleanup function
 * implementation as noted below:
 *   @ In pthread implementation, the destructor function will be called
 *     repeatedly if there is still non-NULL value associated with the function.
 *   @ In Windows native implementation, the destructor function will be called
 *     only once.
 * This semantic discrepancy does not impose any problem because nowhere in
 * WebKit the repeated call bahavior is utilized.
 */

#include "config.h"
#include "ThreadSpecific.h"

#include <proto/exec.h>
#include <clib/debug_protos.h>

namespace WTF {

#ifndef REMOVE
#define REMOVE(n) Remove((struct Node*)n)
#endif

#ifndef ADDTAIL
#define ADDTAIL(l,n) AddTail((struct List*)l,(struct Node*)n)
#endif

#ifndef NEWLIST
#define NEWLIST(l) NewList((struct List*)l)
#endif

/*
struct ThreadSpecificNode
{
	struct MinNode n;
	void (*destructor)(void *);
	void *value;
};
*/
void threadSpecificKeyCreate(ThreadSpecificKey* key, void (*destructor)(void *))
{
	struct Task *t = FindTask(NULL);

	if(t->tc_UserData == NULL)
	{
		t->tc_UserData = (struct MinList *) malloc(sizeof(struct MinList));

		if(t->tc_UserData)
		{
			NEWLIST(t->tc_UserData);
		}
	}

	if(t->tc_UserData)
	{
		*key = (ThreadSpecificKey) malloc(sizeof(struct ThreadSpecificNode));

		if(*key)
		{
			(*key)->value = NULL;
			(*key)->destructor = destructor;

			ADDTAIL(t->tc_UserData, (*key));
		}
	}
}

void threadSpecificKeyDelete(ThreadSpecificKey key)
{
	struct Task *t = FindTask(NULL);

	if(t->tc_UserData)
	{
		if(key->destructor)
		{
			key->destructor(key->value);
		}
		REMOVE(key);
		free(key);
	}
}

void threadSpecificSet(ThreadSpecificKey key, void* value)
{
	key->value = value;
}

void* threadSpecificGet(ThreadSpecificKey key)
{
	return key->value;
}

} // namespace WTF

