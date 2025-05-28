#pragma once
#include "WebKit.h"
#include <WebCore/BackForwardClient.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

namespace WebCore {
	class HistoryItem;
}

typedef WTF::Vector<WTF::Ref<WebCore::HistoryItem>> HistoryItemVector;
typedef WTF::HashSet<WTF::RefPtr<WebCore::HistoryItem>> HistoryItemHashSet;

namespace WebKit {

class WebPage;

class BackForwardClientMorphOS : public WebCore::BackForwardClient
{
public:
    static Ref<BackForwardClientMorphOS> create(WebPage *view)
    {
        return WTF::adoptRef(*new BackForwardClientMorphOS(view));
    }
	
    void addItem(WTF::Ref<WebCore::HistoryItem>&&) override;
    void goBack();
    void goForward();
    void goToItem(WebCore::HistoryItem&) override;
	
    WTF::RefPtr<WebCore::HistoryItem> backItem();
    WTF::RefPtr<WebCore::HistoryItem> currentItem();
    WTF::RefPtr<WebCore::HistoryItem> forwardItem();
    WTF::RefPtr<WebCore::HistoryItem> itemAtIndex(int) override;

    void backListWithLimit(int, HistoryItemVector&);
    void forwardListWithLimit(int, HistoryItemVector&);

    int capacity();
    void setCapacity(int);
    bool enabled();
    void setEnabled(bool);
    unsigned backListCount() const final;
    unsigned forwardListCount() const final;
    bool containsItem(WebCore::HistoryItem*);

    void close() override;
    bool closed();

    void removeItem(WebCore::HistoryItem*);
    HistoryItemVector& entries();

private:
    explicit BackForwardClientMorphOS(WebPage *page);

	WebPage *m_page;
    HistoryItemVector m_entries;
    HistoryItemHashSet m_entryHash;
    unsigned m_current;
    unsigned m_capacity;
    bool m_closed;
    bool m_enabled;
};

}
