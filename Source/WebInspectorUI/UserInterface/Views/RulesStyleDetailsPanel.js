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

WebInspector.RulesStyleDetailsPanel = class RulesStyleDetailsPanel extends WebInspector.StyleDetailsPanel
{
    constructor()
    {
        super("rules", "rules", WebInspector.UIString("Rules"));

        this._sections = [];
    }

    // Public

    refresh(significantChange)
    {
        // We only need to do a rebuild on significant changes. Other changes are handled
        // by the sections and text editors themselves.
        if (!significantChange)
            return;

        var newSections = [];
        var newDOMFragment = document.createDocumentFragment();

        var previousMediaList = [];
        var previousSection = null;
        var previousFocusedSection = null;

        function mediaListsEqual(a, b)
        {
            a = a || [];
            b = b || [];

            if (a.length !== b.length)
                return false;

            for (var i = 0; i < a.length; ++i) {
                var aMedia = a[i];
                var bMedia = b[i];

                if (aMedia.type !== bMedia.type)
                    return false;

                if (aMedia.text !== bMedia.text)
                    return false;

                if (!aMedia.sourceCodeLocation && bMedia.sourceCodeLocation)
                    return false;

                if (aMedia.sourceCodeLocation && !aMedia.sourceCodeLocation.isEqual(bMedia.sourceCodeLocation))
                    return false;
            }

            return true;
        }

        function filteredMediaList(mediaList)
        {
            if (!mediaList)
                return [];

            // Exclude the basic "screen" query since it's very common and just clutters things.
            return mediaList.filter(function(media) {
                return media.text !== "screen";
            });
        }

        function uniqueOrderedStyles(orderedStyles)
        {
            var uniqueStyles = [];

            for (var style of orderedStyles) {
                var rule = style.ownerRule;
                if (!rule) {
                    uniqueStyles.push(style);
                    continue;
                }

                var found = false;
                for (var existingStyle of uniqueStyles) {
                    if (rule.isEqualTo(existingStyle.ownerRule)) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    uniqueStyles.push(style);
            }

            return uniqueStyles;
        }

        function appendStyleSection(style)
        {
            var section = style.__rulesSection;
            if (section && section.focused && !previousFocusedSection)
                previousFocusedSection = section;

            if (!section) {
                section = new WebInspector.CSSStyleDeclarationSection(style);
                style.__rulesSection = section;
            } else
                section.refresh();

            if (this._focusNextNewInspectorRule && style.ownerRule && style.ownerRule.type === WebInspector.CSSRule.Type.Inspector) {
                previousFocusedSection = section;
                delete this._focusNextNewInspectorRule;
            }

            // Reset lastInGroup in case the order/grouping changed.
            section.lastInGroup = false;

            newDOMFragment.appendChild(section.element);
            newSections.push(section);

            previousSection = section;
        }

        function addNewRuleButton()
        {
            if (previousSection)
                previousSection.lastInGroup = true;

            if (!this.nodeStyles.node.isInShadowTree()) {
                var newRuleButton = document.createElement("div");
                newRuleButton.className = "new-rule";
                newRuleButton.addEventListener("click", this._newRuleClicked.bind(this));

                newRuleButton.appendChild(document.createElement("img"));
                newRuleButton.appendChild(document.createTextNode(WebInspector.UIString("New Rule")));

                newDOMFragment.appendChild(newRuleButton);
            }

            addedNewRuleButton = true;
        }

        var pseudoElements = this.nodeStyles.pseudoElements;
        for (var pseudoIdentifier in pseudoElements) {
            var pseudoElement = pseudoElements[pseudoIdentifier];
            var orderedStyles = uniqueOrderedStyles(pseudoElement.orderedStyles);
            for (var style of orderedStyles)
                appendStyleSection.call(this, style);

            if (previousSection)
                previousSection.lastInGroup = true;
        }

        var addedNewRuleButton = false;

        var orderedStyles = uniqueOrderedStyles(this.nodeStyles.orderedStyles);
        for (var i = 0; i < orderedStyles.length; ++i) {
            var style = orderedStyles[i];

            if (style.type === WebInspector.CSSStyleDeclaration.Type.Rule && !addedNewRuleButton)
                addNewRuleButton.call(this);

            if (previousSection && previousSection.style.node !== style.node) {
                previousSection.lastInGroup = true;

                var prefixElement = document.createElement("strong");
                prefixElement.textContent = WebInspector.UIString("Inherited From: ");

                var inheritedLabel = document.createElement("div");
                inheritedLabel.className = "label";
                inheritedLabel.appendChild(prefixElement);
                inheritedLabel.appendChild(WebInspector.linkifyNodeReference(style.node));
                newDOMFragment.appendChild(inheritedLabel);
            }

            // Only include the media list if it is different from the previous media list shown.
            var currentMediaList = filteredMediaList(style.ownerRule && style.ownerRule.mediaList);
            if (!mediaListsEqual(previousMediaList, currentMediaList)) {
                previousMediaList = currentMediaList;

                // Break the section group even if the media list is empty. That way the user knows
                // the previous displayed media list does not apply to the next section.
                if (previousSection)
                    previousSection.lastInGroup = true;

                for (var j = 0; j < currentMediaList.length; ++j) {
                    var media = currentMediaList[j];

                    var prefixElement = document.createElement("strong");
                    prefixElement.textContent = WebInspector.UIString("Media: ");

                    var mediaLabel = document.createElement("div");
                    mediaLabel.className = "label";
                    mediaLabel.appendChild(prefixElement);
                    mediaLabel.appendChild(document.createTextNode(media.text));

                    if (media.sourceCodeLocation) {
                        mediaLabel.appendChild(document.createTextNode(" \u2014 "));
                        mediaLabel.appendChild(WebInspector.createSourceCodeLocationLink(media.sourceCodeLocation, true));
                    }

                    newDOMFragment.appendChild(mediaLabel);
                }
            }

            appendStyleSection.call(this, style);
        }

        if (!addedNewRuleButton)
            addNewRuleButton.call(this);

        if (previousSection)
            previousSection.lastInGroup = true;

        this.element.removeChildren();
        this.element.appendChild(newDOMFragment);

        this._sections = newSections;

        for (var i = 0; i < this._sections.length; ++i)
            this._sections[i].updateLayout();

        if (previousFocusedSection)
            previousFocusedSection.focus();
    }

    // Protected

    shown()
    {
        WebInspector.StyleDetailsPanel.prototype.shown.call(this);

        // Associate the style and section objects so they can be reused.
        // Also update the layout in case we changed widths while hidden.
        for (var i = 0; i < this._sections.length; ++i) {
            var section = this._sections[i];
            section.style.__rulesSection = section;
            section.updateLayout();
        }
    }

    hidden()
    {
        WebInspector.StyleDetailsPanel.prototype.hidden.call(this);

        // Disconnect the style and section objects so they have a chance
        // to release their objects when this panel is not visible.
        for (var i = 0; i < this._sections.length; ++i)
            delete this._sections[i].style.__rulesSection;
    }

    widthDidChange()
    {
        for (var i = 0; i < this._sections.length; ++i)
            this._sections[i].updateLayout();
    }

    // Private

    _newRuleClicked(event)
    {
        this._focusNextNewInspectorRule = true;
        this.nodeStyles.addEmptyRule();
    }
};
