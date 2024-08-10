/*
 * Copyright (C) 2013 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "WebProcessTest.h"
#include <gio/gio.h>
#include <wtf/glib/GUniquePtr.h>

class WebKitFrameTest : public WebProcessTest {
public:
    static std::unique_ptr<WebProcessTest> create() { return std::unique_ptr<WebProcessTest>(new WebKitFrameTest()); }

private:
    bool testMainFrame(WebKitWebPage* page)
    {
        WebKitFrame* frame = webkit_web_page_get_main_frame(page);
        g_assert(WEBKIT_IS_FRAME(frame));
        g_assert(webkit_frame_is_main_frame(frame));

        return true;
    }

    bool testURI(WebKitWebPage* page)
    {
        WebKitFrame* frame = webkit_web_page_get_main_frame(page);
        g_assert(WEBKIT_IS_FRAME(frame));
        g_assert_cmpstr(webkit_web_page_get_uri(page), ==, webkit_frame_get_uri(frame));

        return true;
    }

    bool testJavaScriptContext(WebKitWebPage* page)
    {
        WebKitFrame* frame = webkit_web_page_get_main_frame(page);
        g_assert(WEBKIT_IS_FRAME(frame));
#if PLATFORM(GTK)
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        g_assert(webkit_frame_get_javascript_global_context(frame));
        G_GNUC_END_IGNORE_DEPRECATIONS;
#endif

        GRefPtr<JSCContext> jsContext = adoptGRef(webkit_frame_get_js_context(frame));
        g_assert(JSC_IS_CONTEXT(jsContext.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(jsContext.get()));

        return true;
    }

    bool testJavaScriptValues(WebKitWebPage* page)
    {
        WebKitFrame* frame = webkit_web_page_get_main_frame(page);
        g_assert(WEBKIT_IS_FRAME(frame));

        GRefPtr<JSCContext> jsContext = adoptGRef(webkit_frame_get_js_context(frame));
        g_assert(JSC_IS_CONTEXT(jsContext.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(jsContext.get()));

        WebKitDOMDocument* document = webkit_web_page_get_dom_document(page);
        g_assert(WEBKIT_DOM_IS_DOCUMENT(document));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(document));

        GRefPtr<JSCValue> jsDocument = adoptGRef(webkit_frame_get_js_value_for_dom_object(frame, WEBKIT_DOM_OBJECT(document)));
        g_assert(JSC_IS_VALUE(jsDocument.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(jsDocument.get()));
        g_assert(jsc_value_get_context(jsDocument.get()) == jsContext.get());

        GRefPtr<JSCValue> value = adoptGRef(jsc_context_get_value(jsContext.get(), "document"));
        g_assert(value.get() == jsDocument.get());

#if PLATFORM(GTK)
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        WebKitDOMElement* p = webkit_dom_document_get_element_by_id(document, "paragraph");
        G_GNUC_END_IGNORE_DEPRECATIONS;
        g_assert(WEBKIT_DOM_IS_ELEMENT(p));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(p));

        GRefPtr<JSCValue> jsP = adoptGRef(webkit_frame_get_js_value_for_dom_object(frame, WEBKIT_DOM_OBJECT(p)));
#else
        GRefPtr<JSCValue> jsP = adoptGRef(jsc_value_object_invoke_method(jsDocument.get(), "getElementById", G_TYPE_STRING, "paragraph", G_TYPE_NONE));
#endif
        g_assert(JSC_IS_VALUE(jsP.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(jsP.get()));
        g_assert(jsc_value_is_object(jsP.get()));
        g_assert(jsc_value_get_context(jsP.get()) == jsContext.get());

        value = adoptGRef(jsc_context_evaluate(jsContext.get(), "document.getElementById('paragraph')", -1));
        g_assert(value.get() == jsP.get());
#if PLATFORM(GTK)
        value = adoptGRef(jsc_value_object_invoke_method(jsDocument.get(), "getElementById", G_TYPE_STRING, "paragraph", G_TYPE_NONE));
        g_assert(value.get() == jsP.get());
#endif

        value = adoptGRef(jsc_value_object_get_property(jsP.get(), "innerText"));
        g_assert(JSC_IS_VALUE(value.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(value.get()));
        g_assert(jsc_value_is_string(value.get()));
        GUniquePtr<char> strValue(jsc_value_to_string(value.get()));
        g_assert_cmpstr(strValue.get(), ==, "This is a test");

        GRefPtr<JSCValue> jsImage = adoptGRef(jsc_value_object_invoke_method(jsDocument.get(), "getElementById", G_TYPE_STRING, "image", G_TYPE_NONE));
        g_assert(JSC_IS_VALUE(jsImage.get()));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(jsImage.get()));
        g_assert(jsc_value_is_object(jsImage.get()));

        WebKitDOMNode* image = webkit_dom_node_for_js_value(jsImage.get());
        g_assert(WEBKIT_DOM_IS_ELEMENT(image));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(image));
#if PLATFORM(GTK)
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        g_assert(webkit_dom_document_get_element_by_id(document, "image") == WEBKIT_DOM_ELEMENT(image));
        G_GNUC_END_IGNORE_DEPRECATIONS;
#endif

        return true;
    }

    bool runTest(const char* testName, WebKitWebPage* page) override
    {
        if (!strcmp(testName, "main-frame"))
            return testMainFrame(page);
        if (!strcmp(testName, "uri"))
            return testURI(page);
        if (!strcmp(testName, "javascript-context"))
            return testJavaScriptContext(page);
        if (!strcmp(testName, "javascript-values"))
            return testJavaScriptValues(page);

        g_assert_not_reached();
        return false;
    }
};

static void __attribute__((constructor)) registerTests()
{
    REGISTER_TEST(WebKitFrameTest, "WebKitFrame/main-frame");
    REGISTER_TEST(WebKitFrameTest, "WebKitFrame/uri");
    REGISTER_TEST(WebKitFrameTest, "WebKitFrame/javascript-context");
    REGISTER_TEST(WebKitFrameTest, "WebKitFrame/javascript-values");
}
