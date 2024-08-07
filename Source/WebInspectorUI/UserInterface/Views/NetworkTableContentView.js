/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

WI.NetworkTableContentView = class NetworkTableContentView extends WI.ContentView
{
    constructor(representedObject, extraArguments)
    {
        super(representedObject);

        this._entries = [];
        this._entriesSortComparator = null;
        this._filteredEntries = [];
        this._pendingInsertions = [];
        this._pendingUpdates = [];
        this._pendingFilter = false;

        this._table = null;
        this._nameColumnWidthSetting = new WI.Setting("network-table-content-view-name-column-width", 250);

        this._selectedResource = null;
        this._resourceDetailView = null;
        this._resourceDetailViewMap = new Map;

        this._waterfallStartTime = NaN;
        this._waterfallEndTime = NaN;
        this._waterfallTimelineRuler = null;
        this._waterfallPopover = null;

        // FIXME: Network Timeline.
        // FIXME: Throttling.

        const exclusive = true;
        this._typeFilterScopeBarItemAll = new WI.ScopeBarItem("network-type-filter-all", WI.UIString("All"), exclusive);
        let typeFilterScopeBarItems = [this._typeFilterScopeBarItemAll];

        let uniqueTypes = [
            ["Document", (type) => type === WI.Resource.Type.Document],
            ["Stylesheet", (type) => type === WI.Resource.Type.Stylesheet],
            ["Image", (type) => type === WI.Resource.Type.Image],
            ["Font", (type) => type === WI.Resource.Type.Font],
            ["Script", (type) => type === WI.Resource.Type.Script],
            ["XHR", (type) => type === WI.Resource.Type.XHR || type === WI.Resource.Type.Fetch],
            ["Other", (type) => {
                return type !== WI.Resource.Type.Document
                    && type !== WI.Resource.Type.Stylesheet
                    && type !== WI.Resource.Type.Image
                    && type !== WI.Resource.Type.Font
                    && type !== WI.Resource.Type.Script
                    && type !== WI.Resource.Type.XHR
                    && type !== WI.Resource.Type.Fetch;
            }],
        ];
        for (let [key, checker] of uniqueTypes) {
            let type = WI.Resource.Type[key];
            let scopeBarItem = new WI.ScopeBarItem("network-type-filter-" + key, WI.NetworkTableContentView.shortDisplayNameForResourceType(type));
            scopeBarItem.__checker = checker;
            typeFilterScopeBarItems.push(scopeBarItem);
        }

        this._typeFilterScopeBar = new WI.ScopeBar("network-type-filter-scope-bar", typeFilterScopeBarItems, typeFilterScopeBarItems[0]);
        this._typeFilterScopeBar.addEventListener(WI.ScopeBar.Event.SelectionChanged, this._typeFilterScopeBarSelectionChanged, this);

        this._urlFilterSearchText = null;
        this._urlFilterSearchRegex = null;
        this._urlFilterIsActive = false;

        this._urlFilterNavigationItem = new WI.FilterBarNavigationItem;
        this._urlFilterNavigationItem.filterBar.addEventListener(WI.FilterBar.Event.FilterDidChange, this._urlFilterDidChange, this);
        this._urlFilterNavigationItem.filterBar.placeholder = WI.UIString("Filter Full URL");

        this._activeTypeFilters = this._generateTypeFilter();
        this._activeURLFilterResources = new Set;

        this._emptyFilterResultsMessageElement = null;

        this._clearOnLoadNavigationItem = new WI.CheckboxNavigationItem("perserve-log", WI.UIString("Preserve Log"), !WI.settings.clearNetworkOnNavigate.value);
        this._clearOnLoadNavigationItem.tooltip = WI.UIString("Do not clear network items on new page loads");
        this._clearOnLoadNavigationItem.addEventListener(WI.CheckboxNavigationItem.Event.CheckedDidChange, () => { WI.settings.clearNetworkOnNavigate.value = !WI.settings.clearNetworkOnNavigate.value; });
        WI.settings.clearNetworkOnNavigate.addEventListener(WI.Setting.Event.Changed, this._clearNetworkOnNavigateSettingChanged, this);

        this._harExportNavigationItem = new WI.ButtonNavigationItem("har-export", WI.UIString("Export"), "Images/Export.svg", 15, 15);
        this._harExportNavigationItem.buttonStyle = WI.ButtonNavigationItem.Style.ImageAndText;
        this._harExportNavigationItem.tooltip = WI.UIString("HAR Export (%s)").format(WI.saveKeyboardShortcut.displayName);
        this._harExportNavigationItem.addEventListener(WI.ButtonNavigationItem.Event.Clicked, () => { this._exportHAR(); });

        this._checkboxsNavigationItemGroup = new WI.GroupNavigationItem([this._clearOnLoadNavigationItem, new WI.DividerNavigationItem]);
        this._checkboxsNavigationItemGroup.visibilityPriority = WI.NavigationItem.VisibilityPriority.Low;

        this._buttonsNavigationItemGroup = new WI.GroupNavigationItem([this._harExportNavigationItem, new WI.DividerNavigationItem]);
        this._buttonsNavigationItemGroup.visibilityPriority = WI.NavigationItem.VisibilityPriority.Low;

        // COMPATIBILITY (iOS 10.3): Network.setDisableResourceCaching did not exist.
        if (window.NetworkAgent && NetworkAgent.setResourceCachingDisabled) {
            let toolTipForDisableResourceCache = WI.UIString("Ignore the resource cache when loading resources");
            let activatedToolTipForDisableResourceCache = WI.UIString("Use the resource cache when loading resources");
            this._disableResourceCacheNavigationItem = new WI.ActivateButtonNavigationItem("disable-resource-cache", toolTipForDisableResourceCache, activatedToolTipForDisableResourceCache, "Images/IgnoreCaches.svg", 16, 16);
            this._disableResourceCacheNavigationItem.activated = WI.resourceCachingDisabledSetting.value;

            this._disableResourceCacheNavigationItem.addEventListener(WI.ButtonNavigationItem.Event.Clicked, this._toggleDisableResourceCache, this);
            WI.resourceCachingDisabledSetting.addEventListener(WI.Setting.Event.Changed, this._resourceCachingDisabledSettingChanged, this);
        }

        this._clearNetworkItemsNavigationItem = new WI.ButtonNavigationItem("clear-network-items", WI.UIString("Clear Network Items (%s)").format(WI.clearKeyboardShortcut.displayName), "Images/NavigationItemTrash.svg", 15, 15);
        this._clearNetworkItemsNavigationItem.addEventListener(WI.ButtonNavigationItem.Event.Clicked, () => { this.reset(); });

        WI.Frame.addEventListener(WI.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
        WI.Resource.addEventListener(WI.Resource.Event.LoadingDidFinish, this._resourceLoadingDidFinish, this);
        WI.Resource.addEventListener(WI.Resource.Event.LoadingDidFail, this._resourceLoadingDidFail, this);
        WI.Resource.addEventListener(WI.Resource.Event.TransferSizeDidChange, this._resourceTransferSizeDidChange, this);
        WI.frameResourceManager.addEventListener(WI.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);
        WI.timelineManager.persistentNetworkTimeline.addEventListener(WI.Timeline.Event.RecordAdded, this._networkTimelineRecordAdded, this);

        this._needsInitialPopulate = true;
    }

    // Static

    static displayNameForResource(resource)
    {
        if (resource.type === WI.Resource.Type.Image || resource.type === WI.Resource.Type.Font) {
            let fileExtension;
            if (resource.mimeType)
                fileExtension = WI.fileExtensionForMIMEType(resource.mimeType);
            if (!fileExtension)
                fileExtension = WI.fileExtensionForURL(resource.url);
            if (fileExtension)
                return fileExtension;
        }

        return WI.NetworkTableContentView.shortDisplayNameForResourceType(resource.type).toLowerCase();
    }

    static shortDisplayNameForResourceType(type)
    {
        switch (type) {
        case WI.Resource.Type.Document:
            return WI.UIString("Document");
        case WI.Resource.Type.Stylesheet:
            return "CSS";
        case WI.Resource.Type.Image:
            return WI.UIString("Image");
        case WI.Resource.Type.Font:
            return WI.UIString("Font");
        case WI.Resource.Type.Script:
            return "JS";
        case WI.Resource.Type.XHR:
            return "XHR";
        case WI.Resource.Type.Fetch:
            return WI.UIString("Fetch");
        case WI.Resource.Type.Ping:
            return WI.UIString("Ping");
        case WI.Resource.Type.Beacon:
            return WI.UIString("Beacon");
        case WI.Resource.Type.WebSocket:
        case WI.Resource.Type.Other:
            return WI.UIString("Other");
        default:
            console.error("Unknown resource type", type);
            return null;
        }
    }

    // Public

    get selectionPathComponents()
    {
        return null;
    }

    get navigationItems()
    {
        let items = [this._checkboxsNavigationItemGroup, this._buttonsNavigationItemGroup];
        if (this._disableResourceCacheNavigationItem)
            items.push(this._disableResourceCacheNavigationItem);
        items.push(this._clearNetworkItemsNavigationItem);
        return items;
    }

    get filterNavigationItems()
    {
        return [this._urlFilterNavigationItem, this._typeFilterScopeBar];
    }

    get supportsSave()
    {
        return this._filteredEntries.some((entry) => entry.resource.finished);
    }

    get saveData()
    {
        return {customSaveHandler: () => { this._exportHAR(); }};
    }

    shown()
    {
        super.shown();

        if (this._resourceDetailView)
            this._resourceDetailView.shown();

        if (this._table)
            this._table.restoreScrollPosition();
    }

    hidden()
    {
        this._hidePopover();

        if (this._resourceDetailView)
            this._resourceDetailView.hidden();

        super.hidden();
    }

    closed()
    {
        for (let detailView of this._resourceDetailViewMap.values())
            detailView.dispose();
        this._resourceDetailViewMap.clear();

        this._hidePopover();
        this._hideResourceDetailView();

        WI.Frame.removeEventListener(null, null, this);
        WI.Resource.removeEventListener(null, null, this);
        WI.resourceCachingDisabledSetting.removeEventListener(null, null, this);
        WI.settings.clearNetworkOnNavigate.removeEventListener(null, null, this);
        WI.frameResourceManager.removeEventListener(WI.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);
        WI.timelineManager.persistentNetworkTimeline.removeEventListener(WI.Timeline.Event.RecordAdded, this._networkTimelineRecordAdded, this);

        super.closed();
    }

    reset()
    {
        this._entries = [];
        this._filteredEntries = [];
        this._pendingInsertions = [];

        for (let detailView of this._resourceDetailViewMap.values())
            detailView.dispose();
        this._resourceDetailViewMap.clear();

        this._waterfallStartTime = NaN;
        this._waterfallEndTime = NaN;
        this._updateWaterfallTimelineRuler();
        this._updateExportButton();

        if (this._table) {
            this._selectedResource = null;
            this._table.clearSelectedRow();
            this._table.reloadData();
            this._hidePopover();
            this._hideResourceDetailView();
        }
    }

    showRepresentedObject(representedObject, cookie)
    {
        console.assert(representedObject instanceof WI.Resource);

        let rowIndex = this._rowIndexForResource(representedObject);
        if (rowIndex === -1) {
            this._selectedResource = null;
            this._table.clearSelectedRow();
            this._hideResourceDetailView();
            return;
        }

        this._table.selectRow(rowIndex);
    }

    // NetworkResourceDetailView delegate

    networkResourceDetailViewClose(resourceDetailView)
    {
        this._selectedResource = null;
        this._table.clearSelectedRow();
        this._hideResourceDetailView();
    }

    // Table dataSource

    tableNumberOfRows(table)
    {
        return this._filteredEntries.length;
    }

    tableSortChanged(table)
    {
        this._generateSortComparator();

        if (!this._entriesSortComparator)
            return;

        this._hideResourceDetailView();

        this._entries = this._entries.sort(this._entriesSortComparator);
        this._updateFilteredEntries();
        this._table.reloadData();
    }

    // Table delegate

    tableCellMouseDown(table, cell, column, rowIndex, event)
    {
        if (column !== this._nameColumn)
            return;

        this._table.selectRow(rowIndex);
    }

    tableCellContextMenuClicked(table, cell, column, rowIndex, event)
    {
        if (column !== this._nameColumn)
            return;

        this._table.selectRow(rowIndex);

        let entry = this._filteredEntries[rowIndex];
        let contextMenu = WI.ContextMenu.createFromEvent(event);
        WI.appendContextMenuItemsForSourceCode(contextMenu, entry.resource);

        contextMenu.appendSeparator();
        contextMenu.appendItem(WI.UIString("Export HAR"), () => { this._exportHAR(); });
    }

    tableSelectedRowChanged(table, rowIndex)
    {
        if (isNaN(rowIndex)) {
            this._selectedResource = null;
            this._hideResourceDetailView();
            return;
        }

        let entry = this._filteredEntries[rowIndex];
        if (entry.resource === this._selectedResource)
            return;

        this._selectedResource = entry.resource;
        this._showResourceDetailView(this._selectedResource);
    }

    tablePopulateCell(table, cell, column, rowIndex)
    {
        let entry = this._filteredEntries[rowIndex];

        cell.classList.toggle("error", entry.resource.hadLoadingError());

        switch (column.identifier) {
        case "name":
            this._populateNameCell(cell, entry);
            break;
        case "domain":
            this._populateDomainCell(cell, entry);
            break;
        case "type":
            cell.textContent = entry.displayType || emDash;
            break;
        case "mimeType":
            cell.textContent = entry.mimeType || emDash;
            break;
        case "method":
            cell.textContent = entry.method || emDash;
            break;
        case "scheme":
            cell.textContent = entry.scheme || emDash;
            break;
        case "status":
            cell.textContent = entry.status || emDash;
            break;
        case "protocol":
            cell.textContent = entry.protocol || emDash;
            break;
        case "priority":
            cell.textContent = WI.Resource.displayNameForPriority(entry.priority) || emDash;
            break;
        case "remoteAddress":
            cell.textContent = entry.remoteAddress || emDash;
            break;
        case "connectionIdentifier":
            cell.textContent = entry.connectionIdentifier || emDash;
            break;
        case "resourceSize":
            cell.textContent = isNaN(entry.resourceSize) ? emDash : Number.bytesToString(entry.resourceSize);
            break;
        case "transferSize":
            this._populateTransferSizeCell(cell, entry);
            break;
        case "time":
            // FIXME: <https://webkit.org/b/176748> Web Inspector: Frontend sometimes receives resources with negative duration (responseEnd - requestStart)
            cell.textContent = isNaN(entry.time) ? emDash : Number.secondsToString(Math.max(entry.time, 0));
            break;
        case "waterfall":
            this._populateWaterfallGraph(cell, entry);
            break;
        }

        return cell;
    }

    // Private

    _populateNameCell(cell, entry)
    {
        console.assert(!cell.firstChild, "We expect the cell to be empty.", cell, cell.firstChild);

        let resource = entry.resource;
        if (resource.isLoading()) {
            let statusElement = cell.appendChild(document.createElement("div"));
            statusElement.className = "status";
            let spinner = new WI.IndeterminateProgressSpinner;
            statusElement.appendChild(spinner.element);
        }

        let iconElement = cell.appendChild(document.createElement("img"));
        iconElement.className = "icon";
        cell.classList.add(WI.ResourceTreeElement.ResourceIconStyleClassName, entry.resource.type);

        let nameElement = cell.appendChild(document.createElement("span"));
        nameElement.textContent = entry.name;
    }

    _populateDomainCell(cell, entry)
    {
        console.assert(!cell.firstChild, "We expect the cell to be empty.", cell, cell.firstChild);

        if (!entry.domain) {
            cell.textContent = emDash;
            return;
        }

        let secure = entry.scheme === "https" || entry.scheme === "wss";
        if (secure) {
            let lockIconElement = cell.appendChild(document.createElement("img"));
            lockIconElement.className = "lock";
        }

        cell.append(entry.domain);
    }

    _populateTransferSizeCell(cell, entry)
    {
        let responseSource = entry.resource.responseSource;
        if (responseSource === WI.Resource.ResponseSource.MemoryCache) {
            cell.classList.add("cache-type");
            cell.textContent = WI.UIString("(memory)");
            return;
        }
        if (responseSource === WI.Resource.ResponseSource.DiskCache) {
            cell.classList.add("cache-type");
            cell.textContent = WI.UIString("(disk)");
            return;
        }
        if (responseSource === WI.Resource.ResponseSource.ServiceWorker) {
            cell.classList.add("cache-type");
            cell.textContent = WI.UIString("(service worker)");
            return;
        }

        let transferSize = entry.transferSize;
        cell.textContent = isNaN(transferSize) ? emDash : Number.bytesToString(transferSize);
        console.assert(!cell.classList.contains("cache-type"), "Should not have cache-type class on cell.");
    }

    _populateWaterfallGraph(cell, entry)
    {
        cell.removeChildren();

        let resource = entry.resource;
        if (!resource.hasResponse()) {
            cell.textContent = zeroWidthSpace;
            return;
        }

        let {startTime, domainLookupStart, domainLookupEnd, connectStart, connectEnd, secureConnectionStart, requestStart, responseStart, responseEnd} = resource.timingData;
        if (isNaN(startTime)) {
            cell.textContent = zeroWidthSpace;
            return;
        }

        let graphStartTime = this._waterfallTimelineRuler.startTime;
        if (responseEnd < graphStartTime) {
            cell.textContent = zeroWidthSpace;
            return;
        }

        let graphEndTime = this._waterfallTimelineRuler.endTime;
        if (startTime > graphEndTime) {
            cell.textContent = zeroWidthSpace;
            return;
        }

        let secondsPerPixel = this._waterfallTimelineRuler.secondsPerPixel;

        let container = cell.appendChild(document.createElement("div"));
        container.className = "waterfall-container";

        function appendBlock(startTime, endTime, className) {
            let startOffset = (startTime - graphStartTime) / secondsPerPixel;
            let width = (endTime - startTime) / secondsPerPixel;
            let block = container.appendChild(document.createElement("div"));
            block.classList.add("block", className);
            let styleAttribute = WI.resolvedLayoutDirection() === WI.LayoutDirection.LTR ? "left" : "right";
            block.style[styleAttribute] = startOffset + "px";
            block.style.width = width + "px";
            return block;
        }

        // Mouse block sits on top and accepts mouse events on this group.
        let padSeconds = 10 * secondsPerPixel;
        let mouseBlock = appendBlock(startTime - padSeconds, responseEnd + padSeconds, "mouse-tracking");
        mouseBlock.addEventListener("mousedown", (event) => {
            if (event.button !== 0 || event.ctrlKey)
                return;
            this._handleMousedownWaterfall(mouseBlock, entry, event);
        });

        // Super small visualization.
        let totalWidth = (responseEnd - startTime) / secondsPerPixel;
        if (totalWidth <= 3) {
            appendBlock(startTime, requestStart, "queue");
            appendBlock(startTime, responseEnd, "response");
            return;
        }

        // Each component.
        if (domainLookupStart) {
            appendBlock(startTime, domainLookupStart, "queue");
            appendBlock(domainLookupStart, connectStart || requestStart, "dns");
        } else if (connectStart)
            appendBlock(startTime, connectStart, "queue");
        else if (requestStart)
            appendBlock(startTime, requestStart, "queue");
        if (connectStart)
            appendBlock(connectStart, connectEnd, "connect");
        if (secureConnectionStart)
            appendBlock(secureConnectionStart, connectEnd, "secure");
        appendBlock(requestStart, responseStart, "request");
        appendBlock(responseStart, responseEnd, "response");
    }

    _generateSortComparator()
    {
        let sortColumnIdentifier = this._table.sortColumnIdentifier;
        if (!sortColumnIdentifier) {
            this._entriesSortComparator = null;
            return;
        }

        let comparator;

        switch (sortColumnIdentifier) {
        case "name":
        case "domain":
        case "mimeType":
        case "method":
        case "scheme":
        case "protocol":
        case "remoteAddress":
            // Simple string.
            comparator = (a, b) => (a[sortColumnIdentifier] || "").extendedLocaleCompare(b[sortColumnIdentifier] || "");
            break;

        case "status":
        case "connectionIdentifier":
        case "resourceSize":
        case "time":
            // Simple number.
            comparator = (a, b) => {
                let aValue = a[sortColumnIdentifier];
                if (isNaN(aValue))
                    return 1;
                let bValue = b[sortColumnIdentifier];
                if (isNaN(bValue))
                    return -1;
                return aValue - bValue;
            };
            break;

        case "priority":
            // Resource.NetworkPriority enum.
            comparator = (a, b) => WI.Resource.comparePriority(a.priority, b.priority);
            break;

        case "type":
            // Sort by displayType string.
            comparator = (a, b) => (a.displayType || "").extendedLocaleCompare(b.displayType || "");
            break;

        case "transferSize":
            // Handle (memory) and (disk) values.
            comparator = (a, b) => {
                let transferSizeA = a.transferSize;
                let transferSizeB = b.transferSize;

                // Treat NaN as the largest value.
                if (isNaN(transferSizeA))
                    return 1;
                if (isNaN(transferSizeB))
                    return -1;

                // Treat memory cache and disk cache as small values.
                let sourceA = a.resource.responseSource;
                if (sourceA === WI.Resource.ResponseSource.MemoryCache)
                    transferSizeA = -20;
                else if (sourceA === WI.Resource.ResponseSource.DiskCache)
                    transferSizeA = -10;
                else if (sourceA === WI.Resource.ResponseSource.ServiceWorker)
                    transferSizeA = -5;

                let sourceB = b.resource.responseSource;
                if (sourceB === WI.Resource.ResponseSource.MemoryCache)
                    transferSizeB = -20;
                else if (sourceB === WI.Resource.ResponseSource.DiskCache)
                    transferSizeB = -10;
                else if (sourceB === WI.Resource.ResponseSource.ServiceWorker)
                    transferSizeB = -5;

                return transferSizeA - transferSizeB;
            };
            break;

        case "waterfall":
            // Sort by startTime number.
            comparator = comparator = (a, b) => a.startTime - b.startTime;
            break;

        default:
            console.assert("Unexpected sort column", sortColumnIdentifier);
            return;
        }

        let reverseFactor = this._table.sortOrder === WI.Table.SortOrder.Ascending ? 1 : -1;
        this._entriesSortComparator = (a, b) => reverseFactor * comparator(a, b);
    }

    // Protected

    initialLayout()
    {
        this._waterfallTimelineRuler = new WI.TimelineRuler;
        this._waterfallTimelineRuler.allowsClippedLabels = true;

        this._nameColumn = new WI.TableColumn("name", WI.UIString("Name"), {
            minWidth: WI.Sidebar.AbsoluteMinimumWidth,
            maxWidth: 500,
            initialWidth: this._nameColumnWidthSetting.value,
            resizeType: WI.TableColumn.ResizeType.Locked,
        });

        this._domainColumn = new WI.TableColumn("domain", WI.UIString("Domain"), {
            minWidth: 120,
            maxWidth: 200,
            initialWidth: 150,
        });

        this._typeColumn = new WI.TableColumn("type", WI.UIString("Type"), {
            minWidth: 70,
            maxWidth: 120,
            initialWidth: 90,
        });

        this._mimeTypeColumn = new WI.TableColumn("mimeType", WI.UIString("MIME Type"), {
            hidden: true,
            minWidth: 100,
            maxWidth: 150,
            initialWidth: 120,
        });

        this._methodColumn = new WI.TableColumn("method", WI.UIString("Method"), {
            hidden: true,
            minWidth: 55,
            maxWidth: 80,
            initialWidth: 65,
        });

        this._schemeColumn = new WI.TableColumn("scheme", WI.UIString("Scheme"), {
            hidden: true,
            minWidth: 55,
            maxWidth: 80,
            initialWidth: 65,
        });

        this._statusColumn = new WI.TableColumn("status", WI.UIString("Status"), {
            hidden: true,
            minWidth: 50,
            maxWidth: 50,
            align: "left",
        });

        this._protocolColumn = new WI.TableColumn("protocol", WI.UIString("Protocol"), {
            hidden: true,
            minWidth: 65,
            maxWidth: 80,
            initialWidth: 75,
        });

        this._priorityColumn = new WI.TableColumn("priority", WI.UIString("Priority"), {
            hidden: true,
            minWidth: 65,
            maxWidth: 80,
            initialWidth: 70,
        });

        this._remoteAddressColumn = new WI.TableColumn("remoteAddress", WI.UIString("IP Address"), {
            hidden: true,
            minWidth: 150,
        });

        this._connectionIdentifierColumn = new WI.TableColumn("connectionIdentifier", WI.UIString("Connection ID"), {
            hidden: true,
            minWidth: 50,
            maxWidth: 120,
            initialWidth: 80,
            align: "right",
        });

        this._resourceSizeColumn = new WI.TableColumn("resourceSize", WI.UIString("Resource Size"), {
            hidden: true,
            minWidth: 80,
            maxWidth: 100,
            initialWidth: 80,
            align: "right",
        });

        this._transferSizeColumn = new WI.TableColumn("transferSize", WI.UIString("Transfer Size"), {
            minWidth: 100,
            maxWidth: 150,
            initialWidth: 100,
            align: "right",
        });

        this._timeColumn = new WI.TableColumn("time", WI.UIString("Time"), {
            minWidth: 65,
            maxWidth: 90,
            initialWidth: 65,
            align: "right",
        });

        this._waterfallColumn = new WI.TableColumn("waterfall", WI.UIString("Waterfall"), {
            minWidth: 230,
            headerView: this._waterfallTimelineRuler,
        });

        this._nameColumn.addEventListener(WI.TableColumn.Event.WidthDidChange, this._tableNameColumnDidChangeWidth, this);
        this._waterfallColumn.addEventListener(WI.TableColumn.Event.WidthDidChange, this._tableWaterfallColumnDidChangeWidth, this);

        this._table = new WI.Table("network-table", this, this, 20);

        this._table.addColumn(this._nameColumn);
        this._table.addColumn(this._domainColumn);
        this._table.addColumn(this._typeColumn);
        this._table.addColumn(this._mimeTypeColumn);
        this._table.addColumn(this._methodColumn);
        this._table.addColumn(this._schemeColumn);
        this._table.addColumn(this._statusColumn);
        this._table.addColumn(this._protocolColumn);
        this._table.addColumn(this._priorityColumn);
        this._table.addColumn(this._remoteAddressColumn);
        this._table.addColumn(this._connectionIdentifierColumn);
        this._table.addColumn(this._resourceSizeColumn);
        this._table.addColumn(this._transferSizeColumn);
        this._table.addColumn(this._timeColumn);
        this._table.addColumn(this._waterfallColumn);

        if (!this._table.sortColumnIdentifier) {
            this._table.sortOrder = WI.Table.SortOrder.Ascending;
            this._table.sortColumnIdentifier = "waterfall";
        }

        this.addSubview(this._table);
    }

    layout()
    {
        this._updateWaterfallTimelineRuler();
        this._processPendingEntries();
        this._positionDetailView();
        this._positionEmptyFilterMessage();
        this._updateExportButton();
    }

    handleClearShortcut(event)
    {
        this.reset();
    }

    // Private

    _updateWaterfallTimelineRuler()
    {
        if (!this._waterfallTimelineRuler)
            return;

        if (isNaN(this._waterfallStartTime)) {
            this._waterfallTimelineRuler.zeroTime = 0;
            this._waterfallTimelineRuler.startTime = 0;
            this._waterfallTimelineRuler.endTime = 0.250;
        } else {
            this._waterfallTimelineRuler.zeroTime = this._waterfallStartTime;
            this._waterfallTimelineRuler.startTime = this._waterfallStartTime;
            this._waterfallTimelineRuler.endTime = this._waterfallEndTime;

            // Add a little bit of padding on the each side.
            const paddingPixels = 5;
            let padSeconds = paddingPixels * this._waterfallTimelineRuler.secondsPerPixel;
            this._waterfallTimelineRuler.zeroTime = this._waterfallStartTime - padSeconds;
            this._waterfallTimelineRuler.startTime = this._waterfallStartTime - padSeconds;
            this._waterfallTimelineRuler.endTime = this._waterfallEndTime + padSeconds;
        }
    }

    _updateExportButton()
    {
        let enabled = this._filteredEntries.length > 0;
        this._harExportNavigationItem.enabled = enabled;
    }

    _processPendingEntries()
    {
        let needsSort = this._pendingUpdates.length > 0;
        let needsFilter = this._pendingFilter;

        // No global sort or filter is needed, so just insert new records into their sorted position.
        if (!needsSort && !needsFilter) {
            let originalLength = this._pendingInsertions.length;
            for (let resource of this._pendingInsertions)
                this._insertResourceAndReloadTable(resource);
            console.assert(this._pendingInsertions.length === originalLength);
            this._pendingInsertions = [];
            return;
        }

        for (let resource of this._pendingInsertions)
            this._entries.push(this._entryForResource(resource));
        this._pendingInsertions = [];

        for (let resource of this._pendingUpdates)
            this._updateEntryForResource(resource);
        this._pendingUpdates = [];

        this._pendingFilter = false;

        this._updateSortAndFilteredEntries();
        this._table.reloadData();
    }

    _populateWithInitialResourcesIfNeeded()
    {
        if (!this._needsInitialPopulate)
            return;

        this._needsInitialPopulate = false;

        let populateResourcesForFrame = (frame) => {
            if (frame.provisionalMainResource)
                this._pendingInsertions.push(frame.provisionalMainResource);
            else if (frame.mainResource)
                this._pendingInsertions.push(frame.mainResource);

            for (let resource of frame.resourceCollection.items)
                this._pendingInsertions.push(resource);

            for (let childFrame of frame.childFrameCollection.items)
                populateResourcesForFrame(childFrame);
        };

        let populateResourcesForTarget = (target) => {
            if (target.mainResource instanceof WI.Resource)
                this._pendingInsertions.push(target.mainResource);
            for (let resource of target.resourceCollection.items)
                this._pendingInsertions.push(resource);
        };

        for (let target of WI.targets) {
            if (target === WI.pageTarget)
                populateResourcesForFrame(WI.frameResourceManager.mainFrame);
            else
                populateResourcesForTarget(target);
        }

        this.needsLayout();
    }

    _checkURLFilterAgainstResource(resource)
    {
        if (this._urlFilterSearchRegex.test(resource.url))
            this._activeURLFilterResources.add(resource);
    }

    _rowIndexForResource(resource)
    {
        return this._filteredEntries.findIndex((x) => x.resource === resource);
    }

    _updateEntryForResource(resource)
    {
        let index = this._entries.findIndex((x) => x.resource === resource);
        if (index === -1)
            return;

        let entry = this._entryForResource(resource);
        this._entries[index] = entry;

        let rowIndex = this._rowIndexForResource(resource);
        if (rowIndex === -1)
            return;

        this._filteredEntries[rowIndex] = entry;
    }

    _hidePopover()
    {
        if (this._waterfallPopover)
            this._waterfallPopover.dismiss();
    }

    _hideResourceDetailView()
    {
        if (!this._resourceDetailView)
            return;

        this.element.classList.remove("showing-detail");
        this._table.scrollContainer.style.removeProperty("width");

        this.removeSubview(this._resourceDetailView);

        this._resourceDetailView.hidden();
        this._resourceDetailView = null;

        this._table.resize();
        this._table.reloadVisibleColumnCells(this._waterfallColumn);
    }

    _showResourceDetailView(resource)
    {
        let oldResourceDetailView = this._resourceDetailView;

        this._resourceDetailView = this._resourceDetailViewMap.get(resource);
        if (!this._resourceDetailView) {
            this._resourceDetailView = new WI.NetworkResourceDetailView(resource, this);
            this._resourceDetailViewMap.set(resource, this._resourceDetailView);
        }

        if (oldResourceDetailView) {
            oldResourceDetailView.hidden();
            this.replaceSubview(oldResourceDetailView, this._resourceDetailView);
        } else
            this.addSubview(this._resourceDetailView);
        this._resourceDetailView.shown();

        this.element.classList.add("showing-detail");
        this._table.scrollContainer.style.width = this._nameColumn.width + "px";

        // FIXME: It would be nice to avoid this.
        // Currently the ResourceDetailView is in the heirarchy but has not yet done a layout so we
        // end up seeing the table behind it. This forces us to layout now instead of after a beat.
        this.updateLayout();
    }

    _positionDetailView()
    {
        if (!this._resourceDetailView)
            return;

        let side = WI.resolvedLayoutDirection() === WI.LayoutDirection.RTL ? "right" : "left";
        this._resourceDetailView.element.style[side] = this._nameColumn.width + "px";
        this._table.scrollContainer.style.width = this._nameColumn.width + "px";
    }

    _updateURLFilterActiveIndicator()
    {
        this._urlFilterNavigationItem.filterBar.indicatingActive = this._hasURLFilter();
    }

    _updateEmptyFilterResultsMessage()
    {
        if (this._hasActiveFilter() && !this._filteredEntries.length)
            this._showEmptyFilterResultsMessage();
        else
            this._hideEmptyFilterResultsMessage();
    }

    _showEmptyFilterResultsMessage()
    {
        if (!this._emptyFilterResultsMessageElement) {
            let buttonElement = document.createElement("button");
            buttonElement.textContent = WI.UIString("Clear filters");
            buttonElement.addEventListener("click", () => { this._resetFilters(); });

            this._emptyFilterResultsMessageElement = WI.createMessageTextView(WI.UIString("No Filter Results"));
            this._emptyFilterResultsMessageElement.appendChild(buttonElement);
        }

        this.element.appendChild(this._emptyFilterResultsMessageElement);
        this._positionEmptyFilterMessage();
    }

    _hideEmptyFilterResultsMessage()
    {
        if (!this._emptyFilterResultsMessageElement)
            return;

        this._emptyFilterResultsMessageElement.remove();
    }

    _positionEmptyFilterMessage()
    {
        if (!this._emptyFilterResultsMessageElement)
            return;

        let width = this._nameColumn.width - 1; // For the 1px border.
        this._emptyFilterResultsMessageElement.style.width = width + "px";
    }

    _clearNetworkOnNavigateSettingChanged()
    {
        this._clearOnLoadNavigationItem.checked = !WI.settings.clearNetworkOnNavigate.value;
    }

    _resourceCachingDisabledSettingChanged()
    {
        this._disableResourceCacheNavigationItem.activated = WI.resourceCachingDisabledSetting.value;
    }

    _toggleDisableResourceCache()
    {
        WI.resourceCachingDisabledSetting.value = !WI.resourceCachingDisabledSetting.value;
    }

    _mainResourceDidChange(event)
    {
        let frame = event.target;
        if (!frame.isMainFrame() || !WI.settings.clearNetworkOnNavigate.value)
            return;

        this.reset();

        this._insertResourceAndReloadTable(frame.mainResource);
    }

    _mainFrameDidChange()
    {
        this._populateWithInitialResourcesIfNeeded();
    }

    _resourceLoadingDidFinish(event)
    {
        let resource = event.target;
        this._pendingUpdates.push(resource);

        if (resource.firstTimestamp < this._waterfallStartTime)
            this._waterfallStartTime = resource.firstTimestamp;
        if (resource.timingData.responseEnd > this._waterfallEndTime)
            this._waterfallEndTime = resource.timingData.responseEnd;

        if (this._hasURLFilter())
            this._checkURLFilterAgainstResource(resource);

        this.needsLayout();
    }

    _resourceLoadingDidFail(event)
    {
        let resource = event.target;
        this._pendingUpdates.push(resource);

        if (resource.firstTimestamp < this._waterfallStartTime)
            this._waterfallStartTime = resource.firstTimestamp;
        if (resource.timingData.responseEnd > this._waterfallEndTime)
            this._waterfallEndTime = resource.timingData.responseEnd;

        if (this._hasURLFilter())
            this._checkURLFilterAgainstResource(resource);

        this.needsLayout();
    }

    _resourceTransferSizeDidChange(event)
    {
        if (!this._table)
            return;

        let resource = event.target;

        // In the unlikely event that this is the sort column, we may need to resort.
        if (this._table.sortColumnIdentifier === "transferSize") {
            this._pendingUpdates.push(resource);
            this.needsLayout();
            return;
        }

        let index = this._entries.findIndex((x) => x.resource === resource);
        if (index === -1)
            return;

        let entry = this._entries[index];
        entry.transferSize = !isNaN(resource.networkTotalTransferSize) ? resource.networkTotalTransferSize : resource.estimatedTotalTransferSize;

        let rowIndex = this._rowIndexForResource(resource);
        if (rowIndex === -1)
            return;

        this._table.reloadCell(rowIndex, "transferSize");
    }

    _networkTimelineRecordAdded(event)
    {
        let resourceTimelineRecord = event.data.record;
        console.assert(resourceTimelineRecord instanceof WI.ResourceTimelineRecord);

        let resource = resourceTimelineRecord.resource;
        if (isNaN(this._waterfallStartTime))
            this._waterfallStartTime = this._waterfallEndTime = resource.firstTimestamp;

        this._insertResourceAndReloadTable(resource);
    }

    _isDefaultSort()
    {
        return this._table.sortColumnIdentifier === "waterfall" && this._table.sortOrder === WI.Table.SortOrder.Ascending;
    }

    _insertResourceAndReloadTable(resource)
    {
        if (!this._table || !(WI.tabBrowser.selectedTabContentView instanceof WI.NetworkTabContentView)) {
            this._pendingInsertions.push(resource);
            this.needsLayout();
            return;
        }

        let entry = this._entryForResource(resource);

        // Default sort has fast path.
        if (this._isDefaultSort() || !this._entriesSortComparator) {
            this._entries.push(entry);
            if (this._passFilter(entry)) {
                this._filteredEntries.push(entry);
                this._table.reloadDataAddedToEndOnly();
            }
            return;
        }

        insertObjectIntoSortedArray(entry, this._entries, this._entriesSortComparator);

        if (this._passFilter(entry)) {
            insertObjectIntoSortedArray(entry, this._filteredEntries, this._entriesSortComparator);

            // Probably a useless optimization here, but if we only added this row to the end
            // we may avoid recreating all visible rows by saying as such.
            if (this._filteredEntries.lastValue === entry)
                this._table.reloadDataAddedToEndOnly();
            else
                this._table.reloadData();
        }
    }

    _entryForResource(resource)
    {
        // FIXME: <https://webkit.org/b/143632> Web Inspector: Resources with the same name in different folders aren't distinguished
        // FIXME: <https://webkit.org/b/176765> Web Inspector: Resource names should be less ambiguous

        return {
            resource,
            name: WI.displayNameForURL(resource.url, resource.urlComponents),
            domain: WI.displayNameForHost(resource.urlComponents.host),
            scheme: resource.urlComponents.scheme ? resource.urlComponents.scheme.toLowerCase() : "",
            method: resource.requestMethod,
            type: resource.type,
            displayType: WI.NetworkTableContentView.displayNameForResource(resource),
            mimeType: resource.mimeType,
            status: resource.statusCode,
            cached: resource.cached,
            resourceSize: resource.size,
            transferSize: !isNaN(resource.networkTotalTransferSize) ? resource.networkTotalTransferSize : resource.estimatedTotalTransferSize,
            time: resource.totalDuration,
            protocol: resource.protocol,
            priority: resource.priority,
            remoteAddress: resource.remoteAddress,
            connectionIdentifier: resource.connectionIdentifier,
            startTime: resource.firstTimestamp,
        };
    }

    _hasTypeFilter()
    {
        return !!this._activeTypeFilters;
    }

    _hasURLFilter()
    {
        return this._urlFilterIsActive;
    }

    _hasActiveFilter()
    {
        return this._hasTypeFilter()
            || this._hasURLFilter();
    }

    _passTypeFilter(entry)
    {
        if (!this._hasTypeFilter())
            return true;
        return this._activeTypeFilters.some((checker) => checker(entry.resource.type));
    }

    _passURLFilter(entry)
    {
        if (!this._hasURLFilter())
            return true;
        return this._activeURLFilterResources.has(entry.resource);
    }

    _passFilter(entry)
    {
        return this._passTypeFilter(entry)
            && this._passURLFilter(entry);
    }

    _updateSortAndFilteredEntries()
    {
        this._entries = this._entries.sort(this._entriesSortComparator);
        this._updateFilteredEntries();
    }

    _updateFilteredEntries()
    {
        if (this._hasActiveFilter())
            this._filteredEntries = this._entries.filter(this._passFilter, this);
        else
            this._filteredEntries = this._entries.slice();

        this._restoreSelectedRow();

        this._updateURLFilterActiveIndicator();
        this._updateEmptyFilterResultsMessage();
    }

    _generateTypeFilter()
    {
        let selectedItems = this._typeFilterScopeBar.selectedItems;
        if (!selectedItems.length || selectedItems.includes(this._typeFilterScopeBarItemAll))
            return null;

        return selectedItems.map((item) => item.__checker);
    }

    _resetFilters()
    {
        console.assert(this._hasActiveFilter());

        // Clear url filter.
        this._urlFilterSearchText = null;
        this._urlFilterSearchRegex = null;
        this._urlFilterIsActive = false;
        this._activeURLFilterResources.clear();
        this._urlFilterNavigationItem.filterBar.clear();
        console.assert(!this._hasURLFilter());

        // Clear type filter.
        this._typeFilterScopeBar.resetToDefault();
        console.assert(!this._hasTypeFilter());

        console.assert(!this._hasActiveFilter());

        this._updateFilteredEntries();
        this._table.reloadData();
    }

    _areFilterListsIdentical(listA, listB)
    {
        if (listA && listB) {
            if (listA.length !== listB.length)
                return false;

            for (let i = 0; i < listA.length; ++i) {
                if (listA[i] !== listB[i])
                    return false;
            }

            return true;
        }

        return false;
    }

    _typeFilterScopeBarSelectionChanged(event)
    {
        // FIXME: <https://webkit.org/b/176763> Web Inspector: ScopeBar SelectionChanged event may dispatch multiple times for a single logical change
        // We can't use shallow equals here because the contents are functions.
        let oldFilter = this._activeTypeFilters;
        let newFilter = this._generateTypeFilter();
        if (this._areFilterListsIdentical(oldFilter, newFilter))
            return;

        // Even if the selected resource would still be visible, lets close the detail view if a filter changes.
        this._hideResourceDetailView();

        this._activeTypeFilters = newFilter;
        this._updateFilteredEntries();
        this._table.reloadData();
    }

    _urlFilterDidChange(event)
    {
        let searchQuery = this._urlFilterNavigationItem.filterBar.filters.text;
        if (searchQuery === this._urlFilterSearchText)
            return;

        // Even if the selected resource would still be visible, lets close the detail view if a filter changes.
        this._hideResourceDetailView();

        // Search cleared.
        if (!searchQuery) {
            this._urlFilterSearchText = null;
            this._urlFilterSearchRegex = null;
            this._urlFilterIsActive = false;
            this._activeURLFilterResources.clear();

            this._updateFilteredEntries();
            this._table.reloadData();
            return;
        }

        this._urlFilterIsActive = true;
        this._urlFilterSearchText = searchQuery;
        this._urlFilterSearchRegex = new RegExp(searchQuery.escapeForRegExp(), "i");

        this._activeURLFilterResources.clear();

        for (let entry of this._entries)
            this._checkURLFilterAgainstResource(entry.resource);

        this._updateFilteredEntries();
        this._table.reloadData();
    }

    _restoreSelectedRow()
    {
        if (!this._selectedResource)
            return;

        let rowIndex = this._rowIndexForResource(this._selectedResource);
        if (rowIndex === -1) {
            this._selectedResource = null;
            this._table.clearSelectedRow();
            return;
        }

        this._table.selectRow(rowIndex);
    }

    _HARResources()
    {
        let resources = this._filteredEntries.map((x) => x.resource);
        const supportedHARSchemes = new Set(["http", "https", "ws", "wss"]);
        return resources.filter((resource) => resource.finished && supportedHARSchemes.has(resource.urlComponents.scheme));
    }

    _exportHAR()
    {
        let resources = this._HARResources();
        if (!resources.length) {
            InspectorFrontendHost.beep();
            return;
        }

        WI.HARBuilder.buildArchive(resources).then((har) => {
            let mainFrame = WI.frameResourceManager.mainFrame;
            let archiveName = mainFrame.mainResource.urlComponents.host || mainFrame.mainResource.displayName || "Archive";
            let url = "web-inspector:///" + encodeURI(archiveName) + ".har";
            WI.saveDataToFile({
                url,
                content: JSON.stringify(har, null, 2),
                forceSaveAs: true,
            });
        }).catch(handlePromiseException);
    }

    _waterfallPopoverContentForResource(resource)
    {
        let contentElement = document.createElement("div");
        contentElement.className = "waterfall-popover";

        if (!resource.hasResponse() || !resource.timingData.startTime || !resource.timingData.responseEnd) {
            contentElement.textContent = WI.UIString("Resource has no timing data");
            return contentElement;
        }

        let breakdownView = new WI.ResourceTimingBreakdownView(resource, 300);
        contentElement.appendChild(breakdownView.element);
        breakdownView.updateLayout();

        return contentElement;
    }

    _handleMousedownWaterfall(mouseBlock, entry, event)
    {
        if (!this._waterfallPopover) {
            this._waterfallPopover = new WI.Popover;
            this._waterfallPopover.backgroundStyle = WI.Popover.BackgroundStyle.White;
        }

        if (this._waterfallPopover.visible)
            return;

        let calculateTargetFrame = () => {
            let rowIndex = this._rowIndexForResource(entry.resource);
            let cell = this._table.cellForRowAndColumn(rowIndex, this._waterfallColumn);
            if (!cell) {
                this._waterfallPopover.dismiss();
                return null;
            }

            let mouseBlock = cell.querySelector(".block.mouse-tracking");
            if (!mouseBlock) {
                this._waterfallPopover.dismiss();
                return null;
            }

            return WI.Rect.rectFromClientRect(mouseBlock.getBoundingClientRect());
        };

        let targetFrame = calculateTargetFrame();
        if (!targetFrame)
            return;
        if (!targetFrame.size.width && !targetFrame.size.height)
            return;

        let isRTL = WI.resolvedLayoutDirection() === WI.LayoutDirection.RTL;
        let preferredEdges = isRTL ? [WI.RectEdge.MAX_Y, WI.RectEdge.MIN_Y, WI.RectEdge.MAX_X] : [WI.RectEdge.MAX_Y, WI.RectEdge.MIN_Y, WI.RectEdge.MIN_X];
        this._waterfallPopover.windowResizeHandler = () => {
            let bounds = calculateTargetFrame();
            if (bounds)
                this._waterfallPopover.present(bounds, preferredEdges);
        };

        let popoverContentElement = this._waterfallPopoverContentForResource(entry.resource);
        this._waterfallPopover.presentNewContentWithFrame(popoverContentElement, targetFrame, preferredEdges);
    }

    _tableNameColumnDidChangeWidth(event)
    {
        this._nameColumnWidthSetting.value = event.target.width;

        this._positionDetailView();
        this._positionEmptyFilterMessage();
    }

    _tableWaterfallColumnDidChangeWidth(event)
    {
        this._table.reloadVisibleColumnCells(this._waterfallColumn);
    }
};
