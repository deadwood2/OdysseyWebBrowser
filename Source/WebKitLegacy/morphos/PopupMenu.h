#pragma once
#include "WebKit.h"
#include <WebCore/PopupMenu.h>
#include <WebCore/PopupMenuClient.h>
#include <WebCore/SearchPopupMenu.h>

namespace WebCore {
	class PopupMenuClient;
}

namespace WebKit {

class WebPage;

class PopupMenuMorphOS : public WebCore::PopupMenu
{
public:
    PopupMenuMorphOS(WebCore::PopupMenuClient* client, WebPage* page);
    ~PopupMenuMorphOS();

    void show(const WebCore::IntRect&, WebCore::FrameView*, int index) override;
    void hide() override;
    void updateFromElement() override;
    void disconnectClient() override;

private:
	WebKit::WebPage*          m_page;
	WebCore::PopupMenuClient* m_client;
};

class SearchPopupMenuMorphOS : public WebCore::SearchPopupMenu {
public:
    SearchPopupMenuMorphOS(WebCore::PopupMenuClient*, WebPage* page);

    WebCore::PopupMenu* popupMenu() override;
    void saveRecentSearches(const AtomString& name, const Vector<WebCore::RecentSearch>&) override;
    void loadRecentSearches(const AtomString& name, Vector<WebCore::RecentSearch>&) override;
    bool enabled() override;

private:
    Ref<PopupMenuMorphOS> m_popup;
};

};
