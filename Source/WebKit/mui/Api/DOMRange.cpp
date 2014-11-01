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
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES
 * LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMRange.h"
#include <wtf/text/CString.h>
#include <ExceptionCode.h>
#include <Range.h>

using namespace WebCore;

DOMRange* DOMRange::createInstance(WebCore::Range* range)
{
    return new DOMRange(range);
}

DOMRange::DOMRange(WebCore::Range* range)
    : m_range(range)
{
}

DOMRange::~DOMRange()
{
}

DOMNode* DOMRange::startContainer()
{
    if (!m_range)
        return 0;
    return DOMNode::createInstance(m_range->startContainer());
}

int DOMRange::startOffset()
{
    if (!m_range)
        return 0;
    return m_range->startOffset();
}

DOMNode* DOMRange::endContainer()
{
    if (!m_range)
        return 0;
    return DOMNode::createInstance(m_range->endContainer());
}

int DOMRange::endOffset()
{
    if (!m_range)
        return 0;
    return m_range->endOffset();
}

bool DOMRange::collapsed()
{
    if (!m_range)
        return false;
    ExceptionCode ex;
    return m_range->collapsed(ex);
}

DOMNode* DOMRange::commonAncestorContainer()
{
    if (!m_range)
        return 0;
    ExceptionCode ex;
    return DOMNode::createInstance(m_range->commonAncestorContainer(ex));
}

void DOMRange::setStart(DOMNode* refNode, int offset)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setStart(refNode->node(), offset, ex);
}

void DOMRange::setEnd(DOMNode* refNode, int offset)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setEnd(refNode->node(), offset, ex);
}

void DOMRange::setStartBefore(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setStartBefore(refNode->node(), ex);
}

void DOMRange::setStartAfter(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setStartAfter(refNode->node(), ex);
}

void DOMRange::setEndBefore(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setEndBefore(refNode->node(), ex);
}

void DOMRange::setEndAfter(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->setEndAfter(refNode->node(), ex);
}

void DOMRange::collapse(bool toStart)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->collapse(toStart, ex);
}

void DOMRange::selectNode(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->selectNode(refNode->node(), ex);
}

void DOMRange::selectNodeContents(DOMNode* refNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->selectNodeContents(refNode->node(), ex);
}

void DOMRange::compareBoundaryPoints(unsigned short how, DOMRange* sourceRange)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->compareBoundaryPoints((Range::CompareHow)how, sourceRange->range(), ex);
}

void DOMRange::deleteContents()
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->deleteContents(ex);
}

/*DOMDocumentFragment* DOMRange::extractContents()
{
    return 0;
}

DOMDocumentFragment* DOMRange::cloneContents()
{
    return 0;
}*/

void DOMRange::insertNode(DOMNode* newNode)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->insertNode(newNode->node(), ex);
}

void DOMRange::surroundContents(DOMNode* newParent)
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->surroundContents(newParent->node(), ex);
}

DOMRange* DOMRange::cloneRange()
{
    if (!m_range)
        return 0;
    ExceptionCode ex;
    return DOMRange::createInstance(m_range->cloneRange(ex).get());
}

const char* DOMRange::toString()
{
    if (!m_range)
        return "";
    ExceptionCode ex;
    return m_range->toString(ex).utf8().data();
}

void DOMRange::detach()
{
    if (!m_range)
        return ;
    ExceptionCode ex;
    m_range->detach(ex);
}

