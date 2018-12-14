/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#ifndef ExtCredential_h
#define ExtCredential_h

#include "Credential.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

enum { CREDENTIAL_TYPE_AUTH, CREDENTIAL_TYPE_FORM };
enum { CREDENTIAL_FLAG_BLACKLISTED = 0x10000 };

#define CREDENTIAL_TYPE_MASK 0x0000FFFF
#define CREDENTIAL_FLAG_MASK 0xFFFF0000
    
class ExtCredential {

public:
	ExtCredential()
    : m_user("")
    , m_password("")
	, m_realm("")
	, m_userFieldName("")
	, m_passwordFieldName("")
	, m_type(CREDENTIAL_TYPE_AUTH)
	, m_flags(0)
	{
	}

	ExtCredential(const String& user, const String& password, CredentialPersistence persistence, const String& realm, int flags)
	: m_user(user.length() ? user : "")
    , m_password(password.length() ? password : "")
    , m_persistence(persistence)
	, m_realm(realm.length() ? realm : "")
	, m_userFieldName("")
	, m_passwordFieldName("")
	, m_type(CREDENTIAL_TYPE_AUTH)
	, m_flags(flags)
	{	 
	}

	ExtCredential(const String& user, const String& password, CredentialPersistence persistence, const String& userFieldName, const String &passwordFieldName, int flags)
	: m_user(user.length() ? user : "")
    , m_password(password.length() ? password : "")
    , m_persistence(persistence)
	, m_realm("")
	, m_userFieldName(userFieldName.length() ? userFieldName : "")
	, m_passwordFieldName(passwordFieldName.length() ? passwordFieldName : "")
	, m_type(CREDENTIAL_TYPE_FORM)
	, m_flags(flags)
	{
	}

	bool isEmpty() { return m_user.isEmpty() && m_password.isEmpty(); }

	const String& user() const { return m_user; }
	const String& password() const { return m_password; }

	bool hasPassword() const { return !m_password.isEmpty(); }
	CredentialPersistence persistence() const { return m_persistence; }
	const String& realm() const { return m_realm; }
	const String& userFieldName() const { return m_userFieldName; }
	const String& passwordFieldName() const { return m_passwordFieldName; }
	const int& type() const { return m_type; }
	const int& flags() const { return m_flags; }

	void clear() { m_user = ""; m_password = ""; m_realm = ""; m_userFieldName = ""; m_passwordFieldName = ""; }
	void setFlags(int flags) { m_flags = flags; };
    
private:
	String m_user;
	String m_password;
    CredentialPersistence m_persistence;
	String m_realm;
	String m_userFieldName;
	String m_passwordFieldName;
	int    m_type;
	int    m_flags;
};

inline bool operator==(const ExtCredential& a, const ExtCredential& b)
{
    if (a.user() != b.user())
        return false;
    if (a.password() != b.password())
        return false;
    if (a.persistence() != b.persistence())
        return false;
	if (a.realm() != b.realm())
        return false;
	if (a.userFieldName() != b.userFieldName())
        return false;
	if (a.passwordFieldName() != b.passwordFieldName())
        return false;
	if (a.type() != b.type())
        return false;
	if (a.flags() != b.flags())
		return false;

    return true;
}
inline bool operator!=(const ExtCredential& a, const ExtCredential& b) { return !(a == b); }
    
};
#endif
