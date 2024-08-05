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

WI.ComputedStyleDetailsPanel = class ComputedStyleDetailsPanel extends WI.StyleDetailsPanel
{
    constructor(delegate)
    {
        super(delegate, WI.ComputedStyleDetailsPanel.StyleClassName, "computed", WI.UIString("Styles \u2014 Computed"));

        this._computedStyleShowAllSetting = new WI.Setting("computed-style-show-all", false);

        this.cssStyleDeclarationTextEditorShouldAddPropertyGoToArrows = true;
    }

    // Public

    get regionFlow() { return this._regionFlow; }
    set regionFlow(regionFlow)
    {
        this._regionFlow = regionFlow;
        this._regionFlowNameLabelValue.textContent = regionFlow ? regionFlow.name : "";
        this._regionFlowNameRow.value = regionFlow ? this._regionFlowFragment : null;
        this._updateFlowNamesSectionVisibility();
    }

    get contentFlow() { return this._contentFlow; }
    set contentFlow(contentFlow)
    {
        this._contentFlow = contentFlow;
        this._contentFlowNameLabelValue.textContent = contentFlow ? contentFlow.name : "";
        this._contentFlowNameRow.value = contentFlow ? this._contentFlowFragment : null;
        this._updateFlowNamesSectionVisibility();
    }

    get containerRegions() { return this._containerRegions; }
    set containerRegions(regions)
    {
        this._containerRegions = regions;

        if (!regions || !regions.length) {
            this._containerRegionsFlowSection.element.classList.add("hidden");
            return;
        }

        this._containerRegionsDataGrid.removeChildren();
        for (var regionNode of regions)
            this._containerRegionsDataGrid.appendChild(new WI.DOMTreeDataGridNode(regionNode));

        this._containerRegionsFlowSection.element.classList.remove("hidden");

        this._containerRegionsDataGrid.updateLayoutIfNeeded();
    }

    cssStyleDeclarationTextEditorShowProperty(property, showSource)
    {
        function delegateShowProperty() {
            if (typeof this._delegate.computedStyleDetailsPanelShowProperty === "function")
                this._delegate.computedStyleDetailsPanelShowProperty(property);
        }

        if (!showSource) {
            delegateShowProperty.call(this);
            return;
        }

        let effectiveProperty = this._nodeStyles.effectivePropertyForName(property.name);
        if (!effectiveProperty || !effectiveProperty.styleSheetTextRange) {
            if (!effectiveProperty.relatedShorthandProperty) {
                delegateShowProperty.call(this);
                return;
            }
            effectiveProperty = effectiveProperty.relatedShorthandProperty;
        }

        let ownerRule = effectiveProperty.ownerStyle.ownerRule;
        if (!ownerRule) {
            delegateShowProperty.call(this);
            return;
        }

        let sourceCode = ownerRule.sourceCodeLocation.sourceCode;
        let {startLine, startColumn} = effectiveProperty.styleSheetTextRange;

        const options = {
            ignoreNetworkTab: true,
            ignoreSearchTab: true,
        };
        WI.showSourceCodeLocation(sourceCode.createSourceCodeLocation(startLine, startColumn), options);
    }

    refresh(significantChange)
    {
        // We only need to do a rebuild on significant changes. Other changes are handled
        // by the sections and text editors themselves.
        if (!significantChange) {
            super.refresh();
            return;
        }

        this._propertiesTextEditor.style = this.nodeStyles.computedStyle;
        this._variablesTextEditor.style = this.nodeStyles.computedStyle;
        this._refreshFlowDetails(this.nodeStyles.node);
        this._boxModelDiagramRow.nodeStyles = this.nodeStyles;

        super.refresh();

        this._variablesSection.element.classList.toggle("hidden", !this._variablesTextEditor.shownProperties.length);
    }

    filterDidChange(filterBar)
    {
        let filterText = filterBar.filters.text;
        this._propertiesTextEditor.removeNonMatchingProperties(filterText);
        this._variablesTextEditor.removeNonMatchingProperties(filterText);
    }

    // Protected

    initialLayout()
    {
        let computedStyleShowAllLabel = document.createElement("label");
        computedStyleShowAllLabel.textContent = WI.UIString("Show All");

        this._computedStyleShowAllCheckbox = document.createElement("input");
        this._computedStyleShowAllCheckbox.type = "checkbox";
        this._computedStyleShowAllCheckbox.checked = this._computedStyleShowAllSetting.value;
        this._computedStyleShowAllCheckbox.addEventListener("change", this._computedStyleShowAllCheckboxValueChanged.bind(this));
        computedStyleShowAllLabel.appendChild(this._computedStyleShowAllCheckbox);

        this._propertiesTextEditor = new WI.CSSStyleDeclarationTextEditor(this);
        this._propertiesTextEditor.propertyVisibilityMode = WI.CSSStyleDeclarationTextEditor.PropertyVisibilityMode.HideVariables;
        this._propertiesTextEditor.showsImplicitProperties = this._computedStyleShowAllSetting.value;
        this._propertiesTextEditor.alwaysShowPropertyNames = ["display", "width", "height"];
        this._propertiesTextEditor.sortProperties = true;

        let propertiesRow = new WI.DetailsSectionRow;
        let propertiesGroup = new WI.DetailsSectionGroup([propertiesRow]);
        let propertiesSection = new WI.DetailsSection("computed-style-properties", WI.UIString("Properties"), [propertiesGroup], computedStyleShowAllLabel);
        propertiesSection.addEventListener(WI.DetailsSection.Event.CollapsedStateChanged, this._handlePropertiesSectionCollapsedStateChanged, this);

        this.addSubview(this._propertiesTextEditor);

        propertiesRow.element.appendChild(this._propertiesTextEditor.element);

        this._variablesTextEditor = new WI.CSSStyleDeclarationTextEditor(this);
        this._variablesTextEditor.propertyVisibilityMode = WI.CSSStyleDeclarationTextEditor.PropertyVisibilityMode.HideNonVariables;
        this._variablesTextEditor.sortProperties = true;

        let variablesRow = new WI.DetailsSectionRow;
        let variablesGroup = new WI.DetailsSectionGroup([variablesRow]);
        this._variablesSection = new WI.DetailsSection("computed-style-properties", WI.UIString("Variables"), [variablesGroup]);
        this._variablesSection.addEventListener(WI.DetailsSection.Event.CollapsedStateChanged, this._handleVariablesSectionCollapsedStateChanged, this);

        this.addSubview(this._variablesTextEditor);

        variablesRow.element.appendChild(this._variablesTextEditor.element);

        // Region flow name is used to display the "flow-from" property of the Region Containers.
        this._regionFlowFragment = document.createElement("span");
        this._regionFlowFragment.appendChild(document.createElement("img")).className = "icon";
        this._regionFlowNameLabelValue = this._regionFlowFragment.appendChild(document.createElement("span"));

        let goToRegionFlowButton = this._regionFlowFragment.appendChild(WI.createGoToArrowButton());
        goToRegionFlowButton.addEventListener("click", this._goToRegionFlowArrowWasClicked.bind(this));

        this._regionFlowNameRow = new WI.DetailsSectionSimpleRow(WI.UIString("Region Flow"));
        this._regionFlowNameRow.element.classList.add("content-flow-link");

        // Content flow name is used to display the "flow-into" property of the Content nodes.
        this._contentFlowFragment = document.createElement("span");
        this._contentFlowFragment.appendChild(document.createElement("img")).className = "icon";
        this._contentFlowNameLabelValue = this._contentFlowFragment.appendChild(document.createElement("span"));

        let goToContentFlowButton = this._contentFlowFragment.appendChild(WI.createGoToArrowButton());
        goToContentFlowButton.addEventListener("click", this._goToContentFlowArrowWasClicked.bind(this));

        this._contentFlowNameRow = new WI.DetailsSectionSimpleRow(WI.UIString("Content Flow"));
        this._contentFlowNameRow.element.classList.add("content-flow-link");

        let flowNamesGroup = new WI.DetailsSectionGroup([this._regionFlowNameRow, this._contentFlowNameRow]);
        this._flowNamesSection = new WI.DetailsSection("content-flow", WI.UIString("Flows"), [flowNamesGroup]);

        this._containerRegionsDataGrid = new WI.DOMTreeDataGrid;
        this._containerRegionsDataGrid.headerVisible = false;

        this._containerRegionsRow = new WI.DetailsSectionDataGridRow(this._containerRegionsDataGrid);
        let containerRegionsGroup = new WI.DetailsSectionGroup([this._containerRegionsRow]);
        this._containerRegionsFlowSection = new WI.DetailsSection("container-regions", WI.UIString("Container Regions"), [containerRegionsGroup]);

        this.element.appendChild(propertiesSection.element);
        this.element.appendChild(this._variablesSection.element);
        this.element.appendChild(this._flowNamesSection.element);
        this.element.appendChild(this._containerRegionsFlowSection.element);

        this._resetFlowDetails();

        this._boxModelDiagramRow = new WI.BoxModelDetailsSectionRow;

        let boxModelGroup = new WI.DetailsSectionGroup([this._boxModelDiagramRow]);
        let boxModelSection = new WI.DetailsSection("style-box-model", WI.UIString("Box Model"), [boxModelGroup]);

        this.element.appendChild(boxModelSection.element);
    }

    sizeDidChange()
    {
        super.sizeDidChange();

        // FIXME: <https://webkit.org/b/152269> Web Inspector: Convert DetailsSection classes to use View
        this._containerRegionsRow.sizeDidChange();
    }

    // Private

    _computedStyleShowAllCheckboxValueChanged(event)
    {
        let checked = this._computedStyleShowAllCheckbox.checked;
        this._computedStyleShowAllSetting.value = checked;
        this._propertiesTextEditor.showsImplicitProperties = checked;
        this._propertiesTextEditor.updateLayout();
    }

    _handlePropertiesSectionCollapsedStateChanged(event)
    {
        if (event && event.data && !event.data.collapsed)
            this._propertiesTextEditor.refresh();
    }

    _handleVariablesSectionCollapsedStateChanged(event)
    {
        if (event && event.data && !event.data.collapsed)
            this._variablesTextEditor.refresh();
    }

    _updateFlowNamesSectionVisibility()
    {
        this._flowNamesSection.element.classList.toggle("hidden", !this._contentFlow && !this._regionFlow);
    }

    _resetFlowDetails ()
    {
        this.regionFlow = null;
        this.contentFlow = null;
        this.containerRegions = null;
    }

    _refreshFlowDetails(domNode)
    {
        this._resetFlowDetails();
        if (!domNode)
            return;

        function contentFlowInfoReady(error, flowData)
        {
            // Element is not part of any flow.
            if (error || !flowData) {
                this._resetFlowDetails();
                return;
            }

            this.regionFlow = flowData.regionFlow;
            this.contentFlow = flowData.contentFlow;
            this.containerRegions = flowData.regions;
        }

        WI.domTreeManager.getNodeContentFlowInfo(domNode, contentFlowInfoReady.bind(this));
    }

    _goToRegionFlowArrowWasClicked()
    {
        WI.showRepresentedObject(this._regionFlow);
    }

    _goToContentFlowArrowWasClicked()
    {
        WI.showRepresentedObject(this._contentFlow, {nodeToSelect: this.nodeStyles.node});
    }
};

WI.ComputedStyleDetailsPanel.StyleClassName = "computed";
