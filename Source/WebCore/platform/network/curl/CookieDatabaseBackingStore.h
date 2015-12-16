/*
 * Copyright (C) 2009 Julien Chaffraix <jchaffraix@pleyo.com>
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CookieDatabaseBackingStore_h
#define CookieDatabaseBackingStore_h

#include "SQLiteDatabase.h"
#include "Timer.h"

#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ParsedCookie;

class CookieDatabaseBackingStore {
public:
    static CookieDatabaseBackingStore* create() { return new CookieDatabaseBackingStore; }

    void open(const String& cookieJar);
	void close();

    void insert(const ParsedCookie*);
    void update(const ParsedCookie*);
    void remove(const ParsedCookie*);

    void removeAll();

    // If a limit is not set, the method will return all cookies in the database
    void getCookiesFromDatabase(Vector<ParsedCookie*>& stackOfCookies, unsigned int limit = 0);

	void sendChangesToDatabase();

private:
    enum UpdateParameter {
        Insert,
        Update,
        Delete,
    };

    CookieDatabaseBackingStore();
    ~CookieDatabaseBackingStore();

    void addToChangeQueue(const ParsedCookie* changedCookie, UpdateParameter actionParam);

    typedef std::pair<const ParsedCookie, UpdateParameter> CookieAction;
    Vector<CookieAction> m_changedCookies;

    String m_tableName;
    SQLiteDatabase m_db;
    SQLiteStatement *m_insertStatement;
    SQLiteStatement *m_updateStatement;
    SQLiteStatement *m_deleteStatement;
};

CookieDatabaseBackingStore& cookieBackingStore();

} // namespace WebCore

#endif // CookieDatabaseBackingStore_h
