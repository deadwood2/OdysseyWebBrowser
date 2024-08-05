/*
 * Copyright (C) 2017 Igalia S.L.
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
#include "WebKitWebView.h"

#include "WebKitAuthenticationDialog.h"
#include "WebKitScriptDialogPrivate.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebKitWebViewPrivate.h"
#include <WebCore/GtkUtilities.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

gboolean webkitWebViewAuthenticate(WebKitWebView* webView, WebKitAuthenticationRequest* request)
{
    CredentialStorageMode credentialStorageMode = webkit_authentication_request_can_save_credentials(request) ? AllowPersistentStorage : DisallowPersistentStorage;
    webkitWebViewBaseAddAuthenticationDialog(WEBKIT_WEB_VIEW_BASE(webView), webkitAuthenticationDialogNew(request, credentialStorageMode));

    return TRUE;
}

gboolean webkitWebViewScriptDialog(WebKitWebView* webView, WebKitScriptDialog* scriptDialog)
{
    webkitScriptDialogRun(scriptDialog, webView);
    return TRUE;
}

static void fileChooserDialogResponseCallback(GtkFileChooser* dialog, gint responseID, WebKitFileChooserRequest* request)
{
    GRefPtr<WebKitFileChooserRequest> adoptedRequest = adoptGRef(request);
    if (responseID == GTK_RESPONSE_ACCEPT) {
        GUniquePtr<GSList> filesList(gtk_file_chooser_get_filenames(dialog));
        GRefPtr<GPtrArray> filesArray = adoptGRef(g_ptr_array_new());
        for (GSList* file = filesList.get(); file; file = g_slist_next(file))
            g_ptr_array_add(filesArray.get(), file->data);
        g_ptr_array_add(filesArray.get(), 0);
        webkit_file_chooser_request_select_files(adoptedRequest.get(), reinterpret_cast<const gchar* const*>(filesArray->pdata));
    } else
        webkit_file_chooser_request_cancel(adoptedRequest.get());

#if GTK_CHECK_VERSION(3, 20, 0)
    g_object_unref(dialog);
#else
    gtk_widget_destroy(GTK_WIDGET(dialog));
#endif
}

gboolean webkitWebViewRunFileChooser(WebKitWebView* webView, WebKitFileChooserRequest* request)
{
    GtkWidget* toplevel = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    if (!WebCore::widgetIsOnscreenToplevelWindow(toplevel))
        toplevel = 0;

    gboolean allowsMultipleSelection = webkit_file_chooser_request_get_select_multiple(request);

#if GTK_CHECK_VERSION(3, 20, 0)
    GtkFileChooserNative* dialog = gtk_file_chooser_native_new(allowsMultipleSelection ? _("Select Files") : _("Select File"),
        toplevel ? GTK_WINDOW(toplevel) : nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, nullptr, nullptr);
    if (toplevel)
        gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
#else
    GtkWidget* dialog = gtk_file_chooser_dialog_new(allowsMultipleSelection ? _("Select Files") : _("Select File"),
        toplevel ? GTK_WINDOW(toplevel) : nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        nullptr);
    if (toplevel)
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
#endif

    if (GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(request))
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), allowsMultipleSelection);

    if (const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request))
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), selectedFiles[0]);

    g_signal_connect(dialog, "response", G_CALLBACK(fileChooserDialogResponseCallback), g_object_ref(request));

#if GTK_CHECK_VERSION(3, 20, 0)
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
#else
    gtk_widget_show(dialog);
#endif

    return TRUE;
}

/**
 * webkit_web_view_new:
 *
 * Creates a new #WebKitWebView with the default #WebKitWebContext and
 * no #WebKitUserContentManager associated with it.
 * See also webkit_web_view_new_with_context(),
 * webkit_web_view_new_with_user_content_manager(), and
 * webkit_web_view_new_with_settings().
 *
 * Returns: The newly created #WebKitWebView widget
 */
GtkWidget* webkit_web_view_new()
{
    return webkit_web_view_new_with_context(webkit_web_context_get_default());
}

/**
 * webkit_web_view_new_with_context:
 * @context: the #WebKitWebContext to be used by the #WebKitWebView
 *
 * Creates a new #WebKitWebView with the given #WebKitWebContext and
 * no #WebKitUserContentManager associated with it.
 * See also webkit_web_view_new_with_user_content_manager() and
 * webkit_web_view_new_with_settings().
 *
 * Returns: The newly created #WebKitWebView widget
 */
GtkWidget* webkit_web_view_new_with_context(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

    return GTK_WIDGET(g_object_new(WEBKIT_TYPE_WEB_VIEW,
        "is-ephemeral", webkit_web_context_is_ephemeral(context),
        "web-context", context,
        nullptr));
}

/**
 * webkit_web_view_new_with_related_view: (constructor)
 * @web_view: the related #WebKitWebView
 *
 * Creates a new #WebKitWebView sharing the same web process with @web_view.
 * This method doesn't have any effect when %WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS
 * process model is used, because a single web process is shared for all the web views in the
 * same #WebKitWebContext. When using %WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES process model,
 * this method should always be used when creating the #WebKitWebView in the #WebKitWebView::create signal.
 * You can also use this method to implement other process models based on %WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES,
 * like for example, sharing the same web process for all the views in the same security domain.
 *
 * The newly created #WebKitWebView will also have the same #WebKitUserContentManager
 * and #WebKitSettings as @web_view.
 *
 * Returns: (transfer full): The newly created #WebKitWebView widget
 *
 * Since: 2.4
 */
GtkWidget* webkit_web_view_new_with_related_view(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), nullptr);

    return GTK_WIDGET(g_object_new(WEBKIT_TYPE_WEB_VIEW,
        "user-content-manager", webkit_web_view_get_user_content_manager(webView),
        "settings", webkit_web_view_get_settings(webView),
        "related-view", webView,
        nullptr));
}

/**
 * webkit_web_view_new_with_settings:
 * @settings: a #WebKitSettings
 *
 * Creates a new #WebKitWebView with the given #WebKitSettings.
 * See also webkit_web_view_new_with_context(), and
 * webkit_web_view_new_with_user_content_manager().
 *
 * Returns: The newly created #WebKitWebView widget
 *
 * Since: 2.6
 */
GtkWidget* webkit_web_view_new_with_settings(WebKitSettings* settings)
{
    g_return_val_if_fail(WEBKIT_IS_SETTINGS(settings), nullptr);
    return GTK_WIDGET(g_object_new(WEBKIT_TYPE_WEB_VIEW, "settings", settings, nullptr));
}

/**
 * webkit_web_view_new_with_user_content_manager:
 * @user_content_manager: a #WebKitUserContentManager.
 *
 * Creates a new #WebKitWebView with the given #WebKitUserContentManager.
 * The content loaded in the view may be affected by the content injected
 * in the view by the user content manager.
 *
 * Returns: The newly created #WebKitWebView widget
 *
 * Since: 2.6
 */
GtkWidget* webkit_web_view_new_with_user_content_manager(WebKitUserContentManager* userContentManager)
{
    g_return_val_if_fail(WEBKIT_IS_USER_CONTENT_MANAGER(userContentManager), nullptr);

    return GTK_WIDGET(g_object_new(WEBKIT_TYPE_WEB_VIEW, "user-content-manager", userContentManager, nullptr));
}
