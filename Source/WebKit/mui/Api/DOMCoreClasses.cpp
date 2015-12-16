/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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

#include "config.h"
#include "DOMCoreClasses.h"

#include "DOMCSSClasses.h"
#include "DOMEventsClasses.h"
#include "DOMHTMLClasses.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <DOMWindow.h>
#include <Document.h>
#include <Element.h>
#include <Font.h>
#include <HTMLFormElement.h>
#include <HTMLInputElement.h>
#include <HTMLNames.h>
#include <HTMLOptionElement.h>
#include <HTMLSelectElement.h>
#include <HTMLTextAreaElement.h>
#include <NodeList.h>
#include <RenderElement.h>


using namespace WebCore;
using namespace HTMLNames;

// DOMNode --------------------------------------------------------------------

const char* DOMNode::nodeName()
{
    if (!m_node)
        return "";
    return strdup(m_node->nodeName().utf8().data());
}

const char* DOMNode::nodeValue()
{
    if (!m_node)
        return "";
    return strdup(m_node->nodeValue().utf8().data());
}

void DOMNode::setNodeValue(const char*)
{
}

unsigned short DOMNode::nodeType()
{
    return 0;
}

DOMNode* DOMNode::parentNode()
{
    if (!m_node || !m_node->parentNode())
        return 0;
    return DOMNode::createInstance(m_node->parentNode());
}

DOMNodeList* DOMNode::childNodes()
{
    if (!m_node)
        return 0;

    return DOMNodeList::createInstance(m_node->childNodes().get());
}

DOMNode* DOMNode::firstChild()
{
    return 0;
}

DOMNode* DOMNode::lastChild()
{
    return 0;
}

DOMNode* DOMNode::previousSibling()
{
    return 0;
}

DOMNode* DOMNode::nextSibling()
{
    return 0;
}

/*DOMNamedNodeMap* DOMNode::attributes()
{
    return 0;
}*/

DOMDocument* DOMNode::ownerDocument()
{
    if (!m_node)
        return 0;
    return DOMDocument::createInstance(m_node->ownerDocument());
}

DOMNode* DOMNode::insertBefore(DOMNode* /*newChild*/, DOMNode* /*refChild*/)
{
    return 0;
}

DOMNode* DOMNode::replaceChild(DOMNode* /*newChild*/, DOMNode* /*oldChild*/)
{
    return 0;
}

DOMNode* DOMNode::removeChild(DOMNode* /*oldChild*/)
{
    return 0;
}

DOMNode* DOMNode::appendChild(DOMNode* /*oldChild*/)
{
    return 0;
}

bool DOMNode::hasChildNodes()
{
    return false;
}

DOMNode* DOMNode::cloneNode(bool /*deep*/)
{
    return 0;
}

void DOMNode::normalize()
{
}

bool DOMNode::isSupported(const char* /*feature*/, const char* /*version*/)
{
    return false;
}

const char* DOMNode::namespaceURI()
{
    return "";
}

const char* DOMNode::prefix()
{
    return "";
}

void DOMNode::setPrefix(const char* /*prefix*/)
{
}

const char* DOMNode::localName()
{
    return "";
}

bool DOMNode::hasAttributes()
{
    return false;
}

bool DOMNode::isSameNode(DOMNode* other)
{
    if (!other)
        return false;

    return m_node->isSameNode(other->node()) ? true : false;
}

bool DOMNode::isEqualNode( DOMNode* /*other*/)
{
    return false;
}

const char* DOMNode::textContent()
{
    return strdup(m_node->textContent().utf8().data());
}

void DOMNode::setTextContent(const char* /*text*/)
{
}

// DOMNode - DOMEventTarget --------------------------------------------------

void DOMNode::addEventListener(const char* /*type*/, DOMEventListener* /*listener*/, bool /*useCapture*/)
{
}

void DOMNode::removeEventListener(const char* /*type*/, DOMEventListener* /*listener*/, bool /*useCapture*/)
{
}

bool DOMNode::dispatchEvent(DOMEvent* evt)
{
    if (!m_node || !evt)
        return false;

#if 0   // FIXME - raise dom exceptions
    if (![self _node]->isEventTargetNode())
        WebCore::raiseDOMException(DOM_NOT_SUPPORTED_ERR);
#endif

    //WebCore::ExceptionCode ec = 0;
    //return WebCore::EventTargetNodeCast(m_node)->dispatchEvent(evt->coreEvent(), ec) ? true : false;
    return false;
#if 0   // FIXME - raise dom exceptions
    WebCore::raiseOnDOMError(ec);
#endif
}

// DOMNode - DOMNode ----------------------------------------------------------

DOMNode::DOMNode(WebCore::Node* n)
: m_node(0)
{
    if (n)
        n->ref();

    m_node = n;
}

DOMNode::~DOMNode()
{
    if (m_node)
        m_node->deref();
}

DOMNode* DOMNode::createInstance(WebCore::Node* n)
{
    if (!n)
        return 0;

    DOMNode* domNode = 0;
    WebCore::Node::NodeType nodeType = n->nodeType();

    switch (nodeType) {
    case WebCore::Node::ELEMENT_NODE: 
    {
        domNode = DOMElement::createInstance(static_cast<WebCore::Element*>(n));
    }
    break;
    case WebCore::Node::DOCUMENT_NODE:
    {
        domNode = DOMDocument::createInstance(&n->document());
    }
    break;
    default:
    {
        domNode = new DOMNode(n);
    }
    break;
    }

    return domNode;
}


// DOMNodeList ---------------------------------------------------------------

DOMNode* DOMNodeList::item(unsigned index)
{
    if (!m_nodeList)
        return 0;

    WebCore::Node* itemNode = m_nodeList->item(index);
    if (!itemNode)
        return 0;

    return DOMNode::createInstance(itemNode);
}

unsigned DOMNodeList::length()
{
    if (!m_nodeList)
        return 0;
    return m_nodeList->length();
}

// DOMNodeList - DOMNodeList --------------------------------------------------

DOMNodeList::DOMNodeList(WebCore::NodeList* l)
: m_nodeList(0)
{
    if (l)
        l->ref();

    m_nodeList = l;
}

DOMNodeList::~DOMNodeList()
{
    if (m_nodeList)
        m_nodeList->deref();
}

DOMNodeList* DOMNodeList::createInstance(WebCore::NodeList* l)
{
    if (!l)
        return 0;

    return new DOMNodeList(l);
}

// DOMDocument ----------------------------------------------------------------

/*DOMDocumentType* DOMDocument::doctype()
{
    return 0;
}*/

DOMImplementation* DOMDocument::implementation()
{
    return 0;
}

DOMElement* DOMDocument::documentElement()
{
    return DOMElement::createInstance(m_document->documentElement());
}

DOMElement* DOMDocument::createElement(const char* tagName)
{
    if (!m_document)
        return 0;

    ExceptionCode ec;
    return DOMElement::createInstance(m_document->createElement(tagName, ec).get());
}

/*DOMDocumentFragment* DOMDocument::createDocumentFragment()
{
    return 0;
}*/

// DOMText* DOMDocument::createTextNode(const char* /*data*/)
// {
//     return 0;
// }
// 
// DOMComment* DOMDocument::createComment(const char* /*data*/)
// {
//     return 0;
// }
// 
// DOMCDATASection* DOMDocument::createCDATASection(const char* /*data*/)
// {
//     return 0;
// }
// 
// DOMProcessingInstruction* DOMDocument::createProcessingInstruction(const char* /*target*/, const char* /*data*/)
// {
//     return 0;
// }
// 
// DOMAttr* DOMDocument::createAttribute(const char* /*name*/)
// {
//     return 0;
// }
// 
// DOMEntityReference* DOMDocument::createEntityReference(const char* /*name*/)
// {
//     return 0;
// }

DOMNodeList* DOMDocument::getElementsByTagName(const char* tagName)
{
    if (!m_document)
        return 0;

    return DOMNodeList::createInstance(m_document->getElementsByTagName(tagName).get());
}

DOMNode* DOMDocument::importNode(DOMNode* /*importedNode*/, bool /*deep*/)
{
    return 0;
}

DOMElement* DOMDocument::createElementNS(const char* /*namespaceURI*/, const char* /*qualifiedName*/)
{
    return 0;
}

// DOMAttr* DOMDocument::createAttributeNS(const char* /*namespaceURI*/, const char* /*qualifiedName*/)
// {
//     return 0;
// }

DOMNodeList* DOMDocument::getElementsByTagNameNS(const char* namespaceURI, const char* localName)
{
    if (!m_document)
        return 0;

    return  DOMNodeList::createInstance(m_document->getElementsByTagNameNS(namespaceURI, localName).get());
}

DOMElement* DOMDocument::getElementById(const char* elementId)
{
    if (!m_document)
        return 0;

    return DOMElement::createInstance(m_document->getElementById(String(elementId)));
}

// DOMDocument - DOMViewCSS --------------------------------------------------

DOMCSSStyleDeclaration* DOMDocument::getComputedStyle(DOMElement* elt, const char* pseudoElt)
{
    if (!elt)
        return 0;

    Element* element = elt->element();

    WebCore::DOMWindow* dv = m_document->defaultView();
    
    if (!dv)
        return 0;
    
    return DOMCSSStyleDeclaration::createInstance(dv->getComputedStyle(element, pseudoElt).get());
}

// DOMDocument - DOMDocumentEvent --------------------------------------------

DOMEvent* DOMDocument::createEvent(const char* eventType)
{
    WebCore::ExceptionCode ec = 0;
    return DOMEvent::createInstance(m_document->createEvent(eventType, ec));
}

// DOMDocument - DOMDocument --------------------------------------------------

DOMDocument::DOMDocument(WebCore::Document* d)
: DOMNode(d)
, m_document(d)
{
}

DOMDocument::~DOMDocument()
{
}

DOMDocument* DOMDocument::createInstance(WebCore::Document* d)
{
    if (!d)
        return 0;

    DOMDocument* domDocument = 0;

    if (d->isHTMLDocument()) {
        domDocument = new DOMHTMLDocument(d);
    } else {
        domDocument = new DOMDocument(d);
    }

    return domDocument;
}

// DOMElement - DOMNodeExtensions---------------------------------------------

BalRectangle DOMElement::boundingBox()
{
    if (!m_element)
        return BalRectangle();

    WebCore::RenderObject *renderer = m_element->renderer();
    if (renderer) {
        return renderer->absoluteBoundingBoxRect();
    }

    return BalRectangle();
}

void DOMElement::lineBoxRects(BalRectangle* /*rects*/, int /*cRects*/)
{
}

// DOMElement ----------------------------------------------------------------

const char* DOMElement::tagName()
{
    if (!m_element)
        return "";

    return strdup(m_element->tagName().utf8().data());
}
    
const char* DOMElement::getAttribute(const char* name)
{
    if (!m_element)
        return "";
    return strdup(m_element->getAttribute(name).string().utf8().data());
}
    
void DOMElement::setAttribute(const char* name, const char* value)
{
    if (!m_element)
        return;

    WebCore::ExceptionCode ec = 0;
    m_element->setAttribute(name, value, ec);
}
    
void DOMElement::removeAttribute(const char* /*name*/)
{
}
    
//DOMAttr* DOMElement::getAttributeNode(const char* /*name*/)
//{
//    return 0;
//}
    
// DOMAttr* DOMElement::setAttributeNode(DOMAttr* /*newAttr*/)
// {
//     return 0;
// }
    
// DOMAttr* DOMElement::removeAttributeNode(DOMAttr* /*oldAttr*/)
// {
//     return 0;
// }
    
DOMNodeList* DOMElement::getElementsByTagName(const char* /*name*/)
{
    return 0;
}
    
const char* DOMElement::getAttributeNS(const char* /*namespaceURI*/, const char* /*localName*/)
{
    return "";
}
    
void DOMElement::setAttributeNS(const char* /*namespaceURI*/, const char* /*qualifiedName*/, const char* /*value*/)
{
}
    
void DOMElement::removeAttributeNS(const char* /*namespaceURI*/, const char* /*localName*/)
{
}
    
// DOMAttr* DOMElement::getAttributeNodeNS(const char* /*namespaceURI*/, const char* /*localName*/)
// {
//     return 0;
// }
    
// DOMAttr* DOMElement::setAttributeNodeNS(DOMAttr* /*newAttr*/)
// {
//     return 0;
// }
    
DOMNodeList* DOMElement::getElementsByTagNameNS(const char* /*namespaceURI*/, const char* /*localName*/)
{
    return 0;
}
    
bool DOMElement::hasAttribute(const char* /*name*/)
{
    return false;
}
    
bool DOMElement::hasAttributeNS(const char* /*namespaceURI*/, const char* /*localName*/)
{
    return false;
}

void DOMElement::focus()
{
    if (!m_element)
        return;
    m_element->focus();
}

void DOMElement::blur()
{
    if (!m_element)
        return ;
    m_element->blur();
}

// DOMElementPrivate ---------------------------------------------------------

void *DOMElement::coreElement()
{
    if (!m_element)
        return 0;
    return (void*) m_element;
}

bool DOMElement::isEqual(DOMElement *other)
{
    if (!other)
        return false;
    
    void* otherCoreEle = other->coreElement();
    return (otherCoreEle == (void*)m_element) ? true : false;
}

bool DOMElement::isFocused()
{
    if (!m_element)
        return false;

    if (m_element->document().focusedElement() == m_element)
        return true;
        
    return false;
}

const char* DOMElement::innerText()
{
    if (!m_element) 
        return "";

    return strdup(m_element->innerText().utf8().data());
}

WebFontDescription* DOMElement::font()
{
    ASSERT(m_element);

    WebCore::RenderObject* renderer = m_element->renderer();
    if (!renderer)
        return 0;

    FontDescription fontDescription = renderer->style().fontCascade().fontDescription();
    AtomicString family = fontDescription.firstFamily();
    
    WebFontDescription *webFontDescription = new WebFontDescription();
    webFontDescription->family = strdup(family.string().utf8().data());
    webFontDescription->familyLength = family.length();
    webFontDescription->size = fontDescription.computedSize();
    webFontDescription->weight = fontDescription.weight() >= FontWeight600;
    webFontDescription->italic = fontDescription.italic();

    return webFontDescription;
}

const char * DOMElement::shadowPseudoId()
{
    if (!m_element)
        return "";

    return strdup(m_element->shadowPseudoId().string().utf8().data());
}

// DOMElementCSSInlineStyle --------------------------------------------------

DOMCSSStyleDeclaration* DOMElement::style()
{
    if (!m_element)
        return 0;

    WebCore::CSSStyleDeclaration* style = m_element->style();
    if (!style)
        return 0;

    return DOMCSSStyleDeclaration::createInstance(style);
}

// DOMElementExtensions ------------------------------------------------------

int DOMElement::offsetLeft()
{
    if (!m_element)
        return 0;

    return m_element->offsetLeft();
}

int DOMElement::offsetTop()
{
    if (!m_element)
        return 0;

    return m_element->offsetTop();
}

int DOMElement::offsetWidth()
{
    if (!m_element)
        return 0;

    return m_element->offsetWidth();
}

int DOMElement::offsetHeight()
{
    if (!m_element)
        return 0;

    return m_element->offsetHeight();
}

DOMElement* DOMElement::offsetParent()
{
    return 0;
}

int DOMElement::clientWidth()
{
    if (!m_element)
        return 0;

    return m_element->clientWidth();
}

int DOMElement::clientHeight()
{
    if (!m_element)
        return 0;

    return m_element->clientHeight();
}

int DOMElement::scrollLeft()
{
    if (!m_element)
        return 0;

    return m_element->scrollLeft();
}

void DOMElement::setScrollLeft(int /*newScrollLeft*/)
{
}

int DOMElement::scrollTop()
{
    if (!m_element)
        return 0;

    return m_element->scrollTop();
}

void DOMElement::setScrollTop(int /*newScrollTop*/)
{
}

int DOMElement::scrollWidth()
{
    if (!m_element)
        return 0;

    return m_element->scrollWidth();
}

int DOMElement::scrollHeight()
{
    if (!m_element)
        return 0;

    return m_element->scrollHeight();
}

void DOMElement::scrollIntoView(bool alignWithTop)
{
    if (!m_element)
        return ;

    m_element->scrollIntoView(!!alignWithTop);
}

void DOMElement::scrollIntoViewIfNeeded(bool centerIfNeeded)
{
    if (!m_element)
        return;

    m_element->scrollIntoViewIfNeeded(!!centerIfNeeded);
}

// DOMElement -----------------------------------------------------------------

DOMElement::DOMElement(WebCore::Element* e)
: DOMNode(e)
, m_element(e)
{
}

DOMElement::~DOMElement()
{
}

DOMElement* DOMElement::createInstance(WebCore::Element* e)
{
    if (!e)
        return 0;

    DOMElement* domElement = 0;

    if (is<HTMLFormElement>(e))
        domElement = new DOMHTMLFormElement(e);
    else if (e->hasTagName(selectTag))
        domElement =  new DOMHTMLSelectElement(e);
    else if (is<HTMLOptionElement>(e))
        domElement = new DOMHTMLOptionElement(e);
    else if (is<HTMLInputElement>(e))
        domElement = new DOMHTMLInputElement(e);
    else if (is<HTMLTextAreaElement>(e))
        domElement = new DOMHTMLTextAreaElement(e);
    else if (e->isHTMLElement())
        domElement = DOMHTMLElement::createInstance(e);
    else
        domElement = new DOMElement(e);

    return domElement;
}
