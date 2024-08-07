/*
 * Copyright (C) 2012 Igalia S.L.
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

#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <gio/gio.h>
#if USE(GSTREAMER)
#include <gst/gst.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <wtf/Deque.h>
#include <wtf/ProcessID.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>

#if PLATFORM(GTK)
#include <webkit2/webkit-web-extension.h>
#elif PLATFORM(WPE)
#include <wpe/webkit-web-extension.h>
#endif

static const char introspectionXML[] =
    "<node>"
    " <interface name='org.webkit.gtk.WebExtensionTest'>"
    "  <method name='GetTitle'>"
    "   <arg type='t' name='pageID' direction='in'/>"
    "   <arg type='s' name='title' direction='out'/>"
    "  </method>"
    "  <method name='AbortProcess'>"
    "  </method>"
    "  <method name='RunJavaScriptInIsolatedWorld'>"
    "   <arg type='t' name='pageID' direction='in'/>"
    "   <arg type='s' name='script' direction='in'/>"
    "  </method>"
    "  <method name='GetProcessIdentifier'>"
    "   <arg type='u' name='identifier' direction='out'/>"
    "  </method>"
    "  <method name='RemoveAVPluginsFromGSTRegistry'>"
    "  </method>"
    "  <signal name='DocumentLoaded'/>"
    "  <signal name='FormControlsAssociated'>"
    "   <arg type='s' name='formIds' direction='out'/>"
    "  </signal>"
    "  <signal name='FormSubmissionWillSendDOMEvent'>"
    "    <arg type='s' name='formID' direction='out'/>"
    "    <arg type='s' name='textFieldNames' direction='out'/>"
    "    <arg type='s' name='textFieldValues' direction='out'/>"
    "    <arg type='b' name='targetFrameIsMainFrame' direction='out'/>"
    "    <arg type='b' name='sourceFrameIsMainFrame' direction='out'/>"
    "  </signal>"
    "  <signal name='FormSubmissionWillComplete'>"
    "    <arg type='s' name='formID' direction='out'/>"
    "    <arg type='s' name='textFieldNames' direction='out'/>"
    "    <arg type='s' name='textFieldValues' direction='out'/>"
    "    <arg type='b' name='targetFrameIsMainFrame' direction='out'/>"
    "    <arg type='b' name='sourceFrameIsMainFrame' direction='out'/>"
    "  </signal>"
    "  <signal name='URIChanged'>"
    "   <arg type='s' name='uri' direction='out'/>"
    "  </signal>"
    " </interface>"
    "</node>";


typedef enum {
    DocumentLoadedSignal,
    URIChangedSignal,
#if PLATFORM(GTK)
    FormControlsAssociatedSignal,
    FormSubmissionWillSendDOMEventSignal,
    FormSubmissionWillCompleteSignal,
#endif
} DelayedSignalType;

struct DelayedSignal {
    explicit DelayedSignal(DelayedSignalType type)
        : type(type)
    {
    }

    DelayedSignal(DelayedSignalType type, const char* str)
        : type(type)
        , str(str)
    {
    }

    DelayedSignal(DelayedSignalType type, const char* str, const char* str2, const char* str3, gboolean b, gboolean b2)
        : type(type)
        , str(str)
        , str2(str2)
        , str3(str3)
        , b(b)
        , b2(b2)
    {
    }

    DelayedSignalType type;
    CString str;
    CString str2;
    CString str3;
    gboolean b;
    gboolean b2;
};

Deque<DelayedSignal> delayedSignalsQueue;

static void emitDocumentLoaded(GDBusConnection* connection)
{
    bool ok = g_dbus_connection_emit_signal(
        connection,
        0,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        "DocumentLoaded",
        0,
        0);
    g_assert(ok);
}

static void documentLoadedCallback(WebKitWebPage* webPage, WebKitWebExtension* extension)
{
#if PLATFORM(GTK)
    WebKitDOMDocument* document = webkit_web_page_get_dom_document(webPage);
    GRefPtr<WebKitDOMDOMWindow> window = adoptGRef(webkit_dom_document_get_default_view(document));
    webkit_dom_dom_window_webkit_message_handlers_post_message(window.get(), "dom", "DocumentLoaded");
#endif

    gpointer data = g_object_get_data(G_OBJECT(extension), "dbus-connection");
    if (data)
        emitDocumentLoaded(G_DBUS_CONNECTION(data));
    else
        delayedSignalsQueue.append(DelayedSignal(DocumentLoadedSignal));
}

static void emitURIChanged(GDBusConnection* connection, const char* uri)
{
    bool ok = g_dbus_connection_emit_signal(
        connection,
        0,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        "URIChanged",
        g_variant_new("(s)", uri),
        0);
    g_assert(ok);
}

static void uriChangedCallback(WebKitWebPage* webPage, GParamSpec* pspec, WebKitWebExtension* extension)
{
    gpointer data = g_object_get_data(G_OBJECT(extension), "dbus-connection");
    if (data)
        emitURIChanged(G_DBUS_CONNECTION(data), webkit_web_page_get_uri(webPage));
    else
        delayedSignalsQueue.append(DelayedSignal(URIChangedSignal, webkit_web_page_get_uri(webPage)));
}

static gboolean sendRequestCallback(WebKitWebPage*, WebKitURIRequest* request, WebKitURIResponse* redirectResponse, gpointer)
{
    gboolean returnValue = FALSE;
    const char* requestURI = webkit_uri_request_get_uri(request);
    g_assert(requestURI);

    if (const char* suffix = g_strrstr(requestURI, "/remove-this/javascript.js")) {
        GUniquePtr<char> prefix(g_strndup(requestURI, strlen(requestURI) - strlen(suffix)));
        GUniquePtr<char> newURI(g_strdup_printf("%s/javascript.js", prefix.get()));
        webkit_uri_request_set_uri(request, newURI.get());
    } else if (const char* suffix = g_strrstr(requestURI, "/remove-this/javascript-after-redirection.js")) {
        // Redirected from /redirected.js, redirectResponse should be nullptr.
        g_assert(WEBKIT_IS_URI_RESPONSE(redirectResponse));
        g_assert(g_str_has_suffix(webkit_uri_response_get_uri(redirectResponse), "/redirected.js"));

        GUniquePtr<char> prefix(g_strndup(requestURI, strlen(requestURI) - strlen(suffix)));
        GUniquePtr<char> newURI(g_strdup_printf("%s/javascript-after-redirection.js", prefix.get()));
        webkit_uri_request_set_uri(request, newURI.get());
    } else if (g_str_has_suffix(requestURI, "/redirected.js")) {
        // Original request, redirectResponse should be nullptr.
        g_assert(!redirectResponse);
    } else if (g_str_has_suffix(requestURI, "/add-do-not-track-header")) {
        SoupMessageHeaders* headers = webkit_uri_request_get_http_headers(request);
        g_assert(headers);
        soup_message_headers_append(headers, "DNT", "1");
    } else if (g_str_has_suffix(requestURI, "/normal-change-request")) {
        GUniquePtr<char> prefix(g_strndup(requestURI, strlen(requestURI) - strlen("/normal-change-request")));
        GUniquePtr<char> newURI(g_strdup_printf("%s/request-changed%s", prefix.get(), redirectResponse ? "-on-redirect" : ""));
        webkit_uri_request_set_uri(request, newURI.get());
    } else if (g_str_has_suffix(requestURI, "/http-get-method")) {
        g_assert_cmpstr(webkit_uri_request_get_http_method(request), ==, "GET");
        g_assert(webkit_uri_request_get_http_method(request) == SOUP_METHOD_GET);
    } else if (g_str_has_suffix(requestURI, "/http-post-method")) {
        g_assert_cmpstr(webkit_uri_request_get_http_method(request), ==, "POST");
        g_assert(webkit_uri_request_get_http_method(request) == SOUP_METHOD_POST);
        returnValue = TRUE;
    } else if (g_str_has_suffix(requestURI, "/cancel-this.js"))
        returnValue = TRUE;

    return returnValue;
}

// FIXME: figure out what to do with WebKitWebHitTestResult in WPE.
#if PLATFORM(GTK)
static GVariant* serializeContextMenu(WebKitContextMenu* menu)
{
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
    GList* items = webkit_context_menu_get_items(menu);
    for (GList* it = items; it; it = g_list_next(it))
        g_variant_builder_add(&builder, "u", webkit_context_menu_item_get_stock_action(WEBKIT_CONTEXT_MENU_ITEM(it->data)));
    return g_variant_builder_end(&builder);
}

static GVariant* serializeNode(WebKitDOMNode* node)
{
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add(&builder, "{sv}", "Name", g_variant_new_take_string(webkit_dom_node_get_node_name(node)));
    g_variant_builder_add(&builder, "{sv}", "Type", g_variant_new_uint32(webkit_dom_node_get_node_type(node)));
    g_variant_builder_add(&builder, "{sv}", "Contents", g_variant_new_take_string(webkit_dom_node_get_text_content(node)));
    WebKitDOMNode* parent = webkit_dom_node_get_parent_node(node);
    g_variant_builder_add(&builder, "{sv}", "Parent", parent ? g_variant_new_take_string(webkit_dom_node_get_node_name(parent)) : g_variant_new_string("ROOT"));
    return g_variant_builder_end(&builder);
}

static gboolean contextMenuCallback(WebKitWebPage* page, WebKitContextMenu* menu, WebKitWebHitTestResult* hitTestResult, gpointer)
{
    const char* pageURI = webkit_web_page_get_uri(page);
    if (!g_strcmp0(pageURI, "ContextMenuTestDefault")) {
        webkit_context_menu_set_user_data(menu, serializeContextMenu(menu));
        return FALSE;
    }

    if (!g_strcmp0(pageURI, "ContextMenuTestCustom")) {
        // Remove Back and Forward, and add Inspector action.
        webkit_context_menu_remove(menu, webkit_context_menu_first(menu));
        webkit_context_menu_remove(menu, webkit_context_menu_first(menu));
        webkit_context_menu_append(menu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(menu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT));
        webkit_context_menu_set_user_data(menu, serializeContextMenu(menu));
        return TRUE;
    }

    if (!g_strcmp0(pageURI, "ContextMenuTestClear")) {
        webkit_context_menu_remove_all(menu);
        return TRUE;
    }

    if (!g_strcmp0(pageURI, "ContextMenuTestNode")) {
        WebKitDOMNode* node = webkit_web_hit_test_result_get_node(hitTestResult);
        g_assert(WEBKIT_DOM_IS_NODE(node));
        webkit_context_menu_set_user_data(menu, serializeNode(node));
        return TRUE;
    }

    return FALSE;
}
#endif // PLATFORM(GTK)

static void consoleMessageSentCallback(WebKitWebPage* webPage, WebKitConsoleMessage* consoleMessage)
{
    g_assert(consoleMessage);
    GRefPtr<GVariant> variant = g_variant_new("(uusus)", webkit_console_message_get_source(consoleMessage),
        webkit_console_message_get_level(consoleMessage), webkit_console_message_get_text(consoleMessage),
        webkit_console_message_get_line(consoleMessage), webkit_console_message_get_source_id(consoleMessage));
    GUniquePtr<char> messageString(g_variant_print(variant.get(), FALSE));
#if PLATFORM(GTK)
    GRefPtr<WebKitDOMDOMWindow> window = adoptGRef(webkit_dom_document_get_default_view(webkit_web_page_get_dom_document(webPage)));
    g_assert(WEBKIT_DOM_IS_DOM_WINDOW(window.get()));
    webkit_dom_dom_window_webkit_message_handlers_post_message(window.get(), "console", messageString.get());
#else
    GUniquePtr<char> escapedMessageString(static_cast<char*>(g_malloc(strlen(messageString.get()) * 2 + 1)));
    char* src = messageString.get();
    char* dest = escapedMessageString.get();
    while (*src) {
        if (*src == '"') {
            *dest++ = '\\';
            *dest++ = '"';
        } else
            *dest++ = *src;
        ++src;
    }
    *dest = '\0';
    GUniquePtr<char> script(g_strdup_printf("window.webkit.messageHandlers.console.postMessage(\"%s\");", escapedMessageString.get()));
    JSGlobalContextRef jsContext = webkit_frame_get_javascript_global_context(webkit_web_page_get_main_frame(webPage));
    JSRetainPtr<JSStringRef> jsScript(Adopt, JSStringCreateWithUTF8CString(script.get()));
    JSEvaluateScript(jsContext, jsScript.get(), nullptr, nullptr, 1, nullptr);
#endif
}

#if PLATFORM(GTK)
static void emitFormControlsAssociated(GDBusConnection* connection, const char* formIds)
{
    bool ok = g_dbus_connection_emit_signal(
        connection,
        nullptr,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        "FormControlsAssociated",
        g_variant_new("(s)", formIds),
        nullptr);
    g_assert(ok);
}

static void formControlsAssociatedCallback(WebKitWebPage* webPage, GPtrArray* formElements, WebKitWebExtension* extension)
{
    GString* formIdsBuilder = g_string_new(nullptr);
    for (guint i = 0; i < formElements->len; ++i) {
        g_assert(WEBKIT_DOM_IS_ELEMENT(g_ptr_array_index(formElements, i)));
        auto domElement = WEBKIT_DOM_ELEMENT(g_ptr_array_index(formElements, i));
        GUniquePtr<char> elementID(webkit_dom_element_get_id(domElement));
        if (elementID)
            g_string_append(formIdsBuilder, elementID.get());
    }
    if (!formIdsBuilder->len) {
        g_string_free(formIdsBuilder, TRUE);
        return;
    }
    GUniquePtr<char> formIds(g_string_free(formIdsBuilder, FALSE));
    gpointer data = g_object_get_data(G_OBJECT(extension), "dbus-connection");
    if (data)
        emitFormControlsAssociated(G_DBUS_CONNECTION(data), formIds.get());
    else
        delayedSignalsQueue.append(DelayedSignal(FormControlsAssociatedSignal, formIds.get()));
}

static void emitFormSubmissionEvent(GDBusConnection* connection, const char* methodName, const char* formID, const char* names, const char* values, gboolean targetFrameIsMainFrame, gboolean sourceFrameIsMainFrame)
{
    bool ok = g_dbus_connection_emit_signal(
        connection,
        nullptr,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        methodName,
        g_variant_new("(sssbb)", formID ? formID : "", names, values, targetFrameIsMainFrame, sourceFrameIsMainFrame),
        nullptr);
    g_assert(ok);
}

static void handleFormSubmissionCallback(WebKitWebPage* webPage, DelayedSignalType delayedSignalType, const char* methodName, const char* formID, WebKitFrame* sourceFrame, WebKitFrame* targetFrame, GPtrArray* textFieldNames, GPtrArray* textFieldValues, WebKitWebExtension* extension)
{
    GString* namesBuilder = g_string_new(nullptr);
    for (guint i = 0; i < textFieldNames->len; ++i) {
        auto* name = static_cast<char*>(g_ptr_array_index(textFieldNames, i));
        g_string_append(namesBuilder, name);
        g_string_append_c(namesBuilder, ',');
    }
    GUniquePtr<char> names(g_string_free(namesBuilder, FALSE));

    GString* valuesBuilder = g_string_new(nullptr);
    for (guint i = 0; i < textFieldValues->len; ++i) {
        auto* value = static_cast<char*>(g_ptr_array_index(textFieldValues, i));
        g_string_append(valuesBuilder, value);
        g_string_append_c(valuesBuilder, ',');
    }
    GUniquePtr<char> values(g_string_free(valuesBuilder, FALSE));

    gpointer data = g_object_get_data(G_OBJECT(extension), "dbus-connection");
    if (data)
        emitFormSubmissionEvent(G_DBUS_CONNECTION(data), methodName, formID, names.get(), values.get(), webkit_frame_is_main_frame(targetFrame), webkit_frame_is_main_frame(sourceFrame));
    else
        delayedSignalsQueue.append(DelayedSignal(delayedSignalType, formID, names.get(), values.get(), webkit_frame_is_main_frame(targetFrame), webkit_frame_is_main_frame(sourceFrame)));
}

static void willSubmitFormCallback(WebKitWebPage* webPage, WebKitDOMElement* formElement, WebKitFormSubmissionStep step, WebKitFrame* sourceFrame, WebKitFrame* targetFrame, GPtrArray* textFieldNames, GPtrArray* textFieldValues, WebKitWebExtension* extension)
{
    g_assert(WEBKIT_DOM_IS_HTML_FORM_ELEMENT(formElement));
    GUniquePtr<char> formID(webkit_dom_element_get_id(formElement));
    switch (step) {
    case WEBKIT_FORM_SUBMISSION_WILL_SEND_DOM_EVENT:
        handleFormSubmissionCallback(webPage, FormSubmissionWillSendDOMEventSignal, "FormSubmissionWillSendDOMEvent", formID.get(), sourceFrame, targetFrame, textFieldNames, textFieldValues, extension);
        break;
    case WEBKIT_FORM_SUBMISSION_WILL_COMPLETE:
        handleFormSubmissionCallback(webPage, FormSubmissionWillCompleteSignal, "FormSubmissionWillComplete", formID.get(), sourceFrame, targetFrame, textFieldNames, textFieldValues, extension);
        break;
    default:
        g_assert_not_reached();
    }
}
#endif

static void pageCreatedCallback(WebKitWebExtension* extension, WebKitWebPage* webPage, gpointer)
{
    g_signal_connect(webPage, "document-loaded", G_CALLBACK(documentLoadedCallback), extension);
    g_signal_connect(webPage, "notify::uri", G_CALLBACK(uriChangedCallback), extension);
    g_signal_connect(webPage, "send-request", G_CALLBACK(sendRequestCallback), nullptr);
    g_signal_connect(webPage, "console-message-sent", G_CALLBACK(consoleMessageSentCallback), nullptr);
#if PLATFORM(GTK)
    g_signal_connect(webPage, "context-menu", G_CALLBACK(contextMenuCallback), nullptr);
    g_signal_connect(webPage, "form-controls-associated", G_CALLBACK(formControlsAssociatedCallback), extension);
    g_signal_connect(webPage, "will-submit-form", G_CALLBACK(willSubmitFormCallback), extension);
#endif
}

static JSValueRef echoCallback(JSContextRef jsContext, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    if (argumentCount <= 0)
        return JSValueMakeUndefined(jsContext);

    JSRetainPtr<JSStringRef> string(Adopt, JSValueToStringCopy(jsContext, arguments[0], 0));
    return JSValueMakeString(jsContext, string.get());
}

static void windowObjectCleared(WebKitScriptWorld* world, WebKitWebPage* page, WebKitFrame* frame, gpointer)
{
    JSGlobalContextRef jsContext = webkit_frame_get_javascript_context_for_script_world(frame, world);
    g_assert(jsContext);
    JSObjectRef globalObject = JSContextGetGlobalObject(jsContext);
    g_assert(globalObject);

    JSRetainPtr<JSStringRef> functionName(Adopt, JSStringCreateWithUTF8CString("echo"));
    JSObjectRef function = JSObjectMakeFunctionWithCallback(jsContext, functionName.get(), echoCallback);
    JSObjectSetProperty(jsContext, globalObject, functionName.get(), function, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);
}

static WebKitWebPage* getWebPage(WebKitWebExtension* extension, uint64_t pageID, GDBusMethodInvocation* invocation)
{
    WebKitWebPage* page = webkit_web_extension_get_page(extension, pageID);
    if (!page) {
        g_dbus_method_invocation_return_error(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
            "Invalid page ID: %" G_GUINT64_FORMAT, pageID);
        return 0;
    }

    g_assert_cmpuint(webkit_web_page_get_id(page), ==, pageID);
    return page;
}

static void methodCallCallback(GDBusConnection* connection, const char* sender, const char* objectPath, const char* interfaceName, const char* methodName, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer userData)
{
    if (g_strcmp0(interfaceName, "org.webkit.gtk.WebExtensionTest"))
        return;

    if (!g_strcmp0(methodName, "GetTitle")) {
        uint64_t pageID;
        g_variant_get(parameters, "(t)", &pageID);
        WebKitWebPage* page = getWebPage(WEBKIT_WEB_EXTENSION(userData), pageID, invocation);
        if (!page)
            return;

#if PLATFORM(GTK)
        WebKitDOMDocument* document = webkit_web_page_get_dom_document(page);
        GUniquePtr<char> title(webkit_dom_document_get_title(document));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", title.get()));
#elif PLATFORM(WPE)
        // FIXME: Use JSC API to get the title from JavaScript.
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", ""));
#endif
    } else if (!g_strcmp0(methodName, "RunJavaScriptInIsolatedWorld")) {
        uint64_t pageID;
        const char* script;
        g_variant_get(parameters, "(t&s)", &pageID, &script);
        WebKitWebPage* page = getWebPage(WEBKIT_WEB_EXTENSION(userData), pageID, invocation);
        if (!page)
            return;

        GRefPtr<WebKitScriptWorld> world = adoptGRef(webkit_script_world_new());
        g_assert(webkit_script_world_get_default() != world.get());
        WebKitFrame* frame = webkit_web_page_get_main_frame(page);
        JSGlobalContextRef jsContext = webkit_frame_get_javascript_context_for_script_world(frame, world.get());
        JSRetainPtr<JSStringRef> jsScript(Adopt, JSStringCreateWithUTF8CString(script));
        JSEvaluateScript(jsContext, jsScript.get(), 0, 0, 0, 0);
        g_dbus_method_invocation_return_value(invocation, 0);
    } else if (!g_strcmp0(methodName, "AbortProcess")) {
        abort();
    } else if (!g_strcmp0(methodName, "GetProcessIdentifier")) {
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(u)", static_cast<guint32>(getCurrentProcessID())));
    } else if (!g_strcmp0(methodName, "RemoveAVPluginsFromGSTRegistry")) {
#if USE(GSTREAMER)
        gst_init(nullptr, nullptr);
        static const char* avPlugins[] = { "libav", "omx", "vaapi", nullptr };
        GstRegistry* registry = gst_registry_get();
        for (unsigned i = 0; avPlugins[i]; ++i) {
            if (GstPlugin* plugin = gst_registry_find_plugin(registry, avPlugins[i])) {
                gst_registry_remove_plugin(registry, plugin);
                gst_object_unref(plugin);
            }
        }
#endif
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
}

static const GDBusInterfaceVTable interfaceVirtualTable = {
    methodCallCallback, 0, 0, { 0, }
};

static void busAcquiredCallback(GDBusConnection* connection, const char* name, gpointer userData)
{
    static GDBusNodeInfo* introspectionData = 0;
    if (!introspectionData)
        introspectionData = g_dbus_node_info_new_for_xml(introspectionXML, 0);

    GUniqueOutPtr<GError> error;
    unsigned registrationID = g_dbus_connection_register_object(
        connection,
        "/org/webkit/gtk/WebExtensionTest",
        introspectionData->interfaces[0],
        &interfaceVirtualTable,
        g_object_ref(userData),
        static_cast<GDestroyNotify>(g_object_unref),
        &error.outPtr());
    if (!registrationID)
        g_warning("Failed to register object: %s\n", error->message);

    g_object_set_data(G_OBJECT(userData), "dbus-connection", connection);
    while (delayedSignalsQueue.size()) {
        DelayedSignal delayedSignal = delayedSignalsQueue.takeFirst();
        switch (delayedSignal.type) {
        case DocumentLoadedSignal:
            emitDocumentLoaded(connection);
            break;
        case URIChangedSignal:
            emitURIChanged(connection, delayedSignal.str.data());
            break;
#if PLATFORM(GTK)
        case FormControlsAssociatedSignal:
            emitFormControlsAssociated(connection, delayedSignal.str.data());
            break;
        case FormSubmissionWillCompleteSignal:
            emitFormSubmissionEvent(connection, "FormSubmissionWillComplete", delayedSignal.str.data(), delayedSignal.str2.data(), delayedSignal.str3.data(), delayedSignal.b, delayedSignal.b2);
            break;
        case FormSubmissionWillSendDOMEventSignal:
            emitFormSubmissionEvent(connection, "FormSubmissionWillSendDOMEvent", delayedSignal.str.data(), delayedSignal.str2.data(), delayedSignal.str3.data(), delayedSignal.b, delayedSignal.b2);
            break;
#endif
            g_assert_not_reached();
        }
    }
}

static void registerGResource(void)
{
    GUniquePtr<char> resourcesPath(g_build_filename(WEBKIT_TEST_RESOURCES_DIR, "webkitglib-tests-resources.gresource", nullptr));
    GResource* resource = g_resource_load(resourcesPath.get(), nullptr);
    g_assert(resource);

    g_resources_register(resource);
    g_resource_unref(resource);
}

extern "C" void webkit_web_extension_initialize_with_user_data(WebKitWebExtension* extension, GVariant* userData)
{
    g_signal_connect(extension, "page-created", G_CALLBACK(pageCreatedCallback), extension);
    g_signal_connect(webkit_script_world_get_default(), "window-object-cleared", G_CALLBACK(windowObjectCleared), 0);

    registerGResource();

    g_assert(userData);
    g_assert(g_variant_is_of_type(userData, G_VARIANT_TYPE_UINT32));
    GUniquePtr<char> busName(g_strdup_printf("org.webkit.gtk.WebExtensionTest%u", g_variant_get_uint32(userData)));
    g_bus_own_name(
        G_BUS_TYPE_SESSION,
        busName.get(),
        G_BUS_NAME_OWNER_FLAGS_NONE,
        busAcquiredCallback,
        0, 0,
        g_object_ref(extension),
        static_cast<GDestroyNotify>(g_object_unref));
}
