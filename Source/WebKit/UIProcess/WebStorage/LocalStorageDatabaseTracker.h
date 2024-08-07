/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <WebCore/SQLiteDatabase.h>
#include <wtf/HashSet.h>
#include <wtf/Optional.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/WallTime.h>
#include <wtf/WorkQueue.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class SecurityOrigin;
struct SecurityOriginData;
}

namespace WebKit {

struct LocalStorageDetails;

class LocalStorageDatabaseTracker : public ThreadSafeRefCounted<LocalStorageDatabaseTracker> {
public:
    static Ref<LocalStorageDatabaseTracker> create(Ref<WorkQueue>&&, const String& localStorageDirectory);
    ~LocalStorageDatabaseTracker();

    String databasePath(const WebCore::SecurityOriginData&) const;

    void didOpenDatabaseWithOrigin(const WebCore::SecurityOriginData&);
    void deleteDatabaseWithOrigin(const WebCore::SecurityOriginData&);
    void deleteAllDatabases();

    // Returns a vector of the origins whose databases have been deleted.
    Vector<WebCore::SecurityOriginData> deleteDatabasesModifiedSince(WallTime);

    Vector<WebCore::SecurityOriginData> origins() const;

    struct OriginDetails {
        String originIdentifier;
        std::optional<time_t> creationTime;
        std::optional<time_t> modificationTime;
    };
    Vector<OriginDetails> originDetails();

private:
    LocalStorageDatabaseTracker(Ref<WorkQueue>&&, const String& localStorageDirectory);

    String databasePath(const String& filename) const;
    String trackerDatabasePath() const;

    enum DatabaseOpeningStrategy {
        CreateIfNonExistent,
        SkipIfNonExistent
    };
    void openTrackerDatabase(DatabaseOpeningStrategy);

    void importOriginIdentifiers();
    void updateTrackerDatabaseFromLocalStorageDatabaseFiles();

    void addDatabaseWithOriginIdentifier(const String& originIdentifier, const String& databasePath);
    void removeDatabaseWithOriginIdentifier(const String& originIdentifier);
    String pathForDatabaseWithOriginIdentifier(const String& originIdentifier);

    RefPtr<WorkQueue> m_queue;
    String m_localStorageDirectory;

    WebCore::SQLiteDatabase m_database;
    HashSet<String> m_origins;

#if PLATFORM(IOS)
    void platformMaybeExcludeFromBackup() const;

    mutable bool m_hasExcludedFromBackup { false };
#endif
};

} // namespace WebKit
