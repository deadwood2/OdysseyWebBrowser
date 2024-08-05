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

WI.VisualStyleSelectorTreeItem = class VisualStyleSelectorTreeItem extends WI.GeneralTreeElement
{
    constructor(delegate, style, title, subtitle)
    {
        let iconClassName;
        switch (style.type) {
        case WI.CSSStyleDeclaration.Type.Rule:
            console.assert(style.ownerRule instanceof WI.CSSRule, style.ownerRule);

            if (style.inherited)
                iconClassName = WI.CSSStyleDeclarationSection.InheritedStyleRuleIconStyleClassName;
            else if (style.ownerRule.type === WI.CSSStyleSheet.Type.Author)
                iconClassName = WI.CSSStyleDeclarationSection.AuthorStyleRuleIconStyleClassName;
            else if (style.ownerRule.type === WI.CSSStyleSheet.Type.User)
                iconClassName = WI.CSSStyleDeclarationSection.UserStyleRuleIconStyleClassName;
            else if (style.ownerRule.type === WI.CSSStyleSheet.Type.UserAgent)
                iconClassName = WI.CSSStyleDeclarationSection.UserAgentStyleRuleIconStyleClassName;
            else if (style.ownerRule.type === WI.CSSStyleSheet.Type.Inspector)
                iconClassName = WI.CSSStyleDeclarationSection.InspectorStyleRuleIconStyleClassName;
            break;

        case WI.CSSStyleDeclaration.Type.Inline:
        case WI.CSSStyleDeclaration.Type.Attribute:
            if (style.inherited)
                iconClassName = WI.CSSStyleDeclarationSection.InheritedElementStyleRuleIconStyleClassName;
            else
                iconClassName = WI.DOMTreeElementPathComponent.DOMElementIconStyleClassName;
            break;
        }

        let iconClasses = [iconClassName];
        if (style.ownerRule && style.ownerRule.hasMatchedPseudoElementSelector())
            iconClasses.push(WI.CSSStyleDeclarationSection.PseudoElementSelectorStyleClassName);

        title = title.trim();

        super(["visual-style-selector-item", ...iconClasses], title, subtitle, style);

        this._delegate = delegate;

        this._iconClasses = iconClasses;
        this._lastValue = title;
        this._enableEditing = true;
        this._hasInvalidSelector = false;
    }

    // Public

    get iconClassName()
    {
        return this._iconClasses.join(" ");
    }

    get selectorText()
    {
        let titleText = this._mainTitleElement.textContent;
        if (!titleText || !titleText.length)
            titleText = this._mainTitle;

        return titleText.trim();
    }

    // Protected

    onattach()
    {
        super.onattach();

        this._listItemNode.addEventListener("mouseover", this._highlightNodesWithSelector.bind(this));
        this._listItemNode.addEventListener("mouseout", this._hideDOMNodeHighlight.bind(this));

        this._checkboxElement = document.createElement("input");
        this._checkboxElement.type = "checkbox";
        this._checkboxElement.checked = !this.representedObject[WI.VisualStyleDetailsPanel.StyleDisabledSymbol];
        this._updateCheckboxTitle();
        this._checkboxElement.addEventListener("change", this._handleCheckboxChanged.bind(this));
        this._listItemNode.insertBefore(this._checkboxElement, this._iconElement);

        this._iconElement.addEventListener("click", this._handleIconElementClicked.bind(this));

        this._mainTitleElement.spellcheck = false;
        this._mainTitleElement.addEventListener("mousedown", this._handleMainTitleMouseDown.bind(this));
        this._mainTitleElement.addEventListener("keydown", this._handleMainTitleKeyDown.bind(this));
        this._mainTitleElement.addEventListener("keyup", this._highlightNodesWithSelector.bind(this));
        this._mainTitleElement.addEventListener("blur", this._commitSelector.bind(this));

        this.representedObject.addEventListener(WI.CSSStyleDeclaration.Event.InitialTextModified, this._styleTextModified, this);
        if (this.representedObject.ownerRule)
            this.representedObject.ownerRule.addEventListener(WI.CSSRule.Event.SelectorChanged, this._updateSelectorIcon, this);

        this._styleTextModified();
    }

    ondeselect()
    {
        this._listItemNode.classList.remove("editable");
        this._mainTitleElement.contentEditable = false;
    }

    populateContextMenu(contextMenu, event)
    {
        contextMenu.appendItem(WI.UIString("Copy Rule"), () => {
            InspectorFrontendHost.copyText(this.representedObject.generateCSSRuleString());
        });

        if (this.representedObject.modified) {
            contextMenu.appendItem(WI.UIString("Reset"), () => {
                this.representedObject.resetText();
            });
        }

        if (!this.representedObject.ownerRule)
            return;

        contextMenu.appendItem(WI.UIString("Show Source"), () => {
            const options = {
                ignoreNetworkTab: true,
                ignoreSearchTab: true,
            };

            if (event.metaKey)
                WI.showOriginalUnformattedSourceCodeLocation(this.representedObject.ownerRule.sourceCodeLocation, options);
            else
                WI.showSourceCodeLocation(this.representedObject.ownerRule.sourceCodeLocation, options);
        });

        // Only used one colon temporarily since single-colon pseudo elements are valid CSS.
        if (WI.CSSStyleManager.PseudoElementNames.some((className) => this.representedObject.selectorText.includes(":" + className)))
            return;

        if (WI.CSSStyleManager.ForceablePseudoClasses.every((className) => !this.representedObject.selectorText.includes(":" + className))) {
            contextMenu.appendSeparator();

            for (let pseudoClass of WI.CSSStyleManager.ForceablePseudoClasses) {
                if (pseudoClass === "visited" && this.representedObject.node.nodeName() !== "A")
                    continue;

                let pseudoClassSelector = ":" + pseudoClass;

                contextMenu.appendItem(WI.UIString("Add %s Rule").format(pseudoClassSelector), () => {
                    this.representedObject.node.setPseudoClassEnabled(pseudoClass, true);
                    let pseudoSelectors = this.representedObject.ownerRule.selectors.map((selector) => selector.text + pseudoClassSelector);
                    this.representedObject.nodeStyles.addRule(pseudoSelectors.join(", "));
                });
            }
        }

        contextMenu.appendSeparator();

        for (let pseudoElement of WI.CSSStyleManager.PseudoElementNames) {
            let pseudoElementSelector = "::" + pseudoElement;
            const styleText = "content: \"\";";

            let existingTreeItem = null;
            if (this._delegate && typeof this._delegate.treeItemForStyle === "function") {
                let selectorText = this.representedObject.ownerRule.selectorText;
                let existingRules = this.representedObject.nodeStyles.rulesForSelector(selectorText + pseudoElementSelector);
                if (existingRules.length) {
                    // There shouldn't really ever be more than one pseudo-element rule
                    // that is not in a media query. As such, just focus the first rule
                    // on the assumption that it is the only one necessary.
                    existingTreeItem = this._delegate.treeItemForStyle(existingRules[0].style);
                }
            }

            let title = existingTreeItem ? WI.UIString("Select %s Rule") : WI.UIString("Create %s Rule");
            contextMenu.appendItem(title.format(pseudoElementSelector), () => {
                if (existingTreeItem) {
                    existingTreeItem.select(true, true);
                    return;
                }

                let pseudoSelectors = this.representedObject.ownerRule.selectors.map((selector) => selector.text + pseudoElementSelector);
                this.representedObject.nodeStyles.addRule(pseudoSelectors.join(", "), styleText);
            });
        }

        super.populateContextMenu(contextMenu, event);
    }

    // Private

    _highlightNodesWithSelector()
    {
        if (!this.representedObject.ownerRule) {
            WI.domTreeManager.highlightDOMNode(this.representedObject.node.id);
            return;
        }

        WI.domTreeManager.highlightSelector(this.selectorText, this.representedObject.node.ownerDocument.frameIdentifier);
    }

    _hideDOMNodeHighlight()
    {
        WI.domTreeManager.hideDOMNodeHighlight();
    }

    _handleCheckboxChanged(event)
    {
        this._updateCheckboxTitle();
        this.dispatchEventToListeners(WI.VisualStyleSelectorTreeItem.Event.CheckboxChanged, {enabled: this._checkboxElement.checked});
    }

    _updateCheckboxTitle()
    {
        if (this._checkboxElement.checked)
            this._checkboxElement.title = WI.UIString("Comment out rule");
        else
            this._checkboxElement.title = WI.UIString("Uncomment rule");
    }

    _handleMainTitleMouseDown(event)
    {
        if (event.button !== 0 || event.ctrlKey)
            return;

        this._listItemNode.classList.toggle("editable", this.selected);
        this._mainTitleElement.contentEditable = this.selected ? "plaintext-only" : false;
    }

    _handleMainTitleKeyDown(event)
    {
        this._highlightNodesWithSelector();

        let enterKeyCode = WI.KeyboardShortcut.Key.Enter.keyCode;
        if (event.keyCode === enterKeyCode)
            this._mainTitleElement.blur();
    }

    _commitSelector()
    {
        this._hideDOMNodeHighlight();
        this._listItemNode.classList.remove("editable");
        this._mainTitleElement.contentEditable = false;
        this._updateTitleTooltip();

        let value = this.selectorText;
        if (value === this._lastValue && !this._hasInvalidSelector)
            return;

        this.representedObject.ownerRule.selectorText = value;
    }

    _styleTextModified()
    {
        this._listItemNode.classList.toggle("modified", this.representedObject.modified);
    }

    _updateSelectorIcon(event)
    {
        this._hasInvalidSelector = event && event.data && !event.data.valid;
        this._listItemNode.classList.toggle("selector-invalid", !!this._hasInvalidSelector);
        if (this._hasInvalidSelector) {
            this._iconElement.title = WI.UIString("The selector “%s” is invalid.\nClick to revert to the previous selector.").format(this.selectorText);
            this.mainTitleElement.title = WI.UIString("Using previous selector “%s”").format(this.representedObject.ownerRule.selectorText);
            return;
        }

        this._iconElement.title = null;
        this.mainTitleElement.title = null;

        let hasMatchedPseudoElementSelector = this.representedObject.ownerRule && this.representedObject.ownerRule.hasMatchedPseudoElementSelector();
        this._iconClasses.toggleIncludes(WI.CSSStyleDeclarationSection.PseudoElementSelectorStyleClassName, hasMatchedPseudoElementSelector);
    }

    _handleIconElementClicked(event)
    {
        if (this._hasInvalidSelector && this.representedObject.ownerRule) {
            this.mainTitleElement.textContent = this._lastValue = this.representedObject.ownerRule.selectorText;
            this._updateSelectorIcon();
            return;
        }
    }
};

WI.VisualStyleSelectorTreeItem.Event = {
    CheckboxChanged: "visual-style-selector-item-checkbox-changed"
};
