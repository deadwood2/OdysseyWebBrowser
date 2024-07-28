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

WebInspector.NavigationSidebarPanel = class NavigationSidebarPanel extends WebInspector.SidebarPanel
{
    constructor(identifier, displayName, shouldAutoPruneStaleTopLevelResourceTreeElements, wantsTopOverflowShadow, element, role, label)
    {
        super(identifier, displayName, element, role, label || displayName);

        this.element.classList.add("navigation");

        this._visibleContentTreeOutlines = new Set;

        this.contentView.element.addEventListener("scroll", this._updateContentOverflowShadowVisibility.bind(this));

        this._contentTreeOutline = this.createContentTreeOutline(true);
        this._selectedContentTreeOutline = null;

        this._filterBar = new WebInspector.FilterBar;
        this._filterBar.addEventListener(WebInspector.FilterBar.Event.FilterDidChange, this._filterDidChange, this);
        this.element.appendChild(this._filterBar.element);

        this._bottomOverflowShadowElement = document.createElement("div");
        this._bottomOverflowShadowElement.className = WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName;
        this.element.appendChild(this._bottomOverflowShadowElement);

        if (wantsTopOverflowShadow) {
            this._topOverflowShadowElement = document.createElement("div");
            this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName);
            this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName);
            this.element.appendChild(this._topOverflowShadowElement);
        }

        this._boundUpdateContentOverflowShadowVisibility = this._updateContentOverflowShadowVisibility.bind(this);
        window.addEventListener("resize", this._boundUpdateContentOverflowShadowVisibility);

        this._filtersSetting = new WebInspector.Setting(identifier + "-navigation-sidebar-filters", {});
        this._filterBar.filters = this._filtersSetting.value;

        this._emptyContentPlaceholderElements = new Map;
        this._emptyFilterResults = new Map;

        this._shouldAutoPruneStaleTopLevelResourceTreeElements = shouldAutoPruneStaleTopLevelResourceTreeElements || false;

        if (this._shouldAutoPruneStaleTopLevelResourceTreeElements) {
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._checkForStaleResources, this);
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasRemoved, this._checkForStaleResources, this);
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasRemoved, this._checkForStaleResources, this);
        }

        this._pendingViewStateCookie = null;
        this._restoringState = false;
    }

    // Public

    closed()
    {
        window.removeEventListener("resize", this._boundUpdateContentOverflowShadowVisibility);
        WebInspector.Frame.removeEventListener(null, null, this);
    }

    get contentBrowser()
    {
        return this._contentBrowser;
    }

    set contentBrowser(contentBrowser)
    {
        this._contentBrowser = contentBrowser || null;
    }

    get contentTreeOutline()
    {
        return this._contentTreeOutline;
    }

    set contentTreeOutline(newTreeOutline)
    {
        console.assert(newTreeOutline);
        if (!newTreeOutline)
            return;

        if (this._contentTreeOutline) {
            this.hideEmptyContentPlaceholder(this._contentTreeOutline);
            this._contentTreeOutline.hidden = true;
            this._visibleContentTreeOutlines.delete(this._contentTreeOutline);
        }

        this._contentTreeOutline = newTreeOutline;
        this._contentTreeOutline.hidden = false;

        this._visibleContentTreeOutlines.add(newTreeOutline);

        this._updateFilter();
    }

    get visibleContentTreeOutlines()
    {
        return this._visibleContentTreeOutlines;
    }

    get hasSelectedElement()
    {
        return this._visibleContentTreeOutlines.some((treeOutline) => !!treeOutline.selectedTreeElement);
    }

    get filterBar()
    {
        return this._filterBar;
    }

    get restoringState()
    {
        return this._restoringState;
    }

    cancelRestoringState()
    {
        if (!this._finalAttemptToRestoreViewStateTimeout)
            return;

        clearTimeout(this._finalAttemptToRestoreViewStateTimeout);
        this._finalAttemptToRestoreViewStateTimeout = undefined;
    }

    createContentTreeOutline(dontHideByDefault, suppressFiltering)
    {
        let contentTreeOutline = new WebInspector.TreeOutline;
        contentTreeOutline.allowsRepeatSelection = true;
        contentTreeOutline.hidden = !dontHideByDefault;
        contentTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName);

        this.contentView.element.appendChild(contentTreeOutline.element);

        if (!suppressFiltering) {
            contentTreeOutline.addEventListener(WebInspector.TreeOutline.Event.ElementAdded, this._treeElementAddedOrChanged, this);
            contentTreeOutline.addEventListener(WebInspector.TreeOutline.Event.ElementDidChange, this._treeElementAddedOrChanged, this);
            contentTreeOutline.addEventListener(WebInspector.TreeOutline.Event.ElementDisclosureDidChanged, this._treeElementDisclosureDidChange, this);
            contentTreeOutline.addEventListener(WebInspector.TreeOutline.Event.SelectionDidChange, this._treeSelectionDidChange, this);
        }

        contentTreeOutline[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol] = suppressFiltering;

        if (dontHideByDefault)
            this._visibleContentTreeOutlines.add(contentTreeOutline);

        return contentTreeOutline;
    }

    suppressFilteringOnTreeElements(treeElements)
    {
        console.assert(Array.isArray(treeElements), "TreeElements should be an array.");

        for (let treeElement of treeElements)
            treeElement[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol] = true;

        this._updateFilter();
    }

    treeElementForRepresentedObject(representedObject)
    {
        let treeElement = null;
        for (let treeOutline of this._visibleContentTreeOutlines) {
            treeElement = treeOutline.getCachedTreeElement(representedObject);
            if (treeElement)
                break;
        }

        return treeElement;
    }

    showDefaultContentView()
    {
        // Implemented by subclasses if needed to show a content view when no existing tree element is selected.
    }

    showDefaultContentViewForTreeElement(treeElement)
    {
        console.assert(treeElement);
        console.assert(treeElement.representedObject);
        if (!treeElement || !treeElement.representedObject)
            return;

        this.contentBrowser.showContentViewForRepresentedObject(treeElement.representedObject);
        treeElement.revealAndSelect(true, false, true, true);
    }

    saveStateToCookie(cookie)
    {
        console.assert(cookie);

        // This does not save folder selections, which lack a represented object and content view.
        var selectedTreeElement = null;
        this._visibleContentTreeOutlines.forEach(function(outline) {
            if (outline.selectedTreeElement)
                selectedTreeElement = outline.selectedTreeElement;
        });

        if (!selectedTreeElement)
            return;

        if (this._isTreeElementWithoutRepresentedObject(selectedTreeElement))
            return;

        var representedObject = selectedTreeElement.representedObject;
        cookie[WebInspector.TypeIdentifierCookieKey] = representedObject.constructor.TypeIdentifier;

        if (representedObject.saveIdentityToCookie)
            representedObject.saveIdentityToCookie(cookie);
        else
            console.error("Error: TreeElement.representedObject is missing a saveIdentityToCookie implementation. TreeElement.constructor: ", selectedTreeElement.constructor);
    }

    // This can be supplemented by subclasses that admit a simpler strategy for static tree elements.
    restoreStateFromCookie(cookie, relaxedMatchDelay)
    {
        this._pendingViewStateCookie = cookie;
        this._restoringState = true;

        // Check if any existing tree elements in any outline match the cookie.
        this._checkOutlinesForPendingViewStateCookie();

        if (this._finalAttemptToRestoreViewStateTimeout)
            clearTimeout(this._finalAttemptToRestoreViewStateTimeout);

        if (relaxedMatchDelay === 0)
            return;

        function finalAttemptToRestoreViewStateFromCookie()
        {
            this._finalAttemptToRestoreViewStateTimeout = undefined;

            this._checkOutlinesForPendingViewStateCookie(true);

            this._pendingViewStateCookie = null;
            this._restoringState = false;
        }

        // If the specific tree element wasn't found, we may need to wait for the resources
        // to be registered. We try one last time (match type only) after an arbitrary amount of timeout.
        this._finalAttemptToRestoreViewStateTimeout = setTimeout(finalAttemptToRestoreViewStateFromCookie.bind(this), relaxedMatchDelay);
    }

    showEmptyContentPlaceholder(message, treeOutline)
    {
        console.assert(message);

        treeOutline = treeOutline || this._contentTreeOutline;

        let emptyContentPlaceholderElement = this._createEmptyContentPlaceholderIfNeeded(treeOutline);
        if (emptyContentPlaceholderElement.parentNode && emptyContentPlaceholderElement.children[0].textContent === message)
            return;

        emptyContentPlaceholderElement.children[0].textContent = message;

        let emptyContentPlaceholderParentElement = treeOutline.element.parentNode;
        emptyContentPlaceholderParentElement.appendChild(emptyContentPlaceholderElement);

        this._updateContentOverflowShadowVisibility();
    }

    hideEmptyContentPlaceholder(treeOutline)
    {
        treeOutline = treeOutline || this._contentTreeOutline;

        let emptyContentPlaceholderElement = this._emptyContentPlaceholderElements.get(treeOutline);
        if (!emptyContentPlaceholderElement || !emptyContentPlaceholderElement.parentNode)
            return;

        emptyContentPlaceholderElement.remove();

        this._updateContentOverflowShadowVisibility();
    }

    updateEmptyContentPlaceholder(message, treeOutline)
    {
        treeOutline = treeOutline || this._contentTreeOutline;

        if (!treeOutline.children.length) {
            // No tree elements, so no results.
            this.showEmptyContentPlaceholder(message, treeOutline);
        } else if (!this._emptyFilterResults.get(treeOutline)) {
            // There are tree elements, and not all of them are hidden by the filter.
            this.hideEmptyContentPlaceholder(treeOutline);
        }
    }

    updateFilter()
    {
        this._updateFilter();
    }

    shouldFilterPopulate()
    {
        // Overriden by subclasses if needed.
        return this.hasCustomFilters();
    }

    hasCustomFilters()
    {
        // Implemented by subclasses if needed.
        return false;
    }

    matchTreeElementAgainstCustomFilters(treeElement)
    {
        // Implemented by subclasses if needed.
        return true;
    }

    matchTreeElementAgainstFilterFunctions(treeElement)
    {
        if (!this._filterFunctions || !this._filterFunctions.length)
            return true;

        for (var filterFunction of this._filterFunctions) {
            if (filterFunction(treeElement))
                return true;
        }

        return false;
    }

    applyFiltersToTreeElement(treeElement)
    {
        if (!this._filterBar.hasActiveFilters() && !this.hasCustomFilters()) {
            // No filters, so make everything visible.
            treeElement.hidden = false;

            // If this tree element was expanded during filtering, collapse it again.
            if (treeElement.expanded && treeElement[WebInspector.NavigationSidebarPanel.WasExpandedDuringFilteringSymbol]) {
                treeElement[WebInspector.NavigationSidebarPanel.WasExpandedDuringFilteringSymbol] = false;
                treeElement.collapse();
            }

            return;
        }

        var filterableData = treeElement.filterableData || {};

        var flags = {expandTreeElement: false};
        var filterRegex = this._textFilterRegex;

        function matchTextFilter(inputs)
        {
            if (!inputs || !filterRegex)
                return true;

            console.assert(inputs instanceof Array, "filterableData.text should be an array of text inputs");

            // Loop over all the inputs and try to match them.
            for (var input of inputs) {
                if (!input)
                    continue;
                if (filterRegex.test(input)) {
                    flags.expandTreeElement = true;
                    return true;
                }
            }

            // No inputs matched.
            return false;
        }

        function makeVisible()
        {
            // Make this element visible.
            treeElement.hidden = false;

            // Make the ancestors visible and expand them.
            var currentAncestor = treeElement.parent;
            while (currentAncestor && !currentAncestor.root) {
                currentAncestor.hidden = false;

                // Only expand if the built-in filters matched, not custom filters.
                if (flags.expandTreeElement && !currentAncestor.expanded) {
                    currentAncestor.__wasExpandedDuringFiltering = true;
                    currentAncestor.expand();
                }

                currentAncestor = currentAncestor.parent;
            }
        }

        let suppressFiltering = treeElement[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol];

        if (suppressFiltering || (matchTextFilter(filterableData.text) && this.matchTreeElementAgainstFilterFunctions(treeElement, flags) && this.matchTreeElementAgainstCustomFilters(treeElement, flags))) {
            // Make this element visible since it matches.
            makeVisible();

            // If this tree element didn't match a built-in filter and was expanded earlier during filtering, collapse it again.
            if (!flags.expandTreeElement && treeElement.expanded && treeElement[WebInspector.NavigationSidebarPanel.WasExpandedDuringFilteringSymbol]) {
                treeElement[WebInspector.NavigationSidebarPanel.WasExpandedDuringFilteringSymbol] = false;
                treeElement.collapse();
            }

            return;
        }

        // Make this element invisible since it does not match.
        treeElement.hidden = true;
    }

    treeElementAddedOrChanged(treeElement)
    {
        // Implemented by subclasses if needed.
    }

    show()
    {
        if (!this.parentSidebar)
            return;

        super.show();

        let treeOutline = this._selectedContentTreeOutline;
        if (!treeOutline && this._visibleContentTreeOutlines.length)
            treeOutline = this._visibleContentTreeOutlines[0];

        if (treeOutline)
            treeOutline.element.focus();
    }

    shown()
    {
        super.shown();

        this._updateContentOverflowShadowVisibility();
    }

    // Protected

    representedObjectWasFiltered(representedObject, filtered)
    {
        // Implemented by subclasses if needed.
    }

    pruneStaleResourceTreeElements()
    {
        if (this._checkForStaleResourcesTimeoutIdentifier) {
            clearTimeout(this._checkForStaleResourcesTimeoutIdentifier);
            this._checkForStaleResourcesTimeoutIdentifier = undefined;
        }

        for (var contentTreeOutline of this._visibleContentTreeOutlines) {
            // Check all the ResourceTreeElements at the top level to make sure their Resource still has a parentFrame in the frame hierarchy.
            // If the parentFrame is no longer in the frame hierarchy we know it was removed due to a navigation or some other page change and
            // we should remove the issues for that resource.
            for (var i = contentTreeOutline.children.length - 1; i >= 0; --i) {
                var treeElement = contentTreeOutline.children[i];
                if (!(treeElement instanceof WebInspector.ResourceTreeElement))
                    continue;

                var resource = treeElement.resource;
                if (!resource.parentFrame || resource.parentFrame.isDetached())
                    contentTreeOutline.removeChildAtIndex(i, true, true);
            }
        }
    }

    // Private
    
    _updateContentOverflowShadowVisibilitySoon()
    {
        if (this._updateContentOverflowShadowVisibilityIdentifier)
            return;

        this._updateContentOverflowShadowVisibilityIdentifier = setTimeout(this._updateContentOverflowShadowVisibility.bind(this), 0);
    }

    _updateContentOverflowShadowVisibility()
    {
        this._updateContentOverflowShadowVisibilityIdentifier = undefined;

        let scrollHeight = this.contentView.element.scrollHeight;
        let offsetHeight = this.contentView.element.offsetHeight;

        if (scrollHeight < offsetHeight) {
            if (this._topOverflowShadowElement)
                this._topOverflowShadowElement.style.opacity = 0;
            this._bottomOverflowShadowElement.style.opacity = 0;
            return;
        }

        let edgeThreshold = 1;
        let scrollTop = this.contentView.element.scrollTop;

        let topCoverage = Math.min(scrollTop, edgeThreshold);
        let bottomCoverage = Math.max(0, (offsetHeight + scrollTop) - (scrollHeight - edgeThreshold));

        if (this._topOverflowShadowElement)
            this._topOverflowShadowElement.style.opacity = (topCoverage / edgeThreshold).toFixed(1);
        this._bottomOverflowShadowElement.style.opacity = (1 - (bottomCoverage / edgeThreshold)).toFixed(1);
    }

    _checkForEmptyFilterResults()
    {
        function checkTreeOutlineForEmptyFilterResults(treeOutline)
        {
            // No tree elements, so don't touch the empty content placeholder.
            if (!treeOutline.children.length)
                return;

            // Iterate over all the top level tree elements. If any filterable elements are visible, return early.
            let filterableTreeElementFound = false;
            let unfilteredTreeElementFound = false;
            let currentTreeElement = treeOutline.children[0];
            while (currentTreeElement) {
                let suppressFilteringForTreeElement = currentTreeElement[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol];
                if (!suppressFilteringForTreeElement) {
                    filterableTreeElementFound = true;

                    if (!currentTreeElement.hidden) {
                        unfilteredTreeElementFound = true;
                        break;
                    }
                }

                currentTreeElement = currentTreeElement.nextSibling;
            }

            if (unfilteredTreeElementFound || !filterableTreeElementFound) {
                this.hideEmptyContentPlaceholder(treeOutline);
                this._emptyFilterResults.set(treeOutline, false);
                return;
            }

            // All top level tree elements are hidden, so filtering hid everything. Show a message.
            this.showEmptyContentPlaceholder(WebInspector.UIString("No Filter Results"), treeOutline);
            this._emptyFilterResults.set(treeOutline, true);
        }

        for (let treeOutline of this._visibleContentTreeOutlines) {
            if (treeOutline[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol])
                continue;

            checkTreeOutlineForEmptyFilterResults.call(this, treeOutline);
        }
    }

    _filterDidChange()
    {
        this._updateFilter();
    }

    _updateFilter()
    {
        let selectedTreeElement;
        for (let treeOutline of this.visibleContentTreeOutlines) {
            if (treeOutline.hidden || treeOutline[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol])
                continue;

            selectedTreeElement = treeOutline.selectedTreeElement;
            if (selectedTreeElement)
                break;
        }

        let selectionWasHidden = selectedTreeElement && selectedTreeElement.hidden;

        let filters = this._filterBar.filters;
        this._textFilterRegex = simpleGlobStringToRegExp(filters.text, "i");
        this._filtersSetting.value = filters;
        this._filterFunctions = filters.functions;

        // Don't populate if we don't have any active filters.
        // We only need to populate when a filter needs to reveal.
        let dontPopulate = !this._filterBar.hasActiveFilters() && !this.shouldFilterPopulate();

        // Update all trees that allow filtering.
        for (let treeOutline of this.visibleContentTreeOutlines) {
            if (treeOutline.hidden || treeOutline[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol])
                continue;

            let currentTreeElement = treeOutline.children[0];
            while (currentTreeElement && !currentTreeElement.root) {
                if (!currentTreeElement[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol]) {
                    const currentTreeElementWasHidden = currentTreeElement.hidden;
                    this.applyFiltersToTreeElement(currentTreeElement);
                    if (currentTreeElementWasHidden !== currentTreeElement.hidden)
                        this.representedObjectWasFiltered(currentTreeElement.representedObject, currentTreeElement.hidden);
                }

                currentTreeElement = currentTreeElement.traverseNextTreeElement(false, null, dontPopulate);
            }
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibility();

        // Filter may have hidden the selected resource in the timeline view, which should now notify its listeners.
        // FIXME: This is a layering violation. This should at least be in TimelineSidebarPanel.
        if (selectedTreeElement && selectedTreeElement.hidden !== selectionWasHidden) {
            var currentContentView = this.contentBrowser.currentContentView;
            if (currentContentView instanceof WebInspector.TimelineRecordingContentView && typeof currentContentView.currentTimelineView.filterUpdated === "function")
                currentContentView.currentTimelineView.filterUpdated();
        }
    }

    _treeElementAddedOrChanged(event)
    {
        // Don't populate if we don't have any active filters.
        // We only need to populate when a filter needs to reveal.
        var dontPopulate = !this._filterBar.hasActiveFilters() && !this.shouldFilterPopulate();

        // Apply the filters to the tree element and its descendants.
        let treeElement = event.data.element;
        let currentTreeElement = treeElement;
        while (currentTreeElement && !currentTreeElement.root) {
            if (!currentTreeElement[WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol]) {
                const currentTreeElementWasHidden = currentTreeElement.hidden;
                this.applyFiltersToTreeElement(currentTreeElement);
                if (currentTreeElementWasHidden !== currentTreeElement.hidden)
                    this.representedObjectWasFiltered(currentTreeElement.representedObject, currentTreeElement.hidden);
            }

            currentTreeElement = currentTreeElement.traverseNextTreeElement(false, treeElement, dontPopulate);
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibilitySoon();

        if (this.selected)
            this._checkElementsForPendingViewStateCookie([treeElement]);

        this.treeElementAddedOrChanged(treeElement);
    }

    _treeElementDisclosureDidChange(event)
    {
        this._updateContentOverflowShadowVisibility();
    }

    _treeSelectionDidChange(event)
    {
        let selectedElement = event.data.selectedElement;
        this._selectedContentTreeOutline = selectedElement ? selectedElement.treeOutline : null;
    }

    _checkForStaleResourcesIfNeeded()
    {
        if (!this._checkForStaleResourcesTimeoutIdentifier || !this._shouldAutoPruneStaleTopLevelResourceTreeElements)
            return;
        this.pruneStaleResourceTreeElements();
    }

    _checkForStaleResources(event)
    {
        console.assert(this._shouldAutoPruneStaleTopLevelResourceTreeElements);

        if (this._checkForStaleResourcesTimeoutIdentifier)
            return;

        // Check on a delay to coalesce multiple calls to _checkForStaleResources.
        this._checkForStaleResourcesTimeoutIdentifier = setTimeout(this.pruneStaleResourceTreeElements.bind(this));
    }

    _isTreeElementWithoutRepresentedObject(treeElement)
    {
        return treeElement instanceof WebInspector.FolderTreeElement
            || treeElement instanceof WebInspector.DatabaseHostTreeElement
            || treeElement instanceof WebInspector.IndexedDatabaseHostTreeElement
            || typeof treeElement.representedObject === "string"
            || treeElement.representedObject instanceof String;
    }

    _checkOutlinesForPendingViewStateCookie(matchTypeOnly)
    {
        if (!this._pendingViewStateCookie)
            return;

        this._checkForStaleResourcesIfNeeded();

        var visibleTreeElements = [];
        this._visibleContentTreeOutlines.forEach(function(outline) {
            var currentTreeElement = outline.hasChildren ? outline.children[0] : null;
            while (currentTreeElement) {
                visibleTreeElements.push(currentTreeElement);
                currentTreeElement = currentTreeElement.traverseNextTreeElement(false, null, false);
            }
        });

        return this._checkElementsForPendingViewStateCookie(visibleTreeElements, matchTypeOnly);
    }

    _checkElementsForPendingViewStateCookie(treeElements, matchTypeOnly)
    {
        if (!this._pendingViewStateCookie)
            return;

        var cookie = this._pendingViewStateCookie;

        function treeElementMatchesCookie(treeElement)
        {
            if (this._isTreeElementWithoutRepresentedObject(treeElement))
                return false;

            var representedObject = treeElement.representedObject;
            if (!representedObject)
                return false;

            var typeIdentifier = cookie[WebInspector.TypeIdentifierCookieKey];
            if (typeIdentifier !== representedObject.constructor.TypeIdentifier)
                return false;

            if (matchTypeOnly)
                return true;

            var candidateObjectCookie = {};
            if (representedObject.saveIdentityToCookie)
                representedObject.saveIdentityToCookie(candidateObjectCookie);

            var candidateCookieKeys = Object.keys(candidateObjectCookie);
            return candidateCookieKeys.length && candidateCookieKeys.every(function valuesMatchForKey(key) {
                return candidateObjectCookie[key] === cookie[key];
            });
        }

        var matchedElement = null;
        treeElements.some(function(element) {
            if (treeElementMatchesCookie.call(this, element)) {
                matchedElement = element;
                return true;
            }
        }, this);

        if (matchedElement) {
            this.showDefaultContentViewForTreeElement(matchedElement);

            this._pendingViewStateCookie = null;

            // Delay clearing the restoringState flag until the next runloop so listeners
            // checking for it in this runloop still know state was being restored.
            setTimeout(function() {
                this._restoringState = false;
            }.bind(this));

            if (this._finalAttemptToRestoreViewStateTimeout) {
                clearTimeout(this._finalAttemptToRestoreViewStateTimeout);
                this._finalAttemptToRestoreViewStateTimeout = undefined;
            }
        }
    }

    _createEmptyContentPlaceholderIfNeeded(treeOutline)
    {
        let emptyContentPlaceholderElement = this._emptyContentPlaceholderElements.get(treeOutline);
        if (emptyContentPlaceholderElement)
            return emptyContentPlaceholderElement;

        emptyContentPlaceholderElement = document.createElement("div");
        emptyContentPlaceholderElement.classList.add(WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName);
        this._emptyContentPlaceholderElements.set(treeOutline, emptyContentPlaceholderElement);

        let emptyContentPlaceholderMessageElement = document.createElement("div");
        emptyContentPlaceholderMessageElement.className = WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName;
        emptyContentPlaceholderElement.appendChild(emptyContentPlaceholderMessageElement);

        return emptyContentPlaceholderElement;
    }
};

WebInspector.NavigationSidebarPanel.SuppressFilteringSymbol = Symbol("suppress-filtering");
WebInspector.NavigationSidebarPanel.WasExpandedDuringFilteringSymbol = Symbol("was-expanded-during-filtering");

WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName = "overflow-shadow";
WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName = "top";
WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName = "navigation-sidebar-panel-content-tree-outline";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName = "empty-content-placeholder";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName = "message";
