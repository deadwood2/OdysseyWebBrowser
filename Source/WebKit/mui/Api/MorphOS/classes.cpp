/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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

#include <stdio.h>

#include "include/macros/vapor.h"
#include "classes.h"

/* Classes management */

struct classdesc {
	char * name;
	APTR initfunc;
	APTR cleanupfunc;
};

#define CLASSENT(s) {#s, (APTR) create_##s##class, (APTR) delete_##s##class}

/* classes declaration */

static const struct classdesc cd[] = {
	CLASSENT(owbapp),
	CLASSENT(owbwindow),
	CLASSENT(owbgroup),
	CLASSENT(owbbrowser),

	CLASSENT(navigationgroup),
	CLASSENT(addressbargroup),
	CLASSENT(searchbargroup),
	CLASSENT(findtext),
	CLASSENT(downloadwindow),
	CLASSENT(downloadgroup),
	CLASSENT(downloadlist),
	CLASSENT(prefswindow),
	CLASSENT(toolbutton),
	CLASSENT(transferanim),
	CLASSENT(tabtransferanim),
	CLASSENT(popstring),
	CLASSENT(historypopstring),
//	  CLASSENT(fontfamilypopstring),
	CLASSENT(historylist),
	CLASSENT(title),
	CLASSENT(titlelabel),
	CLASSENT(menu),
	CLASSENT(menuitem),
	CLASSENT(bookmarkwindow),
	CLASSENT(bookmarkgroup),
	CLASSENT(bookmarklisttree),
	CLASSENT(linklist),
	CLASSENT(quicklinkgroup),
	CLASSENT(quicklinkbuttongroup),
	CLASSENT(quicklinkparentgroup),
	CLASSENT(historybutton),
	CLASSENT(networkwindow),
	CLASSENT(networklist),
	CLASSENT(networkledsgroup),
	CLASSENT(splashwindow),
	CLASSENT(loginwindow),
	CLASSENT(consolewindow),
	CLASSENT(consolelist),
	CLASSENT(bookmarkpanelgroup),
	CLASSENT(contextmenugroup),
	CLASSENT(contextmenulist),
	CLASSENT(mimetypegroup),
	CLASSENT(mimetypelist),
	CLASSENT(choosetitlegroup),
	CLASSENT(toolbutton_newtab),
	CLASSENT(toolbutton_addbookmark),
	CLASSENT(toolbutton_bookmarks),
	CLASSENT(urlstring),
	CLASSENT(favicon),
	CLASSENT(icon),
//	  CLASSENT(historywindow),
	CLASSENT(historypanelgroup),
	CLASSENT(historylisttree),
	CLASSENT(passwordmanagerwindow),
	CLASSENT(passwordmanagergroup),
	CLASSENT(passwordmanagerlist),
	CLASSENT(cookiemanagerwindow),
	CLASSENT(cookiemanagergroup),
	CLASSENT(cookiemanagerlisttree),
	CLASSENT(blockmanagerwindow),
	CLASSENT(blockmanagergroup),
	CLASSENT(blockmanagerlist),
	CLASSENT(searchmanagerwindow),
	CLASSENT(searchmanagergroup),
	CLASSENT(searchmanagerlist),
	CLASSENT(scriptmanagerwindow),
	CLASSENT(scriptmanagergroup),
	CLASSENT(scriptmanagerlist),
	CLASSENT(scriptmanagerhostlist),
	CLASSENT(urlprefswindow),
	CLASSENT(urlprefsgroup),
	CLASSENT(urlprefslist),
	CLASSENT(mediacontrolsgroup),
	CLASSENT(seekslider),
	CLASSENT(volumeslider),
	CLASSENT(spacer),
	CLASSENT(suggestlist),
	CLASSENT(suggestpopstring),
	CLASSENT(printerwindow),
	CLASSENT(autofillpopup),
	CLASSENT(autofillpopuplist),
	CLASSENT(colorchooserpopup),
	CLASSENT(datetimechooserpopup),

	{ 0, 0, 0 }
};

ULONG classes_init(void)
{
	ULONG i;

	for (i = 0; cd[i].name; i++)
	{
		if (!(*(int(*)(void))cd[i].initfunc)())
		{
			fprintf(stderr, "Couldn't create class %s.\n", cd[i].name);
			return (FALSE);
		}
	}
	return (TRUE);
}

void classes_cleanup(void)
{
	LONG i;

	for (i = sizeof(cd) / sizeof(struct classdesc) - 2; i >= 0; i--)
	{
		(*(void(*)(void))cd[i].cleanupfunc)();
	}
}

