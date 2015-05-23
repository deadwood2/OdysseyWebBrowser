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
#include <wtf/text/CString.h>
#include "Logging.h"
#include "SearchPopupMenuMorphOS.h"

#include <cstdio>

namespace WebCore {

  SearchPopupMenuMorphOS::SearchPopupMenuMorphOS(PopupMenuClient* client)
	: m_popup(adoptRef(new PopupMenuMorphOS(client)))
  {
    fprintf(stderr, "SearchPopupMenu::SearchPopupMenu\n");
  }

  PopupMenu* SearchPopupMenuMorphOS::popupMenu()
  {
    return m_popup.get();
  }

  void SearchPopupMenuMorphOS::saveRecentSearches(const AtomicString& name, const Vector<String>& searchItems)
  {
    UNUSED_PARAM(searchItems);
    fprintf(stderr, "SearchPopupMenu::saveRecentSearches '%s'\n", name.string().latin1().data());
  }

  void SearchPopupMenuMorphOS::loadRecentSearches(const AtomicString& name, Vector<String>& searchItems)
  {
    UNUSED_PARAM(searchItems);
    fprintf(stderr, "SearchPopupMenu::loadRecentSearches '%s'\n", name.string().latin1().data());
  }

  bool SearchPopupMenuMorphOS::enabled()
  {
    fprintf(stderr, "SearchPopupMenu::enabled\n");
    return true;
  }

}          
