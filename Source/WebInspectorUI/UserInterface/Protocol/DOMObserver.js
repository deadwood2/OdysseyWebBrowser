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

WI.DOMObserver = class DOMObserver
{
    // Events defined by the "DOM" domain.

    documentUpdated()
    {
        WI.domTreeManager._documentUpdated();
    }

    inspect(nodeId)
    {
        WI.domTreeManager.inspectElement(nodeId);
    }

    setChildNodes(parentId, payloads)
    {
        WI.domTreeManager._setChildNodes(parentId, payloads);
    }

    attributeModified(nodeId, name, value)
    {
        WI.domTreeManager._attributeModified(nodeId, name, value);
    }

    attributeRemoved(nodeId, name)
    {
        WI.domTreeManager._attributeRemoved(nodeId, name);
    }

    inlineStyleInvalidated(nodeIds)
    {
        WI.domTreeManager._inlineStyleInvalidated(nodeIds);
    }

    characterDataModified(nodeId, characterData)
    {
        WI.domTreeManager._characterDataModified(nodeId, characterData);
    }

    childNodeCountUpdated(nodeId, childNodeCount)
    {
        WI.domTreeManager._childNodeCountUpdated(nodeId, childNodeCount);
    }

    childNodeInserted(parentNodeId, previousNodeId, payload)
    {
        WI.domTreeManager._childNodeInserted(parentNodeId, previousNodeId, payload);
    }

    childNodeRemoved(parentNodeId, nodeId)
    {
        WI.domTreeManager._childNodeRemoved(parentNodeId, nodeId);
    }

    shadowRootPushed(parentNodeId, nodeId)
    {
        WI.domTreeManager._childNodeInserted(parentNodeId, 0, nodeId);
    }

    shadowRootPopped(parentNodeId, nodeId)
    {
        WI.domTreeManager._childNodeRemoved(parentNodeId, nodeId);
    }

    customElementStateChanged(nodeId, customElementState)
    {
        WI.domTreeManager._customElementStateChanged(nodeId, customElementState);
    }

    pseudoElementAdded(parentNodeId, pseudoElement)
    {
        WI.domTreeManager._pseudoElementAdded(parentNodeId, pseudoElement);
    }

    pseudoElementRemoved(parentNodeId, pseudoElementId)
    {
        WI.domTreeManager._pseudoElementRemoved(parentNodeId, pseudoElementId);
    }

    didAddEventListener(nodeId)
    {
        WI.domTreeManager.didAddEventListener(nodeId);
    }

    willRemoveEventListener(nodeId)
    {
        WI.domTreeManager.willRemoveEventListener(nodeId);
    }
};
