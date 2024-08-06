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

#ifndef DOMCoreClasses_H
#define DOMCoreClasses_H

#include "WebScriptObject.h"
#include "WebKitTypes.h"

namespace WebCore {
    class Element;
    class Document;
    class Node;
    class NodeList;
    class DOMImplementation;
}

struct WebFontDescription {
    const char* family;
    unsigned familyLength;
    float size;
    bool weight;
    bool italic;
};

class DOMNodeList;
class DOMDocument;
class DOMEventListener;
class DOMEvent;
class DOMElement;
class DOMCSSStyleDeclaration;

class WEBKIT_OWB_API DOMObject : public WebScriptObject
{
public:
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return WebScriptObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        WebScriptObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return WebScriptObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        WebScriptObject::setException(description);
    }
};

class WEBKIT_OWB_API DOMNode : public DOMObject
{
protected:
    DOMNode(WebCore::Node* n);

public:
    virtual ~DOMNode();
    static DOMNode* createInstance(WebCore::Node* n);

public:
    
    // IWebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return WebScriptObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        WebScriptObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return WebScriptObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        WebScriptObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName();
    
    virtual const char* nodeValue();
    
    virtual void setNodeValue(const char*);
    
    virtual unsigned short nodeType();
    
    virtual DOMNode* parentNode();
    
    virtual DOMNodeList* childNodes();
    
    virtual DOMNode* firstChild();
    
    virtual DOMNode* lastChild();
    
    virtual DOMNode* previousSibling();
    
    virtual DOMNode* nextSibling();
    
    //virtual DOMNamedNodeMap* attributes();
    
    virtual DOMDocument* ownerDocument();
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild);
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild);
    
    virtual DOMNode* removeChild(DOMNode *oldChild);
    
    virtual DOMNode* appendChild(DOMNode *oldChild);
    
    virtual bool hasChildNodes();
    
    virtual DOMNode* cloneNode(bool deep);
    
    virtual void normalize();
    
    virtual bool isSupported(const char* feature, const char* version);
    
    virtual const char* namespaceURI();
    
    virtual const char* prefix();
    
    virtual void setPrefix(const char*);
    
    virtual const char* localName();
    
    virtual bool hasAttributes();

    virtual bool isSameNode(DOMNode* other);
    
    virtual bool isEqualNode(DOMNode* other);
    
    virtual const char* textContent();
    
    virtual void setTextContent(const char* text);

    // IDOMEventTarget
    virtual void addEventListener(const char* type, DOMEventListener *listener, bool useCapture);
    
    virtual void removeEventListener( const char* type, DOMEventListener *listener, bool useCapture);
    
    virtual bool dispatchEvent(DOMEvent *evt);

    // DOMNode
    WebCore::Node* node() const { return m_node; }

protected:
    WebCore::Node* m_node;
};

class WEBKIT_OWB_API DOMNodeList : public DOMObject
{
protected:
    DOMNodeList(WebCore::NodeList* l);

public:
    virtual ~DOMNodeList();
    static DOMNodeList* createInstance(WebCore::NodeList* l);

public:
    // IWebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return WebScriptObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        WebScriptObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return WebScriptObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        WebScriptObject::setException(description);
    }

    // IDOMNodeList
    virtual DOMNode* item(unsigned index);
    
    virtual unsigned length();

protected:
    WebCore::NodeList* m_nodeList;
};

class WEBKIT_OWB_API DOMDocument : public DOMNode
{
protected:
    DOMDocument(WebCore::Document* d);

public:
    static DOMDocument* createInstance(WebCore::Document* d);
    virtual ~DOMDocument();

    // WebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return WebScriptObject::throwException(exceptionMessage);
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        WebScriptObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return WebScriptObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        WebScriptObject::setException(description);
    }
    
    // DOMNode
    virtual const char* nodeName() { return DOMNode::nodeName(); }
    
    virtual const char* nodeValue() { return DOMNode::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMNode::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMNode::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMNode::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMNode::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMNode::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMNode::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMNode::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMNode::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMNode::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMNode::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMNode::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMNode::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMNode::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMNode::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMNode::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMNode::cloneNode(deep); }
    
    virtual void normalize() { DOMNode::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMNode::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMNode::namespaceURI(); }
    
    virtual const char* prefix() { return DOMNode::prefix(); }
    
    virtual void setPrefix(const char* prefix) { DOMNode::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMNode::localName(); }
    
    virtual bool hasAttributes() { return DOMNode::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMNode::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMNode::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMNode::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMNode::setTextContent(text); }
    
    // DOMDocument
//    virtual DOMDocumentType* doctype();
    
    virtual WebCore::DOMImplementation* implementation();
    
    virtual DOMElement* documentElement();
    
    virtual DOMElement* createElement(const char* tagName);
    
    //virtual DOMDocumentFragment* createDocumentFragment();
    
    /*virtual DOMText* createTextNode(const char* data);
    
    virtual DOMComment* createComment(const char* data);
    
    virtual DOMCDATASection* createCDATASection(const char* data);
    
    virtual DOMProcessingInstruction* createProcessingInstruction(const char* target, const char* data);
    
    virtual DOMAttr* createAttribute(const char* name);
    
    virtual DOMEntityReference* createEntityReference(const char* name);*/
    
    virtual DOMNodeList* getElementsByTagName(const char* tagName);
    
    virtual DOMNode* importNode(DOMNode *importedNode, bool deep);
    
    virtual DOMElement* createElementNS(const char* namespaceURI, const char* qualifiedName);
    
    //virtual DOMAttr* createAttributeNS(const char* namespaceURI, const char* qualifiedName);
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName);
    
    virtual DOMElement* getElementById(const char* elementId);

    // DOMViewCSS
    virtual DOMCSSStyleDeclaration* getComputedStyle(DOMElement *elt, const char* pseudoElt);

    // DOMDocumentEvent
    virtual DOMEvent* createEvent(const char* eventType);

    // DOMDocument
    WebCore::Document* document() { return m_document; }

protected:
    WebCore::Document* m_document;
};

class WEBKIT_OWB_API DOMElement : public DOMNode
{
protected:
    DOMElement(WebCore::Element* e);
    ~DOMElement();

public:
    static DOMElement* createInstance(WebCore::Element* e);

    // IWebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return WebScriptObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        WebScriptObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return WebScriptObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        WebScriptObject::setException(description);
    }
    
    // DOMNode
    virtual const char* nodeName() { return DOMNode::nodeName(); }
    
    virtual const char* nodeValue() { return DOMNode::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMNode::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMNode::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMNode::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMNode::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMNode::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMNode::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMNode::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMNode::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMNode::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMNode::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMNode::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMNode::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMNode::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMNode::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMNode::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMNode::cloneNode(deep); }
    
    virtual void normalize() { DOMNode::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMNode::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMNode::namespaceURI(); }
    
    virtual const char* prefix() { return DOMNode::prefix(); }
    
    virtual void setPrefix(const char* prefix) { DOMNode::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMNode::localName(); }
    
    virtual bool hasAttributes() { return DOMNode::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMNode::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMNode::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMNode::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMNode::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName();
    
    virtual const char* getAttribute(const char* name);
    
    virtual void setAttribute(const char* name, const char* value);
    
    virtual void removeAttribute(const char* name);
    
    //virtual DOMAttr* getAttributeNode(const char* name);
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr);
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr);
    
    virtual DOMNodeList* getElementsByTagName(const char* name);
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName);
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value);
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName);
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, const char* localName);
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr);
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName);
    
    virtual bool hasAttribute(const char* name);
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName);

    virtual void focus();
    
    virtual void blur();

    // DOMNodeExtensions
    virtual BalRectangle boundingBox();
    
    virtual void lineBoxRects(BalRectangle *rects, int cRects);

    // DOMElementPrivate
    virtual void* coreElement();

    virtual bool isEqual( DOMElement *other);

    virtual bool isFocused();

    virtual const char* innerText();

    virtual WebFontDescription* font();

    virtual const char*  shadowPseudoId();

    // DOMElementCSSInlineStyle
    virtual DOMCSSStyleDeclaration* style();

    // DOMElementExtensions
    virtual int offsetLeft();
    
    virtual int offsetTop();
    
    virtual int offsetWidth();
    
    virtual int offsetHeight();
    
    virtual DOMElement* offsetParent();
    
    virtual int clientWidth();
    
    virtual int clientHeight();
    
    virtual int scrollLeft();
    
    virtual void setScrollLeft(int newScrollLeft);
    
    virtual int scrollTop();
    
    virtual void setScrollTop(int newScrollTop);
    
    virtual int scrollWidth();
    
    virtual int scrollHeight();
    
    virtual void scrollIntoView(bool alignWithTop);
    
    virtual void scrollIntoViewIfNeeded(bool centerIfNeeded);

    // DOMElement
    WebCore::Element* element() { return m_element; }

protected:
    WebCore::Element* m_element;
};

#endif
