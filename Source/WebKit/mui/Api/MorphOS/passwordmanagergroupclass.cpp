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

#include "config.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include "ResourceHandleManager.h"
#include "ResourceHandle.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clib/macros.h>

#include "ExtCredential.h"
#include "gui.h"

#define D(x)

using namespace WebCore;

#define PASSWORDDB "PROGDIR:Conf/Passwords.db"
static SQLiteDatabase m_passwordDB;

static HashMap<String, ExtCredential*> credentialMap;

struct Data
{
	Object *lv_credentials;
	ULONG loading;
};

DEFNEW
{
	Object *lv_credentials, *bt_remove, *bt_clear;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, lv_credentials = (Object *) NewObject(getpasswordmanagerlistclass(), NULL, TAG_DONE),
		Child, HGroup,
			Child, bt_remove = (Object *) MakeButton(GSI(MSG_PASSWORDMANAGERGROUP_REMOVE)),
			Child, bt_clear  = (Object *) MakeButton(GSI(MSG_PASSWORDMANAGERGROUP_REMOVE_ALL)),
		End,
		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;
		data->lv_credentials = lv_credentials;

		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_PasswordManagerGroup_Remove, NULL);
		DoMethod(bt_clear, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_PasswordManagerGroup_Clear);

		DoMethod(obj, MM_PasswordManagerGroup_Load);
	}

	return ((ULONG)obj);
}

DEFDISP
{
	HashMap<String, ExtCredential*>::iterator first = credentialMap.begin();
	HashMap<String, ExtCredential*>::iterator end = credentialMap.end();

	for (HashMap<String, ExtCredential*>::iterator credentialIterator = first; credentialIterator != end; ++credentialIterator)
		delete credentialIterator->value;

	return DOSUPER;
}

DEFTMETHOD(PasswordManagerGroup_Load)
{
	GETDATA;

	if(!m_passwordDB.isOpen() && !m_passwordDB.open(PASSWORDDB))
	{
		LOG_ERROR("Cannot open the passwords database");
		return 0;
    }

    // Check that the database is correctly initialized.
	if(!m_passwordDB.tableExists(String("passwords")))
	{
		m_passwordDB.close();
		return 0;
    }

	String request = "SELECT host, realm, username, password, usernameField, passwordField, type FROM passwords;";

	SQLiteStatement select(m_passwordDB, request);

	if(select.prepare())
	{
		LOG_ERROR("Cannot retrieve passwords in the database");
		return 0;
    }

	set(data->lv_credentials, MUIA_List_Quiet, TRUE);
	data->loading = TRUE;

	while(select.step() == SQLITE_ROW)
	{
		String host = select.getColumnText(0);
		String realm = select.getColumnText(1);
		String user = select.getColumnText(2);
		String password = select.getColumnText(3);
		String userField = select.getColumnText(4);
		String passwordField = select.getColumnText(5);
		int type  = select.getColumnInt(6) & CREDENTIAL_TYPE_MASK;
		int flags = select.getColumnInt(6) & CREDENTIAL_FLAG_MASK;

		if(type == CREDENTIAL_TYPE_AUTH)
		{
			ExtCredential credential(user, password, CredentialPersistencePermanent, realm, flags);
			DoMethod(obj, MM_PasswordManagerGroup_Insert, &host, &credential);
		}
		else if(type == CREDENTIAL_TYPE_FORM)
		{
			ExtCredential credential(user, password, CredentialPersistencePermanent, userField, passwordField, flags);
			DoMethod(obj, MM_PasswordManagerGroup_Insert, &host, &credential);
		}
	}

	set(data->lv_credentials, MUIA_List_Quiet, FALSE);
	data->loading = FALSE;

	return 0;
}

DEFTMETHOD(PasswordManagerGroup_Clear)
{
	GETDATA;

	/* Remove from list */
	DoMethod(data->lv_credentials, MUIM_List_Clear);

	/* Remove from map */
	HashMap<String, ExtCredential*>::iterator first = credentialMap.begin();
	HashMap<String, ExtCredential*>::iterator end = credentialMap.end();
	for (HashMap<String, ExtCredential*>::iterator credentialIterator = first; credentialIterator != end; ++credentialIterator)
		delete credentialIterator->value;

	credentialMap.clear();

	/* Remove from database */
	if(!m_passwordDB.isOpen() && !m_passwordDB.open(PASSWORDDB))
	{
		LOG_ERROR("Cannot open the passwords database");
		return 0;
    }

	SQLiteStatement deleteStmt(m_passwordDB, String("DELETE FROM passwords;"));

	if(deleteStmt.prepare())
	{
		return 0;
    }

	if(!deleteStmt.executeCommand())
	{
		LOG_ERROR("Cannot save passwords");
    }

	return 0;
}

DEFSMETHOD(PasswordManagerGroup_Insert)
{
	GETDATA;
	String *host  = (String *) msg->host;
	ExtCredential *credential = (ExtCredential *) msg->credential;

	/* Insertion in map and list */
	if(credential->persistence() == CredentialPersistencePermanent)
	{
		ExtCredential *newCredential = NULL;

		if(!(newCredential = credentialMap.get(*host)))
		{
			struct credential_entry *entry = (struct credential_entry *) malloc(sizeof(*entry));

			if(credential->type() == CREDENTIAL_TYPE_AUTH)
			{
				newCredential = new ExtCredential(credential->user(), credential->password(), credential->persistence(), credential->realm(), credential->flags());
			}
			else if(credential->type() == CREDENTIAL_TYPE_FORM)
			{
				newCredential = new ExtCredential(credential->user(), credential->password(), credential->persistence(), credential->userFieldName(), credential->passwordFieldName(), credential->flags());
			}

			if(newCredential)
			{
				credentialMap.add(*host, newCredential);

				if(entry)
				{
					entry->host  = strdup(host->utf8().data());
					entry->username = strdup(newCredential->user().utf8().data());
					entry->password = strdup(newCredential->password().utf8().data());

					entry->realm = NULL;
					entry->username_field = NULL;
					entry->password_field = NULL;
					entry->flags = newCredential->flags();

					if(newCredential->type() == CREDENTIAL_TYPE_AUTH)
					{
						entry->realm = strdup(newCredential->realm().utf8().data());
						entry->type = CREDENTIAL_TYPE_AUTH;
					}
					else if(newCredential->type() == CREDENTIAL_TYPE_FORM)
					{
						entry->username_field = strdup(newCredential->userFieldName().utf8().data());
						entry->password_field = strdup(newCredential->passwordFieldName().utf8().data());
						entry->type = CREDENTIAL_TYPE_FORM;
					}

					DoMethod(data->lv_credentials, MUIM_List_InsertSingle, entry, MUIV_List_Insert_Sorted);
				}
			}
		}
		else
		{
			struct credential_entry *entry;
			ULONG i;

			/*
			if(credential == credentialMap.get(*host))
			    return 0;
			*/

			newCredential = credentialMap.take(*host);
			delete newCredential;
			newCredential = NULL;

			if(credential->type() == CREDENTIAL_TYPE_AUTH)
			{
				newCredential = new ExtCredential(credential->user(), credential->password(), credential->persistence(), credential->realm(), credential->flags());
			}
			else if(credential->type() == CREDENTIAL_TYPE_FORM)
			{
				newCredential = new ExtCredential(credential->user(), credential->password(), credential->persistence(), credential->userFieldName(), credential->passwordFieldName(), credential->flags());
			}

			if(newCredential)
			{
				credentialMap.add(*host, newCredential);

				for (i = 0; ; i++)
				{
					DoMethod(data->lv_credentials, MUIM_List_GetEntry, i, &entry);

					if (!entry)
						break;

					if (*host == entry->host)
					{
						free(entry->realm);
						free(entry->username);
						free(entry->password);
						free(entry->username_field);
						free(entry->password_field);

						entry->username_field = NULL;
						entry->password_field = NULL;
						entry->realm = NULL;
                        entry->flags = newCredential->flags();

						entry->username = strdup(newCredential->user().utf8().data());
						entry->password = strdup(newCredential->password().utf8().data());

						if(newCredential->type() == CREDENTIAL_TYPE_AUTH)
						{
							entry->realm = strdup(newCredential->realm().utf8().data());
							entry->type = CREDENTIAL_TYPE_AUTH;
						}
						else if(newCredential->type() == CREDENTIAL_TYPE_FORM)
						{
							entry->username_field = strdup(newCredential->userFieldName().utf8().data());
							entry->password_field = strdup(newCredential->passwordFieldName().utf8().data());
							entry->type = CREDENTIAL_TYPE_FORM;
						}

						DoMethod(data->lv_credentials, MUIM_List_Redraw, MUIV_List_Redraw_Entry, entry);

						break;
					}
				}
			}
		}

		/* Insertion in database */
		if(!data->loading)
		{
			if(!m_passwordDB.isOpen() && !m_passwordDB.open(PASSWORDDB))
			{
				LOG_ERROR("Cannot open the passwords database");
				return 0;
		    }

			if(!m_passwordDB.tableExists(String("passwords")))
			{
				m_passwordDB.executeCommand(String("CREATE TABLE passwords (host TEXT, realm TEXT, username TEXT, password TEXT, usernameField TEXT, passwordField TEXT, type INTEGER);"));
			}

			SQLiteStatement deleteStmt(m_passwordDB, String("DELETE FROM passwords WHERE host=?1;"));

			if(deleteStmt.prepare())
			{
				return false;
		    }

		    // Binds all the values
			if(deleteStmt.bindText(1, *host))
			{
				LOG_ERROR("Cannot save passwords");
		    }

			if(!deleteStmt.executeCommand()) {
				LOG_ERROR("Cannot save passwords");
		    }

			SQLiteStatement insert(m_passwordDB, String("INSERT INTO passwords (host, realm, username, password, usernameField, passwordField, type) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);"));

			if(insert.prepare())
			{
				return 0;
		    }

		    // Binds all the values
			if(insert.bindText(1, *host) || insert.bindText(2, newCredential->realm())
			|| insert.bindText(3, newCredential->user()) || insert.bindText(4, newCredential->password())
			|| insert.bindText(5, newCredential->userFieldName()) || insert.bindText(6, newCredential->passwordFieldName())
			|| insert.bindInt64(7, newCredential->type() | newCredential->flags())
			)
			{
				LOG_ERROR("Cannot save passwords");
				return 0;
		    }

			if(!insert.executeCommand()) {
				LOG_ERROR("Cannot save passwords");
				return 0;
		    }
		}
	}

	return 0;
}

DEFSMETHOD(PasswordManagerGroup_Remove)
{
	GETDATA;
	String host;
	struct credential_entry *entry;
	ULONG i;

	/* Remove from list */
	if(msg->host == NULL)
	{
		DoMethod(data->lv_credentials, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct credential_entry *) &entry);
		
		if(entry)
		{
			host = entry->host;
			DoMethod(data->lv_credentials, MUIM_List_Remove, MUIV_List_Remove_Active);
		}
	}
	else
	{
		host = *((String *)msg->host);

		for (i = 0; ; i++)
		{
			DoMethod(data->lv_credentials, MUIM_List_GetEntry, i, &entry);

			if (!entry)
				break;

			if (host == entry->host)
			{
				DoMethod(data->lv_credentials, MUIM_List_Remove, i);
				break;
			}
		}
	}

	/* Remove from map */
	ExtCredential *credential = credentialMap.take(host);
	delete credential;

	/* Remove from database */
	if(!m_passwordDB.isOpen() && !m_passwordDB.open(PASSWORDDB))
	{
		LOG_ERROR("Cannot open the passwords database");
		return 0;
    }

	SQLiteStatement deleteStmt(m_passwordDB, String("DELETE FROM passwords where host=?1;"));

	if(deleteStmt.prepare())
	{
		return 0;
    }

	if(deleteStmt.bindText(1, host))
	{
		LOG_ERROR("Cannot save passwords");
		return 0;
    }

	if(!deleteStmt.executeCommand()) {
		LOG_ERROR("Cannot save passwords");
		return 0;
    }

	return 0;
}

DEFSMETHOD(PasswordManagerGroup_Get)
{
	String *host = (String *) msg->host;
	ExtCredential *credential = credentialMap.get(*host);

	return (ULONG) credential;
}


BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(PasswordManagerGroup_Load)
DECTMETHOD(PasswordManagerGroup_Clear)
DECSMETHOD(PasswordManagerGroup_Insert)
DECSMETHOD(PasswordManagerGroup_Remove)
DECSMETHOD(PasswordManagerGroup_Get)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, passwordmanagergroupclass)
