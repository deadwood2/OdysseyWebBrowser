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

WI.CSSStyleDeclaration = class CSSStyleDeclaration extends WI.Object
{
    constructor(nodeStyles, ownerStyleSheet, id, type, node, inherited, text, properties, styleSheetTextRange)
    {
        super();

        console.assert(nodeStyles);
        this._nodeStyles = nodeStyles;

        this._ownerRule = null;

        this._ownerStyleSheet = ownerStyleSheet || null;
        this._id = id || null;
        this._type = type || null;
        this._node = node || null;
        this._inherited = inherited || false;

        this._locked = false;
        this._pendingProperties = [];
        this._propertyNameMap = {};

        this._initialText = text;
        this._hasModifiedInitialText = false;

        this._allProperties = [];
        this._allVisibleProperties = null;

        this.update(text, properties, styleSheetTextRange, {dontFireEvents: true});
    }

    // Public

    get id()
    {
        return this._id;
    }

    get ownerStyleSheet()
    {
        return this._ownerStyleSheet;
    }

    get type()
    {
        return this._type;
    }

    get inherited()
    {
        return this._inherited;
    }

    get node()
    {
        return this._node;
    }

    get editable()
    {
        if (!this._id)
            return false;

        if (this._type === WI.CSSStyleDeclaration.Type.Rule)
            return this._ownerRule && this._ownerRule.editable;

        if (this._type === WI.CSSStyleDeclaration.Type.Inline)
            return !this._node.isInUserAgentShadowTree();

        return false;
    }

    get selectorEditable()
    {
        return this._ownerRule && this._ownerRule.editable;
    }

    get locked() { return this._locked; }
    set locked(value) { this._locked = value; }

    update(text, properties, styleSheetTextRange, options = {})
    {
        let dontFireEvents = options.dontFireEvents || false;
        let suppressLock = options.suppressLock || false;

        if (this._locked && !suppressLock && text !== this._text)
            return;

        text = text || "";
        properties = properties || [];

        var oldProperties = this._properties || [];
        var oldText = this._text;

        this._text = text;
        this._properties = properties.filter((property) => property.enabled);
        this._allProperties = properties;

        this._styleSheetTextRange = styleSheetTextRange;
        this._propertyNameMap = {};

        delete this._visibleProperties;
        this._allVisibleProperties = null;

        var editable = this.editable;

        for (let property of this._allProperties) {
            property.ownerStyle = this;

            // Store the property in a map if we aren't editable. This
            // allows for quick lookup for computed style. Editable
            // styles don't use the map since they need to account for
            // overridden properties.
            if (!editable)
                this._propertyNameMap[property.name] = property;
            else {
                // Remove from pendingProperties (if it was pending).
                this._pendingProperties.remove(property);
            }
        }

        var removedProperties = [];
        for (var i = 0; i < oldProperties.length; ++i) {
            var oldProperty = oldProperties[i];

            if (!this._properties.includes(oldProperty)) {
                // Clear the index, since it is no longer valid.
                oldProperty.index = NaN;

                removedProperties.push(oldProperty);

                // Keep around old properties in pending in case they
                // are needed again during editing.
                if (editable)
                    this._pendingProperties.push(oldProperty);
            }
        }

        if (dontFireEvents)
            return;

        var addedProperties = [];
        for (var i = 0; i < this._properties.length; ++i) {
            if (!oldProperties.includes(this._properties[i]))
                addedProperties.push(this._properties[i]);
        }

        // Don't fire the event if there is text and it hasn't changed.
        if (oldText && this._text && oldText === this._text) {
            if (!this._locked || suppressLock) {
                // We shouldn't have any added or removed properties in this case.
                console.assert(!addedProperties.length && !removedProperties.length);
            }

            if (!addedProperties.length && !removedProperties.length)
                return;
        }

        function delayed()
        {
            this.dispatchEventToListeners(WI.CSSStyleDeclaration.Event.PropertiesChanged, {addedProperties, removedProperties});
        }

        // Delay firing the PropertiesChanged event so DOMNodeStyles has a chance to mark overridden and associated properties.
        setTimeout(delayed.bind(this), 0);
    }

    get ownerRule()
    {
        return this._ownerRule;
    }

    set ownerRule(rule)
    {
        this._ownerRule = rule || null;
    }

    get text()
    {
        return this._text;
    }

    set text(text)
    {
        if (this._text === text)
            return;

        let trimmedText = WI.CSSStyleDeclaration.PrefixWhitespace + text.trim();
        if (this._text === trimmedText)
            return;

        if (trimmedText === WI.CSSStyleDeclaration.PrefixWhitespace || this._type === WI.CSSStyleDeclaration.Type.Inline)
            text = trimmedText;

        let modified = text !== this._initialText;
        if (modified !== this._hasModifiedInitialText) {
            this._hasModifiedInitialText = modified;
            this.dispatchEventToListeners(WI.CSSStyleDeclaration.Event.InitialTextModified);
        }

        // Update text immediately when it was modified via the styles sidebar.
        if (this._locked)
            this._text = text;

        this._nodeStyles.changeStyleText(this, text);
    }

    resetText()
    {
        this.text = this._initialText;
    }

    get modified()
    {
        return this._hasModifiedInitialText;
    }

    get properties()
    {
        return this._properties;
    }

    get allProperties() { return this._allProperties; }

    get allVisibleProperties()
    {
        if (!this._allVisibleProperties)
            this._allVisibleProperties = this._allProperties.filter((property) => !!property.styleDeclarationTextRange);

        return this._allVisibleProperties;
    }

    get visibleProperties()
    {
        if (!this._visibleProperties)
            this._visibleProperties = this._properties.filter((property) => !!property.styleDeclarationTextRange);

        return this._visibleProperties;
    }

    get pendingProperties()
    {
        return this._pendingProperties;
    }

    get styleSheetTextRange()
    {
        return this._styleSheetTextRange;
    }

    get mediaList()
    {
        if (this._ownerRule)
            return this._ownerRule.mediaList;
        return [];
    }

    get selectorText()
    {
        if (this._ownerRule)
            return this._ownerRule.selectorText;
        return this._node.appropriateSelectorFor(true);
    }

    propertyForName(name, dontCreateIfMissing)
    {
        console.assert(name);
        if (!name)
            return null;

        if (!this.editable)
            return this._propertyNameMap[name] || null;

        // Editable styles don't use the map since they need to
        // account for overridden properties.

        function findMatch(properties)
        {
            for (var i = 0; i < properties.length; ++i) {
                var property = properties[i];
                if (property.canonicalName !== name && property.name !== name)
                    continue;
                if (bestMatchProperty && !bestMatchProperty.overridden && property.overridden)
                    continue;
                bestMatchProperty = property;
            }
        }

        var bestMatchProperty = null;

        findMatch(this._properties);

        if (bestMatchProperty)
            return bestMatchProperty;

        if (dontCreateIfMissing || !this.editable)
            return null;

        findMatch(this._pendingProperties, true);

        if (bestMatchProperty)
            return bestMatchProperty;

        var newProperty = new WI.CSSProperty(NaN, null, name);
        newProperty.ownerStyle = this;

        this._pendingProperties.push(newProperty);

        return newProperty;
    }

    generateCSSRuleString()
    {
        let indentString = WI.indentString();
        let styleText = "";
        let mediaList = this.mediaList;
        let mediaQueriesCount = mediaList.length;
        for (let i = mediaQueriesCount - 1; i >= 0; --i)
            styleText += indentString.repeat(mediaQueriesCount - i - 1) + "@media " + mediaList[i].text + " {\n";

        styleText += indentString.repeat(mediaQueriesCount) + this.selectorText + " {\n";

        for (let property of this._properties) {
            if (property.anonymous)
                continue;

            styleText += indentString.repeat(mediaQueriesCount + 1) + property.text.trim();

            if (!styleText.endsWith(";"))
                styleText += ";";

            styleText += "\n";
        }

        for (let i = mediaQueriesCount; i > 0; --i)
            styleText += indentString.repeat(i) + "}\n";

        styleText += "}";

        return styleText;
    }

    isInspectorRule()
    {
        return this._ownerRule && this._ownerRule.type === WI.CSSStyleSheet.Type.Inspector;
    }

    hasProperties()
    {
        return !!this._properties.length;
    }

    newBlankProperty(propertyIndex)
    {
        let text, name, value, priority, overridden, implicit, anonymous;
        let enabled = true;
        let valid = false;
        let styleSheetTextRange = this._rangeAfterPropertyAtIndex(propertyIndex - 1);

        let property = new WI.CSSProperty(propertyIndex, text, name, value, priority, enabled, overridden, implicit, anonymous, valid, styleSheetTextRange);

        this._allProperties.insertAtIndex(property, propertyIndex);
        for (let index = propertyIndex + 1; index < this._allProperties.length; index++)
            this._allProperties[index].index = index;

        this.update(this._text, this._allProperties, this._styleSheetTextRange, {dontFireEvents: true, suppressLock: true});

        return property;
    }

    shiftPropertiesAfter(cssProperty, lineDelta, columnDelta, propertyWasRemoved)
    {
        // cssProperty.index could be set to NaN by WI.CSSStyleDeclaration.prototype.update.
        let realIndex = this._allProperties.indexOf(cssProperty);
        if (realIndex === -1)
            return;

        let endLine = cssProperty.styleSheetTextRange.endLine;

        for (let i = realIndex + 1; i < this._allProperties.length; i++) {
            let property = this._allProperties[i];

            if (property._styleSheetTextRange) {
                if (property.styleSheetTextRange.startLine === endLine) {
                    // Only update column data if it's on the same line.
                    property._styleSheetTextRange = property._styleSheetTextRange.cloneAndModify(lineDelta, columnDelta, lineDelta, columnDelta);
                } else
                    property._styleSheetTextRange = property._styleSheetTextRange.cloneAndModify(lineDelta, 0, lineDelta, 0);
            }

            if (propertyWasRemoved && !isNaN(property._index))
                property._index--;
        }

        if (propertyWasRemoved)
            this._allProperties.splice(realIndex, 1);

        // Invalidate cached properties.
        this._allVisibleProperties = null;
    }

    // Protected

    get nodeStyles()
    {
        return this._nodeStyles;
    }

    // Private

    _rangeAfterPropertyAtIndex(index)
    {
        if (index < 0)
            return this._styleSheetTextRange.collapseToStart();

        if (index >= this.allVisibleProperties.length)
            return this._styleSheetTextRange.collapseToEnd();

        let property = this.allVisibleProperties[index];
        return property.styleSheetTextRange.collapseToEnd();
    }
};

WI.CSSStyleDeclaration.Event = {
    PropertiesChanged: "css-style-declaration-properties-changed",
    InitialTextModified: "css-style-declaration-initial-text-modified"
};

WI.CSSStyleDeclaration.Type = {
    Rule: "css-style-declaration-type-rule",
    Inline: "css-style-declaration-type-inline",
    Attribute: "css-style-declaration-type-attribute",
    Computed: "css-style-declaration-type-computed"
};

WI.CSSStyleDeclaration.PrefixWhitespace = "\n";
