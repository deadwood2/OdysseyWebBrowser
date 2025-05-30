/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "AutofillBackingStore.h"

#include <wtf/FileSystem.h>
#include <WebCore/SQLiteStatement.h>

#define HANDLE_SQL_EXEC_FAILURE(statement, returnValue, ...) \
    if (statement) { \
        LOG_ERROR(__VA_ARGS__); \
        return returnValue; \
    }

namespace WebCore {

AutofillBackingStore& autofillBackingStore()
{
    static NeverDestroyed<AutofillBackingStore> backingStore;

    if (!backingStore.get().m_database.isOpen())
        backingStore.get().open("PROGDIR:Conf/AutoFill.db");
    return backingStore;
}

AutofillBackingStore::AutofillBackingStore()
    : m_addStatement(nullptr)
    , m_updateStatement(nullptr)
    , m_containsStatement(nullptr)
    , m_getStatement(nullptr)
{
}

AutofillBackingStore::~AutofillBackingStore()
{
    if (m_database.isOpen())
        m_database.close();
}

bool AutofillBackingStore::open(const String& dbPath)
{
    ASSERT(!m_database.isOpen());

    HANDLE_SQL_EXEC_FAILURE(!m_database.open(dbPath), false,
        "Failed to open database file %s for autofill database", dbPath.utf8().data());

    if (!m_database.tableExists("autofill")) {
        HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("CREATE TABLE autofill (id INTEGER PRIMARY KEY, name VARCHAR NOT NULL, value VARCHAR NOT NULL, count INTEGER DEFAULT 1)"_s),
            false, "Failed to create table autofill for autofill database");

        // Create index for table autofill.
        HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("CREATE INDEX autofill_name ON autofill (name)"_s),
            false, "Failed to create autofill_name index for table autofill");
    }

    // Prepare the statements.
    auto s1 = m_database.prepareHeapStatement("INSERT INTO autofill (name, value) VALUES (?, ?)"_s);
    HANDLE_SQL_EXEC_FAILURE(s1,
        false, "Failed to prepare add statement");
    m_addStatement = s1.value().moveToUniquePtr();

    auto s2 = m_database.prepareHeapStatement("UPDATE autofill SET count = (SELECT count + 1 from autofill WHERE name = ? AND value = ?) WHERE name = ? AND value = ?"_s);
    HANDLE_SQL_EXEC_FAILURE(s2,
        false, "Failed to prepare update statement");
    m_updateStatement = s2.value().moveToUniquePtr();

    auto s3 = m_database.prepareHeapStatement("SELECT COUNT(*) FROM autofill WHERE name = ? AND value = ?"_s);
    HANDLE_SQL_EXEC_FAILURE(s3,
        false, "Failed to prepare contains statement");
    m_containsStatement = s3.value().moveToUniquePtr();

    auto s4 = m_database.prepareHeapStatement("SELECT value FROM autofill WHERE name = ? and value like ? ORDER BY count DESC"_s);
    HANDLE_SQL_EXEC_FAILURE(s4,
        false, "Failed to prepare get statement");
    m_getStatement = s4.value().moveToUniquePtr();

    return true;
}

bool AutofillBackingStore::add(const String& name, const String& value)
{
    if (name.isEmpty() || value.isEmpty())
        return false;

    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    if (contains(name, value))
        return update(name, value);

    if (!m_addStatement)
        return false;

    m_addStatement->bindText(1, name);
    m_addStatement->bindText(2, value);

    int result = m_addStatement->step();
    m_addStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLITE_DONE, false,
        "Failed to add autofill item into table autofill - %i", result);

    return true;
}

bool AutofillBackingStore::update(const String& name, const String& value)
{
    if (!m_updateStatement)
        return false;

    m_updateStatement->bindText(1, name);
    m_updateStatement->bindText(2, value);
    m_updateStatement->bindText(3, name);
    m_updateStatement->bindText(4, value);

    int result = m_updateStatement->step();
    m_updateStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLITE_DONE, false,
        "Failed to update autofill item in table autofill - %i", result);

    return true;
}

bool AutofillBackingStore::contains(const String& name, const String& value) const
{
    if (!m_containsStatement)
        return false;

    m_containsStatement->bindText(1, name);
    m_containsStatement->bindText(2, value);

    int result = m_containsStatement->step();
    int numberOfRows = m_containsStatement->columnInt(0);
    m_containsStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLITE_ROW, false,
        "Failed to execute select autofill item from table autofill in contains - %i", result);

    return numberOfRows;
}

Vector<String> AutofillBackingStore::get(const String& name, const String& valueHint)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    Vector<String> candidates;
    if (name.isEmpty() || valueHint.isEmpty() || !m_getStatement)
        return candidates;

    String value = "%" + valueHint + "%";
    m_getStatement->bindText(1, name);
    m_getStatement->bindText(2, value);

    int result;
    while ((result = m_getStatement->step()) == SQLITE_ROW)
        candidates.append(m_getStatement->columnText(0));
    m_getStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLITE_DONE, candidates,
        "Failed to execute select autofill item from table autofill in get - %i", result);

    return candidates;
}

bool AutofillBackingStore::clear()
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("DELETE FROM autofill"_s),
        false, "Failed to clear table autofill");

    return true;
}

} // namespace WebCore
