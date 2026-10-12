#include "WebKit.h"
#import "LegacyHistoryItemClient.h"

LegacyHistoryItemClient& LegacyHistoryItemClient::singleton()
{
    static NeverDestroyed<Ref<LegacyHistoryItemClient>> client { adoptRef(*new LegacyHistoryItemClient) };
    return client.get().get();
}

void LegacyHistoryItemClient::historyItemChanged(const WebCore::HistoryItem&)
{
}
