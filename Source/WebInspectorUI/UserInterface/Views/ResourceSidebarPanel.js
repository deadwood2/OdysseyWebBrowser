/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
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

WebInspector.ResourceSidebarPanel = class ResourceSidebarPanel extends WebInspector.NavigationSidebarPanel
{
    constructor(contentBrowser)
    {
        super("resource", WebInspector.UIString("Resources"), true);

        this.contentBrowser = contentBrowser;

        this.filterBar.placeholder = WebInspector.UIString("Filter Resource List");

        this._waitingForInitialMainFrame = true;

        WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);

        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptAdded, this._scriptWasAdded, this);
        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptsCleared, this._scriptsCleared, this);

        WebInspector.notifications.addEventListener(WebInspector.Notification.ExtraDomainsActivated, this._extraDomainsActivated, this);

        this.contentTreeOutline.onselect = this._treeElementSelected.bind(this);
        this.contentTreeOutline.includeSourceMapResourceChildren = true;

        if (WebInspector.debuggableType === WebInspector.DebuggableType.JavaScript)
            this.contentTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName);
    }

    // Public

    showDefaultContentView()
    {
        if (WebInspector.frameResourceManager.mainFrame) {
            this.contentBrowser.showContentViewForRepresentedObject(WebInspector.frameResourceManager.mainFrame);
            return;
        }

        var firstTreeElement = this.contentTreeOutline.children[0];
        if (firstTreeElement)
            this.showDefaultContentViewForTreeElement(firstTreeElement);
    }

    treeElementForRepresentedObject(representedObject)
    {
        // A custom implementation is needed for this since the frames are populated lazily.

        if (!this._mainFrameTreeElement && (representedObject instanceof WebInspector.Resource || representedObject instanceof WebInspector.Frame)) {
            // All resources are under the main frame, so we need to return early if we don't have the main frame yet.
            return null;
        }

        // The Frame is used as the representedObject instead of the main resource in our tree.
        if (representedObject instanceof WebInspector.Resource && representedObject.parentFrame && representedObject.parentFrame.mainResource === representedObject)
            representedObject = representedObject.parentFrame;

        function isAncestor(ancestor, resourceOrFrame)
        {
            // SourceMapResources are descendants of another SourceCode object.
            if (resourceOrFrame instanceof WebInspector.SourceMapResource) {
                if (resourceOrFrame.sourceMap.originalSourceCode === ancestor)
                    return true;

                // Not a direct ancestor, so check the ancestors of the parent SourceCode object.
                resourceOrFrame = resourceOrFrame.sourceMap.originalSourceCode;
            }

            var currentFrame = resourceOrFrame.parentFrame;
            while (currentFrame) {
                if (currentFrame === ancestor)
                    return true;
                currentFrame = currentFrame.parentFrame;
            }

            return false;
        }

        function getParent(resourceOrFrame)
        {
            // SourceMapResources are descendants of another SourceCode object.
            if (resourceOrFrame instanceof WebInspector.SourceMapResource)
                return resourceOrFrame.sourceMap.originalSourceCode;
            return resourceOrFrame.parentFrame;
        }

        var treeElement = this.contentTreeOutline.findTreeElement(representedObject, isAncestor, getParent);
        if (treeElement)
            return treeElement;

        // Only special case Script objects.
        if (!(representedObject instanceof WebInspector.Script))
            return null;

        // If the Script has a URL we should have found it earlier.
        if (representedObject.url) {
            console.error("Didn't find a ScriptTreeElement for a Script with a URL.");
            return null;
        }

        // Since the Script does not have a URL we consider it an 'anonymous' script. These scripts happen from calls to
        // window.eval() or browser features like Auto Fill and Reader. They are not normally added to the sidebar, but since
        // we have a ScriptContentView asking for the tree element we will make a ScriptTreeElement on demand and add it.

        if (!this._anonymousScriptsFolderTreeElement)
            this._anonymousScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Anonymous Scripts"));

        if (!this._anonymousScriptsFolderTreeElement.parent) {
            var index = insertionIndexForObjectInListSortedByFunction(this._anonymousScriptsFolderTreeElement, this.contentTreeOutline.children, this._compareTreeElements);
            this.contentTreeOutline.insertChild(this._anonymousScriptsFolderTreeElement, index);
        }

        var scriptTreeElement = new WebInspector.ScriptTreeElement(representedObject);
        this._anonymousScriptsFolderTreeElement.appendChild(scriptTreeElement);

        return scriptTreeElement;
    }

    // Private

    _mainFrameDidChange(event)
    {
        if (this._mainFrameTreeElement) {
            this._mainFrameTreeElement.frame.removeEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainFrameMainResourceDidChange, this);
            this.contentTreeOutline.removeChild(this._mainFrameTreeElement);
            this._mainFrameTreeElement = null;
        }

        var newFrame = WebInspector.frameResourceManager.mainFrame;
        if (newFrame) {
            newFrame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainFrameMainResourceDidChange, this);
            this._mainFrameTreeElement = new WebInspector.FrameTreeElement(newFrame);
            this.contentTreeOutline.insertChild(this._mainFrameTreeElement, 0);

            // Select a tree element by default. Allow onselect if we aren't showing a content view.
            if (!this.contentTreeOutline.selectedTreeElement) {
                var currentContentView = this.contentBrowser.currentContentView;
                var treeElement = currentContentView ? this.treeElementForRepresentedObject(currentContentView.representedObject) : null;
                if (!treeElement)
                    treeElement = this._mainFrameTreeElement;
                this.showDefaultContentViewForTreeElement(treeElement);
            }
        }

        // The navigation path needs update when the main frame changes, since all resources are under the main frame.
        this.contentBrowser.updateHierarchicalPathForCurrentContentView();

        // We only care about the first time the main frame changes.
        if (!this._waitingForInitialMainFrame)
            return;

        // Only if there is a main frame.
        if (!newFrame)
            return;

        this._waitingForInitialMainFrame = false;
    }

    _mainFrameMainResourceDidChange(event)
    {
        var wasShowingResourceSidebar = this.selected;
        var currentContentView = this.contentBrowser.currentContentView;
        var wasShowingResourceContentView = currentContentView instanceof WebInspector.ResourceContentView
            || currentContentView instanceof WebInspector.ResourceClusterContentView
            || currentContentView instanceof WebInspector.FrameContentView
            || currentContentView instanceof WebInspector.ScriptContentView;

        this.contentBrowser.contentViewContainer.closeAllContentViews();

        function delayedWork()
        {
            // Show the main frame since there is no content view showing or we were showing a resource before.
            // Cookie restoration will attempt to re-select the resource we were showing.
            if (!this.contentBrowser.currentContentView || wasShowingResourceContentView) {
                // NOTE: This selection, during provisional loading, won't be saved, so it is
                // safe to do and not-clobber cookie restoration.
                this.showDefaultContentViewForTreeElement(this._mainFrameTreeElement);
            }
        }

        // Delay this work because other listeners of this event might not have fired yet. So selecting the main frame
        // before those listeners do their work might cause the content of the old page to show instead of the new page.
        setTimeout(delayedWork.bind(this), 0);
    }

    _scriptWasAdded(event)
    {
        var script = event.data.script;

        // We don't add scripts without URLs here. Those scripts can quickly clutter the interface and
        // are usually more transient. They will get added if/when they need to be shown in a content view.
        if (!script.url)
            return;

        // Exclude inspector scripts.
        if (script.url.startsWith("__WebInspector"))
            return;

        // If the script URL matches a resource we can assume it is part of that resource and does not need added.
        if (script.resource)
            return;

        var insertIntoTopLevel = false;

        if (script.injected) {
            if (!this._extensionScriptsFolderTreeElement)
                this._extensionScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Extension Scripts"));
            var parentFolderTreeElement = this._extensionScriptsFolderTreeElement;
        } else {
            if (WebInspector.debuggableType === WebInspector.DebuggableType.JavaScript && !WebInspector.hasExtraDomains)
                insertIntoTopLevel = true;
            else {
                if (!this._extraScriptsFolderTreeElement)
                    this._extraScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Extra Scripts"));
                var parentFolderTreeElement = this._extraScriptsFolderTreeElement;
            }
        }

        var scriptTreeElement = new WebInspector.ScriptTreeElement(script);

        if (insertIntoTopLevel) {
            var index = insertionIndexForObjectInListSortedByFunction(scriptTreeElement, this.contentTreeOutline.children, this._compareTreeElements);
            this.contentTreeOutline.insertChild(scriptTreeElement, index);
        } else {
            if (!parentFolderTreeElement.parent) {
                var index = insertionIndexForObjectInListSortedByFunction(parentFolderTreeElement, this.contentTreeOutline.children, this._compareTreeElements);
                this.contentTreeOutline.insertChild(parentFolderTreeElement, index);
            }

            parentFolderTreeElement.appendChild(scriptTreeElement);
        }
    }

    _scriptsCleared(event)
    {
        if (this._extensionScriptsFolderTreeElement) {
            if (this._extensionScriptsFolderTreeElement.parent)
                this._extensionScriptsFolderTreeElement.parent.removeChild(this._extensionScriptsFolderTreeElement);
            this._extensionScriptsFolderTreeElement = null;
        }

        if (this._extraScriptsFolderTreeElement) {
            if (this._extraScriptsFolderTreeElement.parent)
                this._extraScriptsFolderTreeElement.parent.removeChild(this._extraScriptsFolderTreeElement);
            this._extraScriptsFolderTreeElement = null;
        }

        if (this._anonymousScriptsFolderTreeElement) {
            if (this._anonymousScriptsFolderTreeElement.parent)
                this._anonymousScriptsFolderTreeElement.parent.removeChild(this._anonymousScriptsFolderTreeElement);
            this._anonymousScriptsFolderTreeElement = null;
        }
    }

    _treeElementSelected(treeElement, selectedByUser)
    {
        if (treeElement instanceof WebInspector.FolderTreeElement)
            return;

        if (treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement) {
            WebInspector.showRepresentedObject(treeElement.representedObject);
            return;
        }

        console.error("Unknown tree element", treeElement);
    }

    _compareTreeElements(a, b)
    {
        // Always sort the main frame element first.
        if (a instanceof WebInspector.FrameTreeElement)
            return -1;
        if (b instanceof WebInspector.FrameTreeElement)
            return 1;

        console.assert(a.mainTitle);
        console.assert(b.mainTitle);

        return (a.mainTitle || "").localeCompare(b.mainTitle || "");
    }

    _extraDomainsActivated()
    {
        if (WebInspector.debuggableType === WebInspector.DebuggableType.JavaScript)
            this.contentTreeOutline.element.classList.remove(WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName);
    }
};
