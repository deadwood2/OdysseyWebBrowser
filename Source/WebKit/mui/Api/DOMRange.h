/*
 * Copyright (C) 2009 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef DOMRange_H
#define DOMRange_H

#include "WebKitTypes.h"
#include "DOMCoreClasses.h"

namespace WebCore {
    class Range;
}

typedef enum {
   WebSelectionAffinityUpstream = 0,
   WebSelectionAffinityDownstream = 1
} WebSelectionAffinity;

enum {
    //
    // DOM range exception codes
    //
    DOM_BAD_BOUNDARYPOINTS_ERR        = 1,
    DOM_INVALID_NODE_TYPE_ERR         = 2,
};

enum {
    //
    // DOM Range comparison codes
    //
    DOM_START_TO_START                = 0,
    DOM_START_TO_END                  = 1,
    DOM_END_TO_END                    = 2,
    DOM_END_TO_START                  = 3,
};

class WEBKIT_OWB_API DOMRange : public DOMObject
{
protected:
    friend class WebEditorClient;
    static DOMRange* createInstance(WebCore::Range*);
    WebCore::Range* range() {return m_range;}
public:
    ~DOMRange();

    /*
     * startContainer;
     */
    DOMNode* startContainer();

    /*
     * startOffset;
     */
    int startOffset();

    /*
     * endContainer;
     */
    DOMNode* endContainer();

    /*
     * endOffset;
     */
    int endOffset();

    /*
     * collapsed;
     */
    bool collapsed();

    /*
     * commonAncestorContainer;
     */
    DOMNode* commonAncestorContainer();

    /*
     * setStart
     */
    void setStart(DOMNode* refNode, int offset);

    /*
     *setEnd
     */
    void setEnd(DOMNode* refNode, int offset);

    /*
     * setStartBefore
     */
    void setStartBefore(DOMNode* refNode);

    /*
     *setStartAfter;
     */
    void setStartAfter(DOMNode* refNode);

    /*
     *setEndBefore;
     */
    void setEndBefore(DOMNode* refNode);

    /*
     *setEndAfter;
     */
    void setEndAfter(DOMNode* refNode);

    /*
     *collapse;
     */
    void collapse(bool toStart);

    /*
     *selectNode;
     */
    void selectNode(DOMNode* refNode);

    /*
     * selectNodeContents;
     */
    void selectNodeContents(DOMNode* refNode);

    /*
     * compareBoundaryPoints;
     */
    void compareBoundaryPoints(unsigned short how, DOMRange* sourceRange);

    /*
     *deleteContents;
     */
    void deleteContents();

    /*
     * extractContents;
     */
//    DOMDocumentFragment* extractContents();

    /*
     * cloneContents;
     */
//    DOMDocumentFragment* cloneContents();

    /*
     * insertNode
     */
    void insertNode(DOMNode* newNode);

    /*
     * surroundContents
     */
    void surroundContents(DOMNode* newParent);

    /*
     * cloneRange;
     */
    DOMRange* cloneRange();

    /*
     * toString;
     */
    const char* toString();

    /*
     * detach;
     */
    void detach();
private:
    DOMRange(WebCore::Range*);
    WebCore::Range* m_range;
};

#endif //DOMRange
