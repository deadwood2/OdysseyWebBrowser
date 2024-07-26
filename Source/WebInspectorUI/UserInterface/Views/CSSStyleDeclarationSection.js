/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.CSSStyleDeclarationSection = function(delegate, style)
{
    // FIXME: Convert this to a WebInspector.Object subclass, and call super().
    // WebInspector.Object.call(this);

    this._delegate = delegate || null;

    console.assert(style);
    this._style = style || null;
    this._selectorElements = [];
    this._ruleDisabled = false;

    this._element = document.createElement("div");
    this._element.className = "style-declaration-section";

    this._headerElement = document.createElement("div");
    this._headerElement.className = "header";

    this._iconElement = document.createElement("img");
    this._iconElement.className = "icon";
    this._headerElement.appendChild(this._iconElement);

    this._selectorElement = document.createElement("span");
    this._selectorElement.className = "selector";
    this._selectorElement.setAttribute("spellcheck", "false");
    this._selectorElement.addEventListener("mouseover", this._handleMouseOver.bind(this));
    this._selectorElement.addEventListener("mouseout", this._handleMouseOut.bind(this));
    this._selectorElement.addEventListener("keydown", this._handleKeyDown.bind(this));
    this._selectorElement.addEventListener("keyup", this._handleKeyUp.bind(this));
    this._selectorElement.addEventListener("paste", this._handleSelectorPaste.bind(this));
    this._headerElement.appendChild(this._selectorElement);

    this._originElement = document.createElement("span");
    this._originElement.className = "origin";
    this._headerElement.appendChild(this._originElement);

    this._propertiesElement = document.createElement("div");
    this._propertiesElement.className = "properties";

    this._propertiesTextEditor = new WebInspector.CSSStyleDeclarationTextEditor(this, style);
    this._propertiesElement.appendChild(this._propertiesTextEditor.element);

    this._element.appendChild(this._headerElement);
    this._element.appendChild(this._propertiesElement);

    var iconClassName;
    switch (style.type) {
    case WebInspector.CSSStyleDeclaration.Type.Rule:
        console.assert(style.ownerRule);

        if (style.inherited)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InheritedStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.Author)
            iconClassName = WebInspector.CSSStyleDeclarationSection.AuthorStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.User)
            iconClassName = WebInspector.CSSStyleDeclarationSection.UserStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.UserAgent)
            iconClassName = WebInspector.CSSStyleDeclarationSection.UserAgentStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.Inspector)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InspectorStyleRuleIconStyleClassName;
        break;

    case WebInspector.CSSStyleDeclaration.Type.Inline:
    case WebInspector.CSSStyleDeclaration.Type.Attribute:
        if (style.inherited)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InheritedElementStyleRuleIconStyleClassName;
        else
            iconClassName = WebInspector.DOMTreeElementPathComponent.DOMElementIconStyleClassName;
        break;
    }

    // Matches all situations except for User Agent styles.
    if (!(style.ownerRule && style.ownerRule.type === WebInspector.CSSRule.Type.UserAgent)) {
        this._iconElement.classList.add("toggle-able");
        this._iconElement.title = WebInspector.UIString("Comment All Properties");
        this._iconElement.addEventListener("click", this._toggleRuleOnOff.bind(this));
    }

    console.assert(iconClassName);
    this._element.classList.add(iconClassName);

    if (!style.editable)
        this._element.classList.add(WebInspector.CSSStyleDeclarationSection.LockedStyleClassName);
    else if (style.ownerRule) {
        this._style.ownerRule.addEventListener(WebInspector.CSSRule.Event.SelectorChanged, this._markSelector.bind(this));
        this._commitSelectorKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Enter, this._commitSelector.bind(this), this._selectorElement);
        this._selectorElement.addEventListener("blur", this._commitSelector.bind(this));
    } else
        this._element.classList.add(WebInspector.CSSStyleDeclarationSection.SelectorLockedStyleClassName);

    if (!WebInspector.CSSStyleDeclarationSection._generatedLockImages) {
        WebInspector.CSSStyleDeclarationSection._generatedLockImages = true;

        var specifications = {"style-lock-normal": {fillColor: [0, 0, 0, 0.5]}};
        generateColoredImagesForCSS("Images/Locked.svg", specifications, 8, 10);
    }

    this.refresh();

    this._headerElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this));
};

WebInspector.CSSStyleDeclarationSection.LockedStyleClassName = "locked";
WebInspector.CSSStyleDeclarationSection.SelectorLockedStyleClassName = "selector-locked";
WebInspector.CSSStyleDeclarationSection.SelectorInvalidClassName = "invalid-selector";
WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName = "last-in-group";
WebInspector.CSSStyleDeclarationSection.MatchedSelectorElementStyleClassName = "matched";

WebInspector.CSSStyleDeclarationSection.AuthorStyleRuleIconStyleClassName = "author-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.UserStyleRuleIconStyleClassName = "user-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.UserAgentStyleRuleIconStyleClassName = "user-agent-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InspectorStyleRuleIconStyleClassName = "inspector-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InheritedStyleRuleIconStyleClassName = "inherited-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InheritedElementStyleRuleIconStyleClassName = "inherited-element-style-rule-icon";

WebInspector.CSSStyleDeclarationSection.prototype = {
    constructor: WebInspector.CSSStyleDeclarationSection,

    // Public

    get element()
    {
        return this._element;
    },

    get style()
    {
        return this._style;
    },

    get lastInGroup()
    {
        return this._element.classList.contains(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
    },

    set lastInGroup(last)
    {
        if (last)
            this._element.classList.add(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
        else
            this._element.classList.remove(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
    },

    get focused()
    {
        return this._propertiesTextEditor.focused;
    },

    focus: function()
    {
        this._propertiesTextEditor.focus();
    },

    refresh: function()
    {
        this._selectorElement.removeChildren();
        this._originElement.removeChildren();
        this._selectorElements = [];

        this._originElement.append(" \u2014 ");

        function appendSelector(selector, matched)
        {
            console.assert(selector instanceof WebInspector.CSSSelector);

            var selectorElement = document.createElement("span");
            selectorElement.textContent = selector.text;

            if (matched)
                selectorElement.className = WebInspector.CSSStyleDeclarationSection.MatchedSelectorElementStyleClassName;

            var specificity = selector.specificity;
            if (specificity) {
                var tooltip = WebInspector.UIString("Specificity: (%d, %d, %d)").format(specificity[0], specificity[1], specificity[2]);
                if (selector.dynamic) {
                    tooltip += "\n";
                    if (this._style.inherited)
                        tooltip += WebInspector.UIString("Dynamically calculated for the parent element");
                    else
                        tooltip += WebInspector.UIString("Dynamically calculated for the selected element");
                }
                selectorElement.title = tooltip;
            } else if (selector.dynamic) {
                var tooltip = WebInspector.UIString("Specificity: No value for selected element");
                tooltip += "\n";
                tooltip += WebInspector.UIString("Dynamically calculated for the selected element and did not match");
                selectorElement.title = tooltip;
            }

            this._selectorElement.appendChild(selectorElement);
            this._selectorElements.push(selectorElement);
        }

        function appendSelectorTextKnownToMatch(selectorText)
        {
            var selectorElement = document.createElement("span");
            selectorElement.textContent = selectorText;
            selectorElement.className = WebInspector.CSSStyleDeclarationSection.MatchedSelectorElementStyleClassName;
            this._selectorElement.appendChild(selectorElement);
        }

        switch (this._style.type) {
        case WebInspector.CSSStyleDeclaration.Type.Rule:
            console.assert(this._style.ownerRule);

            var selectors = this._style.ownerRule.selectors;
            var matchedSelectorIndices = this._style.ownerRule.matchedSelectorIndices;
            var alwaysMatch = !matchedSelectorIndices.length;
            if (selectors.length) {
                for (var i = 0; i < selectors.length; ++i) {
                    appendSelector.call(this, selectors[i], alwaysMatch || matchedSelectorIndices.includes(i));
                    if (i < selectors.length - 1)
                        this._selectorElement.append(", ");
                }
            } else
                appendSelectorTextKnownToMatch.call(this, this._style.ownerRule.selectorText);

            if (this._style.ownerRule.sourceCodeLocation) {
                var sourceCodeLink = WebInspector.createSourceCodeLocationLink(this._style.ownerRule.sourceCodeLocation, true);
                this._originElement.appendChild(sourceCodeLink);
            } else {
                var originString;
                switch (this._style.ownerRule.type) {
                case WebInspector.CSSRule.Type.Author:
                    originString = WebInspector.UIString("Author Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.User:
                    originString = WebInspector.UIString("User Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.UserAgent:
                    originString = WebInspector.UIString("User Agent Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.Inspector:
                    originString = WebInspector.UIString("Web Inspector");
                    break;
                }

                console.assert(originString);
                if (originString)
                    this._originElement.append(originString);
            }

            break;

        case WebInspector.CSSStyleDeclaration.Type.Inline:
            appendSelectorTextKnownToMatch.call(this, WebInspector.displayNameForNode(this._style.node));
            this._originElement.append(WebInspector.UIString("Style Attribute"));
            break;

        case WebInspector.CSSStyleDeclaration.Type.Attribute:
            appendSelectorTextKnownToMatch.call(this, WebInspector.displayNameForNode(this._style.node));
            this._originElement.append(WebInspector.UIString("HTML Attributes"));
            break;
        }
    },

    highlightProperty: function(property)
    {
        if (this._propertiesTextEditor.highlightProperty(property)) {
            this._element.scrollIntoView();
            return true;
        }

        return false;
    },

    findMatchingPropertiesAndSelectors: function(needle)
    {
        this._element.classList.remove(WebInspector.CSSStyleDetailsSidebarPanel.NoFilterMatchInSectionClassName, WebInspector.CSSStyleDetailsSidebarPanel.FilterMatchingSectionHasLabelClassName);

        var hasMatchingSelector = false;

        for (var selectorElement of this._selectorElements) {
            selectorElement.classList.remove(WebInspector.CSSStyleDetailsSidebarPanel.FilterMatchSectionClassName);

            if (needle && selectorElement.textContent.includes(needle)) {
                selectorElement.classList.add(WebInspector.CSSStyleDetailsSidebarPanel.FilterMatchSectionClassName);
                hasMatchingSelector = true;
            }
        }

        if (!needle) {
            this._propertiesTextEditor.resetFilteredProperties();
            return false;
        }

        var hasMatchingProperty = this._propertiesTextEditor.findMatchingProperties(needle);

        if (!hasMatchingProperty && !hasMatchingSelector) {
            this._element.classList.add(WebInspector.CSSStyleDetailsSidebarPanel.NoFilterMatchInSectionClassName);
            return false;
        }

        return true;
    },

    updateLayout: function()
    {
        this._propertiesTextEditor.updateLayout();
    },

    clearSelection: function()
    {
        this._propertiesTextEditor.clearSelection();
    },

    cssStyleDeclarationTextEditorFocused: function()
    {
        if (typeof this._delegate.cssStyleDeclarationSectionEditorFocused === "function")
            this._delegate.cssStyleDeclarationSectionEditorFocused(this);
    },

    cssStyleDeclarationTextEditorSwitchRule: function(reverse)
    {
        if (!this._delegate)
            return;

        if (reverse && typeof this._delegate.cssStyleDeclarationSectionEditorPreviousRule === "function")
            this._delegate.cssStyleDeclarationSectionEditorPreviousRule(this);
        else if (!reverse && typeof this._delegate.cssStyleDeclarationSectionEditorNextRule === "function")
            this._delegate.cssStyleDeclarationSectionEditorNextRule(this);
    },

    focusRuleSelector: function(reverse)
    {
        if (this.selectorLocked) {
            this.focus();
            return;
        }

        if (this.locked) {
            this.cssStyleDeclarationTextEditorSwitchRule(reverse);
            return;
        }

        var selection = window.getSelection();
        selection.removeAllRanges();

        this._element.scrollIntoViewIfNeeded();

        var range = document.createRange();
        range.selectNodeContents(this._selectorElement);
        selection.addRange(range);
    },

    selectLastProperty: function()
    {
        this._propertiesTextEditor.selectLastProperty();
    },

    get selectorLocked()
    {
        return !this.locked && !this._style.ownerRule;
    },

    get locked()
    {
        return !this._style.editable;
    },

    // Private

    get _currentSelectorText()
    {
        if (!this._style.ownerRule)
            return;

        var selectorText = this._selectorElement.textContent;
        if (!selectorText || !selectorText.length)
            selectorText = this._style.ownerRule.selectorText;

        return selectorText.trim();
    },

    _handleSelectorPaste: function(event)
    {
        if (this._style.type === WebInspector.CSSStyleDeclaration.Type.Inline || !this._style.ownerRule)
            return;

        if (!event || !event.clipboardData)
            return;

        var data = event.clipboardData.getData("text/plain");
        if (!data)
            return;

        function parseTextForRule(text)
        {
            var containsBraces = /[\{\}]/;
            if (!containsBraces.test(text))
                return null;

            var match = text.match(/([^{]+){(.*)}/);
            if (!match)
                return null;

            // If the match "body" contains braces, parse that body as if it were a rule.
            // This will usually happen if the user includes a media query in the copied text.
            return containsBraces.test(match[2]) ? parseTextForRule(match[2]) : match;
        }

        var match = parseTextForRule(data);
        if (!match)
            return;

        var selector = match[1].trim();
        this._selectorElement.textContent = selector;
        this._style.nodeStyles.changeRule(this._style.ownerRule, selector, match[2]);
        event.preventDefault();
    },

    _handleContextMenuEvent: function(event)
    {
        if (window.getSelection().toString().length)
            return;

        var contextMenu = new WebInspector.ContextMenu(event);

        contextMenu.appendItem(WebInspector.UIString("Copy Rule"), function() {
            InspectorFrontendHost.copyText(this._generateCSSRuleString());
        }.bind(this));

        contextMenu.show();
    },

    _generateCSSRuleString: function()
    {
        var numMediaQueries = 0;
        var styleText = "";

        if (this._style.ownerRule) {
            var mediaList = this._style.ownerRule.mediaList;
            if (mediaList.length) {
                numMediaQueries = mediaList.length;

                for (var i = numMediaQueries - 1; i >= 0; --i)
                    styleText += "    ".repeat(numMediaQueries - i - 1) + "@media " + mediaList[i].text + " {\n";
            }

            styleText += "    ".repeat(numMediaQueries) + this._style.ownerRule.selectorText;
        } else
            styleText += this._selectorElement.textContent;

        styleText += " {\n";

        for (var property of this._style.visibleProperties) {
            styleText += "    ".repeat(numMediaQueries + 1) + property.text.trim();

            if (!styleText.endsWith(";"))
                styleText += ";";

            styleText += "\n";
        }

        for (var i = numMediaQueries; i > 0; --i)
            styleText += "    ".repeat(i) + "}\n";

        styleText += "}";

        return styleText;
    },

    _toggleRuleOnOff: function()
    {
        if (this._hasInvalidSelector)
            return;

        this._ruleDisabled = this._ruleDisabled ? !this._propertiesTextEditor.uncommentAllProperties() : this._propertiesTextEditor.commentAllProperties();
        this._iconElement.title = this._ruleDisabled ? WebInspector.UIString("Uncomment All Properties") : WebInspector.UIString("Comment All Properties");
        this._element.classList.toggle("rule-disabled", this._ruleDisabled);
    },

    _highlightNodesWithSelector: function()
    {
        if (!this._style.ownerRule) {
            WebInspector.domTreeManager.highlightDOMNode(this._style.node.id);
            return;
        }

        WebInspector.domTreeManager.highlightSelector(this._currentSelectorText, this._style.node.ownerDocument.frameIdentifier);
    },

    _hideDOMNodeHighlight: function()
    {
        WebInspector.domTreeManager.hideDOMNodeHighlight();
    },

    _handleMouseOver: function(event)
    {
        this._highlightNodesWithSelector();
    },

    _handleMouseOut: function(event)
    {
        this._hideDOMNodeHighlight();
    },

    _handleKeyDown: function(event)
    {
        if (event.keyCode !== 9) {
            this._highlightNodesWithSelector();
            return;
        }

        if (event.shiftKey && this._delegate && typeof this._delegate.cssStyleDeclarationSectionEditorPreviousRule === "function") {
            event.preventDefault();
            this._delegate.cssStyleDeclarationSectionEditorPreviousRule(this, true);
            return;
        }

        if (!event.metaKey) {
            event.preventDefault();
            this.focus();
            this._propertiesTextEditor.selectFirstProperty();
            return;
        }
    },

    _handleKeyUp: function(event)
    {
        this._highlightNodesWithSelector();
    },

    _commitSelector: function(mutations)
    {
        console.assert(this._style.ownerRule);
        if (!this._style.ownerRule)
            return;

        var newSelectorText = this._selectorElement.textContent.trim();
        if (!newSelectorText) {
            // Revert to the current selector (by doing a refresh) since the new selector is empty.
            this.refresh();
            return;
        }

        this._style.ownerRule.selectorText = newSelectorText;
    },

    _markSelector: function(event)
    {
        var valid = event && event.data && event.data.valid;
        this._element.classList.toggle(WebInspector.CSSStyleDeclarationSection.SelectorInvalidClassName, !valid);
        if (valid) {
            this._iconElement.title = this._ruleDisabled ? WebInspector.UIString("Uncomment All Properties") : WebInspector.UIString("Comment All Properties");
            this._selectorElement.title = null;
            this.refresh();
            return;
        }

        this._iconElement.title = WebInspector.UIString("The selector '%s' is invalid.").format(this._selectorElement.textContent.trim());
        this._selectorElement.title = WebInspector.UIString("Using the previous selector '%s'.").format(this._style.ownerRule.selectorText);
        for (var i = 0; i < this._selectorElement.children.length; ++i)
            this._selectorElement.children[i].title = null;
    },

    get _hasInvalidSelector()
    {
        return this._element.classList.contains(WebInspector.CSSStyleDeclarationSection.SelectorInvalidClassName);
    }
};

WebInspector.CSSStyleDeclarationSection.prototype.__proto__ = WebInspector.StyleDetailsPanel.prototype;
