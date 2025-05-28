/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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

#import "config.h"

#import "PlatformUtilities.h"
#import <WebCore/SQLiteDatabase.h>
#import <WebCore/SQLiteStatement.h>
#import <WebCore/SQLiteTransaction.h>
#import <WebKit/WKFoundation.h>
#import <WebKit/WKProcessPoolPrivate.h>
#import <WebKit/WKWebViewPrivateForTesting.h>
#import <WebKit/WKWebsiteDataStorePrivate.h>
#import <WebKit/_WKWebsiteDataStoreConfiguration.h>
#import <wtf/RetainPtr.h>

// FIXME: Remove _statisticsDatabaseColumnsForTable and its infrastructure in another patch.
// It is no longer needed, but removing it is likely to cause unnecessary merge conflicts.

static RetainPtr<WKWebView> webViewWithResourceLoadStatisticsEnabledInNetworkProcess()
{
    auto *sharedProcessPool = [WKProcessPool _sharedProcessPool];
    auto *dataStore = [WKWebsiteDataStore defaultDataStore];
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [configuration setProcessPool: sharedProcessPool];
    configuration.get().websiteDataStore = dataStore;

    // We need an active NetworkProcess to perform PCM operations.
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);
    [dataStore _setResourceLoadStatisticsEnabled:YES];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"about:blank"]]];
    return webView;
}

template<size_t size>
void addValuesToTable(WebCore::SQLiteDatabase& database, ASCIILiteral query, std::array<Variant<StringView, int, double>, size> values)
{
    auto statement = database.prepareStatement(query);
    EXPECT_TRUE(!!statement);
    for (size_t i = 0; i < size; i++) {
        auto visitor = WTF::makeVisitor([&] (StringView string) {
            return statement->bindText(i + 1, string);
        }, [&] (int integer) {
            return statement->bindInt(i + 1, integer);
        }, [&] (double real) {
            return statement->bindDouble(i + 1, real);
        });
        auto result = WTF::visit(visitor, values[i]);
        EXPECT_EQ(result, SQLITE_OK);
    }
    EXPECT_EQ(statement->step(), SQLITE_DONE);
    EXPECT_EQ(statement->reset(), SQLITE_OK);
}

static double earliestTimeToSend()
{
    // Set the earliest time to send to tomorrow to make sure we don't actually send attribution reports while running this test.
    return (WallTime::now() + Seconds(24 * 60 * 60)).secondsSinceEpoch().seconds();
}

static void addAttributedPCMv1(WebCore::SQLiteDatabase& database)
{
    constexpr auto createAttributedPrivateClickMeasurementV1 = "CREATE TABLE AttributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, attributeOnSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "attributionTriggerData INTEGER NOT NULL, priority INTEGER NOT NULL, timeOfAdClick REAL NOT NULL, "
        "earliestTimeToSend REAL NOT NULL, "
        "FOREIGN KEY(sourceSiteDomainID) REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(attributeOnSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createAttributedPrivateClickMeasurementV1));

    constexpr auto insertAttributedPrivateClickMeasurementQueryV1 = "INSERT OR REPLACE INTO AttributedPrivateClickMeasurement (sourceSiteDomainID, attributeOnSiteDomainID, "
        "sourceID, attributionTriggerData, priority, timeOfAdClick, earliestTimeToSend) VALUES (?, ?, ?, ?, ?, ?, ?)"_s;
    addValuesToTable<7>(database, insertAttributedPrivateClickMeasurementQueryV1, { 1, 2, 42, 14, 7, 1.0, earliestTimeToSend() });
}

static void addUnattributedPCMv1(WebCore::SQLiteDatabase& database)
{
    constexpr auto createUnattributedPrivateClickMeasurementV1 = "CREATE TABLE UnattributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, attributeOnSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "timeOfAdClick REAL NOT NULL, FOREIGN KEY(sourceSiteDomainID) "
        "REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(attributeOnSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createUnattributedPrivateClickMeasurementV1));

    constexpr auto insertUnattributedPrivateClickMeasurementQueryV1 = "INSERT OR REPLACE INTO UnattributedPrivateClickMeasurement (sourceSiteDomainID, attributeOnSiteDomainID, "
        "sourceID, timeOfAdClick) VALUES (?, ?, ?, ?)"_s;
    addValuesToTable<4>(database, insertUnattributedPrivateClickMeasurementQueryV1, { 2, 3, 43, 1.0 });
}

static void addAttributedPCMv2(WebCore::SQLiteDatabase& database)
{
    constexpr auto createAttributedPrivateClickMeasurementV2 = "CREATE TABLE AttributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, attributeOnSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "attributionTriggerData INTEGER NOT NULL, priority INTEGER NOT NULL, timeOfAdClick REAL NOT NULL, "
        "earliestTimeToSend REAL NOT NULL, token TEXT, signature TEXT, keyID TEXT, "
        "FOREIGN KEY(sourceSiteDomainID) REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(attributeOnSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createAttributedPrivateClickMeasurementV2));

    constexpr auto insertAttributedPrivateClickMeasurementQueryV2 = "INSERT OR REPLACE INTO AttributedPrivateClickMeasurement (sourceSiteDomainID, attributeOnSiteDomainID, "
        "sourceID, attributionTriggerData, priority, timeOfAdClick, earliestTimeToSend, token, signature, keyID) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s;
    addValuesToTable<10>(database, insertAttributedPrivateClickMeasurementQueryV2, { 1, 2, 42, 14, 7, 1.0, earliestTimeToSend(), "test token", "test signature", "test key id" });
}

static void addUnattributedPCMv2(WebCore::SQLiteDatabase& database)
{
    constexpr auto createUnattributedPrivateClickMeasurementV2 = "CREATE TABLE UnattributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, attributeOnSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "timeOfAdClick REAL NOT NULL, token TEXT, signature TEXT, keyID TEXT, FOREIGN KEY(sourceSiteDomainID) "
        "REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(attributeOnSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createUnattributedPrivateClickMeasurementV2));

    constexpr auto insertUnattributedPrivateClickMeasurementQueryV2 = "INSERT OR REPLACE INTO UnattributedPrivateClickMeasurement (sourceSiteDomainID, attributeOnSiteDomainID, "
        "sourceID, timeOfAdClick, token, signature, keyID) VALUES (?, ?, ?, ?, ?, ?, ?)"_s;
    addValuesToTable<7>(database, insertUnattributedPrivateClickMeasurementQueryV2, { 2, 3, 43, 1.0, "test token", "test signature", "test key id" });
}

static void addAttributedPCMv3(WebCore::SQLiteDatabase& database)
{
    constexpr auto createAttributedPrivateClickMeasurementV3 = "CREATE TABLE AttributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, destinationSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "attributionTriggerData INTEGER NOT NULL, priority INTEGER NOT NULL, timeOfAdClick REAL NOT NULL, "
        "earliestTimeToSendToSource REAL, token TEXT, signature TEXT, keyID TEXT, earliestTimeToSendToDestination REAL, "
        "FOREIGN KEY(sourceSiteDomainID) REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(destinationSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createAttributedPrivateClickMeasurementV3));

    constexpr auto insertAttributedPrivateClickMeasurementQueryV3 = "INSERT OR REPLACE INTO AttributedPrivateClickMeasurement (sourceSiteDomainID, destinationSiteDomainID, "
        "sourceID, attributionTriggerData, priority, timeOfAdClick, earliestTimeToSendToSource, token, signature, keyID, earliestTimeToSendToDestination) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s;
    addValuesToTable<11>(database, insertAttributedPrivateClickMeasurementQueryV3, { 1, 2, 42, 14, 7, 1.0, earliestTimeToSend(), "test token", "test signature", "test key id", earliestTimeToSend() });
}

static void addUnattributedPCMv3(WebCore::SQLiteDatabase& database)
{
    constexpr auto createUnattributedPrivateClickMeasurementV3 = "CREATE TABLE UnattributedPrivateClickMeasurement ("
        "sourceSiteDomainID INTEGER NOT NULL, destinationSiteDomainID INTEGER NOT NULL, sourceID INTEGER NOT NULL, "
        "timeOfAdClick REAL NOT NULL, token TEXT, signature TEXT, keyID TEXT, FOREIGN KEY(sourceSiteDomainID) "
        "REFERENCES ObservedDomains(domainID) ON DELETE CASCADE, FOREIGN KEY(destinationSiteDomainID) REFERENCES "
        "ObservedDomains(domainID) ON DELETE CASCADE)"_s;
    EXPECT_TRUE(database.executeCommand(createUnattributedPrivateClickMeasurementV3));

    constexpr auto insertUnattributedPrivateClickMeasurementQueryV3 = "INSERT OR REPLACE INTO UnattributedPrivateClickMeasurement (sourceSiteDomainID, destinationSiteDomainID, "
        "sourceID, timeOfAdClick, token, signature, keyID) VALUES (?, ?, ?, ?, ?, ?, ?)"_s;
    addValuesToTable<7>(database, insertUnattributedPrivateClickMeasurementQueryV3, { 2, 3, 43, 1.0, "test token", "test signature", "test key id" });
}

static RetainPtr<NSString> dumpedPCM(WKWebView *webView)
{
    __block RetainPtr<NSString> pcm;
    [webView _dumpPrivateClickMeasurement:^(NSString *privateClickMeasurement) {
        pcm = privateClickMeasurement;
    }];
    while (!pcm)
        TestWebKitAPI::Util::spinRunLoop();
    return pcm;
}

static void pollUntilPCMIsMigrated(WKWebView *webView)
{
    // This query is the first thing to open the old database, so migration has not happened yet.
    const char* emptyPCMDatabase = "\nNo stored Private Click Measurement data.\n";
    EXPECT_WK_STREQ(dumpedPCM(webView).get(), emptyPCMDatabase);

    NSString *expectedMigratedPCMDatabase = @""
        "Unattributed Private Click Measurements:\n"
        "WebCore::PrivateClickMeasurement 1\n"
        "Source site: webkit.org\n"
        "Attribute on site: www.webkit.org\n"
        "Source ID: 43\n"
        "No attribution trigger data.\n"
        "\n"
        "Attributed Private Click Measurements:\n"
        "WebCore::PrivateClickMeasurement 2\n"
        "Source site: example.com\n"
        "Attribute on site: webkit.org\n"
        "Source ID: 42\n"
        "Attribution trigger data: 14\n"
        "Attribution priority: 7\n"
        "Attribution earliest time to send: Outside 24-48 hours\n";

    while (![dumpedPCM(webView) isEqualToString:expectedMigratedPCMDatabase])
        usleep(10000);
}

static NSString *emptyObservationsDBPath()
{
    NSFileManager *defaultFileManager = NSFileManager.defaultManager;
    NSURL *itpRootURL = WKWebsiteDataStore.defaultDataStore._configuration._resourceLoadStatisticsDirectory;
    NSURL *fileURL = [itpRootURL URLByAppendingPathComponent:@"observations.db"];
    [defaultFileManager removeItemAtPath:itpRootURL.path error:nil];
    EXPECT_FALSE([defaultFileManager fileExistsAtPath:itpRootURL.path]);
    [defaultFileManager createDirectoryAtURL:itpRootURL withIntermediateDirectories:YES attributes:nil error:nil];
    return fileURL.path;
}

static void cleanUp()
{
    NSFileManager *defaultFileManager = NSFileManager.defaultManager;
    NSString *itpRoot = WKWebsiteDataStore.defaultDataStore._configuration._resourceLoadStatisticsDirectory.path;
    [defaultFileManager removeItemAtPath:itpRoot error:nil];
    EXPECT_FALSE([defaultFileManager fileExistsAtPath:itpRoot]);
}

static void createAndPopulateObservedDomainTable(WebCore::SQLiteDatabase& database)
{
    auto addObservedDomain = [&](const char* domain) {
        constexpr auto insertObservedDomainQuery = "INSERT INTO ObservedDomains (registrableDomain, lastSeen, hadUserInteraction,"
        "mostRecentUserInteractionTime, grandfathered, isPrevalent, isVeryPrevalent, dataRecordsRemoved, timesAccessedAsFirstPartyDueToUserInteraction,"
        "timesAccessedAsFirstPartyDueToStorageAccessAPI, isScheduledForAllButCookieDataRemoval) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s;
        addValuesToTable<11>(database, insertObservedDomainQuery, { domain, 1.0, 1, 1.0, 1, 1, 1, 1, 1, 1, 1 });
    };

    constexpr auto createObservedDomain = "CREATE TABLE ObservedDomains ("
    "domainID INTEGER PRIMARY KEY, registrableDomain TEXT NOT NULL UNIQUE ON CONFLICT FAIL, lastSeen REAL NOT NULL, "
    "hadUserInteraction INTEGER NOT NULL, mostRecentUserInteractionTime REAL NOT NULL, grandfathered INTEGER NOT NULL, "
    "isPrevalent INTEGER NOT NULL, isVeryPrevalent INTEGER NOT NULL, dataRecordsRemoved INTEGER NOT NULL,"
    "timesAccessedAsFirstPartyDueToUserInteraction INTEGER NOT NULL, timesAccessedAsFirstPartyDueToStorageAccessAPI INTEGER NOT NULL,"
    "isScheduledForAllButCookieDataRemoval INTEGER NOT NULL)"_s;

    EXPECT_TRUE(database.executeCommand(createObservedDomain));
    addObservedDomain("example.com");
    addObservedDomain("webkit.org");
    addObservedDomain("www.webkit.org");
}

void setUp(void(*addUnattributedPCM)(WebCore::SQLiteDatabase&), void(*addAttributedPCM)(WebCore::SQLiteDatabase&))
{
    WebCore::SQLiteDatabase database;
    EXPECT_TRUE(database.open(emptyObservationsDBPath()));
    createAndPopulateObservedDomainTable(database);
    addUnattributedPCM(database);
    addAttributedPCM(database);
    database.close();
}

TEST(PrivateClickMeasurement, MigrateFromResourceLoadStatistics1)
{
    setUp(addUnattributedPCMv1, addAttributedPCMv1);
    auto webView = webViewWithResourceLoadStatisticsEnabledInNetworkProcess();
    pollUntilPCMIsMigrated(webView.get());
    cleanUp();
}

TEST(PrivateClickMeasurement, MigrateFromResourceLoadStatistics2)
{
    setUp(addUnattributedPCMv2, addAttributedPCMv2);
    auto webView = webViewWithResourceLoadStatisticsEnabledInNetworkProcess();
    pollUntilPCMIsMigrated(webView.get());
    cleanUp();
}

TEST(PrivateClickMeasurement, MigrateFromResourceLoadStatistics3)
{
    setUp(addUnattributedPCMv3, addAttributedPCMv3);
    auto webView = webViewWithResourceLoadStatisticsEnabledInNetworkProcess();
    pollUntilPCMIsMigrated(webView.get());
    cleanUp();
}
