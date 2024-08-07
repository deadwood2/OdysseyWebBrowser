/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

WI.VisualStyleSelectorSection = class VisualStyleSelectorSection extends WI.DetailsSection
{
    constructor()
    {
        let selectorSection = {element: document.createElement("div")};
        selectorSection.element.classList.add("selectors");

        let controlElement = document.createElement("div");
        controlElement.classList.add("controls");

        super("visual-style-selector-section", WI.UIString("Style Rules"), [selectorSection], controlElement);

        this._nodeStyles = null;

        this._currentSelectorElement = document.createElement("div");
        this._currentSelectorElement.classList.add("current-selector");

        let currentSelectorIconElement = document.createElement("img");
        currentSelectorIconElement.classList.add("icon");
        this._currentSelectorElement.appendChild(currentSelectorIconElement);

        this._currentSelectorText = document.createElement("span");
        this._currentSelectorElement.appendChild(this._currentSelectorText);

        this._headerElement.appendChild(this._currentSelectorElement);

        let selectorListElement = document.createElement("ol");
        selectorListElement.classList.add("selector-list");
        selectorSection.element.appendChild(selectorListElement);

        this._selectors = new WI.TreeOutline(selectorListElement);
        this._selectors.disclosureButtons = false;
        this._selectors.addEventListener(WI.TreeOutline.Event.SelectionDidChange, this._selectorChanged, this);

        this._newInspectorRuleSelector = null;

        let addGlyphElement = WI.ImageUtilities.useSVGSymbol("Images/Plus13.svg", "visual-style-selector-section-add-rule", WI.UIString("Add new rule"));
        addGlyphElement.addEventListener("click", this._addNewRuleClick.bind(this));
        addGlyphElement.addEventListener("contextmenu", this._addNewRuleContextMenu.bind(this));
        controlElement.appendChild(addGlyphElement);

        this._headerElement.addEventListener("mouseover", this._handleMouseOver.bind(this));
        this._headerElement.addEventListener("mouseout", this._handleMouseOut.bind(this));
    }

    // Public

    update(nodeStyles)
    {
        let style = this.currentStyle();
        if (style)
            this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol] = style;

        if (nodeStyles)
            this._nodeStyles = nodeStyles;

        if (!this._nodeStyles)
            return;

        this._selectors.removeChildren();
        let previousRule = null;

        // Pseudo Styles
        let pseudoRules = [];
        let pseudoElements = this._nodeStyles.pseudoElements;
        for (let pseudoIdentifier in pseudoElements)
            pseudoRules = pseudoRules.concat(pseudoElements[pseudoIdentifier].matchedRules);

        let orderedPseudoRules = uniqueOrderedRules(pseudoRules);
        // Reverse the array to ensure that splicing the array will not mess with the order.
        if (orderedPseudoRules.length)
            orderedPseudoRules.reverse();

        function createSelectorItem(style, title, subtitle) {
            let selector = new WI.VisualStyleSelectorTreeItem(this, style, title, subtitle);
            selector.addEventListener(WI.VisualStyleSelectorTreeItem.Event.CheckboxChanged, this._treeElementCheckboxToggled, this);
            this._selectors.appendChild(selector);

            if (style.isInspectorRule() && this._newInspectorRuleSelector === style.selectorText && !style.hasProperties()) {
                selector.select(true);
                selector.element.scrollIntoView();
                this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol] = style;
                this._newInspectorRuleSelector = null;
                return;
            }

            if (this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol] === style) {
                selector.select(true);
                selector.element.scrollIntoView();
            }
        }

        function uniqueOrderedRules(orderedRules)
        {
            if (!orderedRules || !orderedRules.length)
                return new Array;

            let uniqueRules = new Map;
            for (let rule of orderedRules) {
                if (!uniqueRules.has(rule.id))
                    uniqueRules.set(rule.id, rule);
            }
            return Array.from(uniqueRules.values());
        }

        function insertAllMatchingPseudoRules(force)
        {
            if (!orderedPseudoRules.length)
                return;

            if (force) {
                for (let i = orderedPseudoRules.length - 1; i >= 0; --i) {
                    let pseudoRule = orderedPseudoRules[i];
                    createSelectorItem.call(this, pseudoRule.style, pseudoRule.selectorText, pseudoRule.mediaText);
                }
                orderedPseudoRules = [];
            }

            if (!previousRule)
                return;

            for (let i = orderedPseudoRules.length - 1; i >= 0; --i) {
                let pseudoRule = orderedPseudoRules[i];
                if (!pseudoRule.selectorIsGreater(previousRule.mostSpecificSelector))
                    continue;

                createSelectorItem.call(this, pseudoRule.style, pseudoRule.selectorText, pseudoRule.mediaText);
                previousRule = pseudoRule;
                orderedPseudoRules.splice(i, 1);
            }
        }

        if (this._nodeStyles.inlineStyle) {
            if (!this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol])
                this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol] = this._nodeStyles.inlineStyle;

            // Inline Style
            createSelectorItem.call(this, this._nodeStyles.inlineStyle, WI.UIString("This Element"));
        } else if (!this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol])
            this._nodeStyles[WI.VisualStyleSelectorSection.LastSelectedRuleSymbol] = this._nodeStyles.matchedRules[0].style;

        // Matched Rules
        for (let rule of uniqueOrderedRules(this._nodeStyles.matchedRules)) {
            if (rule.type === WI.CSSStyleSheet.Type.UserAgent) {
                insertAllMatchingPseudoRules.call(this, true);
                continue;
            }

            insertAllMatchingPseudoRules.call(this);
            createSelectorItem.call(this, rule.style, rule.selectorText, rule.mediaText);
            previousRule = rule;
        }

        // Just in case there are any remaining pseudo-styles.
        insertAllMatchingPseudoRules.call(this, true);

        // Inherited Rules
        for (let inherited of this._nodeStyles.inheritedRules) {
            if (!inherited.matchedRules || !inherited.matchedRules.length)
                continue;

            let divider = null;

            for (let rule of uniqueOrderedRules(inherited.matchedRules)) {
                if (rule.type === WI.CSSStyleSheet.Type.UserAgent)
                    continue;

                if (!divider) {
                    let dividerText = WI.UIString("Inherited from %s").format(inherited.node.displayName);
                    divider = new WI.GeneralTreeElement("section-divider", dividerText);
                    divider.selectable = false;
                    this._selectors.appendChild(divider);
                }

                createSelectorItem.call(this, rule.style, rule.selectorText, rule.mediaText);
            }
        }

        this._newInspectorRuleSelector = null;
    }

    currentStyle()
    {
        if (!this._nodeStyles || !this._selectors.selectedTreeElement)
            return null;

        return this._selectors.selectedTreeElement.representedObject;
    }

    treeItemForStyle(style)
    {
        for (let item of this._selectors.children) {
            if (item.representedObject === style)
                return item;
        }
        return null;
    }

    selectEmptyStyleTreeItem(style)
    {
        if (style.hasProperties())
            return false;

        let treeItem = this.treeItemForStyle(style);
        if (!treeItem)
            return false;

        treeItem.select(true, true);
        return true;
    }

    // Private

    _selectorChanged(event)
    {
        let selectedTreeElement = event.data.selectedElement;
        if (!selectedTreeElement)
            return;

        // The class needs to be completely reset as the previously selected treeElement most likely had
        // a different icon className and it is simpler to regenerate the class than to find out which
        // class was previously applied.
        this._currentSelectorElement.className = "current-selector " + selectedTreeElement.iconClassName;

        let selectorText = selectedTreeElement.mainTitle;
        let mediaText = selectedTreeElement.subtitle;
        if (mediaText && mediaText.length)
            selectorText += " \u2014 " + mediaText; // em-dash

        this._currentSelectorText.textContent = selectorText;

        this.dispatchEventToListeners(WI.VisualStyleSelectorSection.Event.SelectorChanged);
    }

    _addNewRuleClick(event)
    {
        if (!this._nodeStyles || this._nodeStyles.node.isInUserAgentShadowTree())
            return;

        let selector = this.currentStyle().selectorText;
        let existingRules = this._nodeStyles.rulesForSelector(selector);
        for (let rule of existingRules) {
            if (this.selectEmptyStyleTreeItem(rule.style))
                return;
        }

        this._newInspectorRuleSelector = selector;
        this._nodeStyles.addRule(selector);
    }

    _addNewRuleContextMenu(event)
    {
        if (!this._nodeStyles || this._nodeStyles.node.isInUserAgentShadowTree())
            return;

        let styleSheets = WI.cssStyleManager.styleSheets.filter(styleSheet => styleSheet.hasInfo() && !styleSheet.isInlineStyleTag() && !styleSheet.isInlineStyleAttributeStyleSheet());
        if (!styleSheets.length)
            return;

        let contextMenu = WI.ContextMenu.createFromEvent(event);

        const handler = null;
        const disabled = true;
        contextMenu.appendItem(WI.UIString("Available Style Sheets"), handler, disabled);

        for (let styleSheet of styleSheets) {
            contextMenu.appendItem(styleSheet.displayName, () => {
                const text = "";
                this._nodeStyles.addRule(this.currentStyle().selectorText, text, styleSheet.id);
            });
        }
    }

    _treeElementCheckboxToggled(event)
    {
        let style = this.currentStyle();
        if (!style)
            return;

        let styleText = style.text;
        if (!styleText || !styleText.length)
            return;

        // Comment or uncomment the style text.
        let newStyleText = "";
        let styleEnabled = event && event.data && event.data.enabled;
        if (styleEnabled)
            newStyleText = styleText.replace(/\s*(\/\*|\*\/)\s*/g, "");
        else
            newStyleText = "/* " + styleText.replace(/(\s*;(?!$)\s*)/g, "$1 *//* ") + " */";

        style.text = newStyleText;
        style[WI.VisualStyleDetailsPanel.StyleDisabledSymbol] = !styleEnabled;
        this.dispatchEventToListeners(WI.VisualStyleSelectorSection.Event.SelectorChanged);

    }

    _handleMouseOver()
    {
        if (!this.collapsed)
            return;

        let style = this.currentStyle();
        if (!style)
            return;

        if (!style.ownerRule) {
            WI.domTreeManager.highlightDOMNode(style.node.id);
            return;
        }

        WI.domTreeManager.highlightSelector(style.ownerRule.selectorText, style.node.ownerDocument.frameIdentifier);
    }

    _handleMouseOut()
    {
        if (!this.collapsed)
            return;

        WI.domTreeManager.hideDOMNodeHighlight();
    }
};

WI.VisualStyleSelectorSection.LastSelectedRuleSymbol = Symbol("visual-style-selector-section-last-selected-rule");

WI.VisualStyleSelectorSection.Event = {
    SelectorChanged: "visual-style-selector-section-selector-changed"
};
