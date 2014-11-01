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

#ifndef DOMHTMLClasses_H
#define DOMHTMLClasses_H

#include "DOMCoreClasses.h"
#include "WebScriptObject.h"

namespace WebCore {
    class HTMLCollection;
    class HTMLElement;
}

class DOMHTMLElement;

#undef accept

class DOMHTMLCollection : public DOMObject
{
protected:
    DOMHTMLCollection(WebCore::HTMLCollection* c);

public:
    static DOMHTMLCollection* createInstance(WebCore::HTMLCollection*);

    // WebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMHTMLCollection
    virtual unsigned length();
    
    virtual DOMNode* item(unsigned index);
    
    virtual DOMNode* namedItem(const char* name);

protected:
    WebCore::HTMLCollection* m_collection;
};

class DOMHTMLOptionsCollection : public DOMObject
{
    // IWebScriptObject
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMHTMLOptionsCollection
    virtual unsigned int length();
    
    virtual void setLength(unsigned int);
    
    virtual DOMNode* item(unsigned int index);
    
    virtual DOMNode* namedItem(const char* name);
};

class DOMHTMLDocument : public DOMDocument
{
protected:
    DOMHTMLDocument();
public:
    DOMHTMLDocument(WebCore::Document* d) : DOMDocument(d) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMDocument::nodeName(); }
    
    virtual const char* nodeValue() { return DOMDocument::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMDocument::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMDocument::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMDocument::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMDocument::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMDocument::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMDocument::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMDocument::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMDocument::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMDocument::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMDocument::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMDocument::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMDocument::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMDocument::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMDocument::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMDocument::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMDocument::cloneNode(deep); }
    
    virtual void normalize() { DOMDocument::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMDocument::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMDocument::namespaceURI(); }
    
    virtual const char* prefix() { return DOMDocument::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMDocument::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMDocument::localName(); }
    
    virtual bool hasAttributes() { return DOMDocument::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMDocument::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMDocument::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMDocument::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMDocument::setTextContent(text); }
    
    // IDOMDocument
    //virtual DOMDocumentType* doctype() { return DOMDocument::doctype(); }
    
    //virtual DOMImplementation* implementation() { return DOMDocument::implementation(); }
    
    virtual DOMElement* documentElement() { return DOMDocument::documentElement(); }
    
    virtual DOMElement* createElement(const char* tagName) { return DOMDocument::createElement(tagName); }
    
    //virtual DOMDocumentFragment* createDocumentFragment() { return DOMDocument::createDocumentFragment(); }
    
    //virtual DOMText* createTextNode(const char* data) { return DOMDocument::createTextNode(data); }
    
    //virtual DOMComment* createComment(const char* data) { return DOMDocument::createComment(data); }
    
    //virtual DOMCDATASection* createCDATASection(const char* data) { return DOMDocument::createCDATASection(data); }
    
    //virtual DOMProcessingInstruction* createProcessingInstruction(const char* target, const char* data) { return DOMDocument::createProcessingInstruction(target, data); }
    
    //virtual DOMAttr* createAttribute(const char* name) { return DOMDocument::createAttribute(name); }
    
    //virtual DOMEntityReference* createEntityReference(const char* name) { return DOMDocument::createEntityReference(name); }
    
    virtual DOMNodeList* getElementsByTagName(const char* tagName) { return DOMDocument::getElementsByTagName(tagName); }
    
    virtual DOMNode* importNode(DOMNode *importedNode, bool deep) { return DOMDocument::importNode(importedNode, deep); }
    
    virtual DOMElement* createElementNS(const char* namespaceURI, const char* qualifiedName) { return DOMDocument::createElementNS(namespaceURI, qualifiedName); }
    
    //virtual DOMAttr* createAttributeNS(const char* namespaceURI, const char* qualifiedName) { return DOMDocument::createAttributeNS(namespaceURI, qualifiedName); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMDocument::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual DOMElement* getElementById(const char* elementId) { return DOMDocument::getElementById(elementId); }

    // IDOMHTMLDocument
    virtual const char* title();
    
    virtual void setTitle(const char*);
    
    virtual const char* referrer();
    
    virtual const char* domain();
    
    virtual const char* URL();
    
    virtual DOMHTMLElement* body();
    
    virtual void setBody(DOMHTMLElement *body);
    
    virtual DOMHTMLCollection* images();
    
    virtual DOMHTMLCollection* applets();
    
    virtual DOMHTMLCollection* links();
    
    virtual DOMHTMLCollection* forms();
    
    virtual DOMHTMLCollection* anchors();
    
    virtual const char* cookie();
    
    virtual void setCookie(const char* cookie);
    
    virtual void open();
    
    virtual void close();
    
    virtual void write(const char* text);
    
    virtual void writeln(const char* text);
    
    virtual DOMElement* getElementById_(const char* elementId);
    
    virtual DOMNodeList* getElementsByName(const char* elementName);
};

class DOMHTMLElement : public DOMElement
{
protected:
    DOMHTMLElement();
    DOMHTMLElement(WebCore::Element* e) : DOMElement(e) {}
public:
    static DOMHTMLElement* createInstance(WebCore::Element*);

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMElement::nextSibling(); }
    
//    virtual DOMNamedNodeMap* attributes() { return DOMElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMElement::cloneNode(deep); }
    
    virtual void normalize() { DOMElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMElement::localName(); }
    
    virtual bool hasAttributes() { return DOMElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMElement::focus(); }
    
    virtual void blur() { DOMElement::blur(); }

    // IDOMHTMLElement
    virtual const char* idName();
    
    virtual void setIdName(const char* idName);
    
    virtual const char* title();
    
    virtual void setTitle(const char* title);
    
    virtual const char* lang();
    
    virtual void setLang(const char* lang);
    
    virtual const char* dir();
    
    virtual void setDir(const char* dir);
    
    virtual const char* className();
    
    virtual void setClassName(const char* className);

    virtual const char* innerHTML();
        
    virtual void setInnerHTML(const char* html);
        
    virtual const char* innerText();
        
    virtual void setInnerText(const char* text);        

};

class DOMHTMLFormElement : public DOMHTMLElement
{
protected:
    DOMHTMLFormElement();
public:
    DOMHTMLFormElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMHTMLElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMHTMLElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMHTMLElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMHTMLElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMHTMLElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMHTMLElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMHTMLElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMHTMLElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMHTMLElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMHTMLElement::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMHTMLElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMHTMLElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMHTMLElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMHTMLElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMHTMLElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMHTMLElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMHTMLElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMHTMLElement::cloneNode(deep); }
    
    virtual void normalize() { DOMHTMLElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMHTMLElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMHTMLElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMHTMLElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMHTMLElement::localName(); }
    
    virtual bool hasAttributes() { return DOMHTMLElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMHTMLElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMHTMLElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMHTMLElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMHTMLElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMHTMLElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMHTMLElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMHTMLElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMHTMLElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMHTMLElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMHTMLElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMHTMLElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMHTMLElement::focus(); }
    
    virtual void blur() { DOMHTMLElement::blur(); }


    // IDOMHTMLElement
    virtual const char* idName() { return DOMHTMLElement::idName(); }
    
    virtual void setIdName(const char* idName) { DOMHTMLElement::setIdName(idName); }
    
    virtual const char* title() { return DOMHTMLElement::title(); }
    
    virtual void setTitle(const char* title) { DOMHTMLElement::setTitle(title); }
    
    virtual const char* lang() { return DOMHTMLElement::lang(); }
    
    virtual void setLang(const char* lang) { DOMHTMLElement::setLang(lang); }
    
    virtual const char* dir() { return DOMHTMLElement::dir(); }
    
    virtual void setDir(const char* dir) { DOMHTMLElement::setDir(dir); }
    
    virtual const char* className() { return DOMHTMLElement::className(); }
    
    virtual void setClassName(const char* className) { DOMHTMLElement::setClassName(className); }

    virtual const char* innerHTML() { return DOMHTMLElement::innerHTML(); }
        
    virtual void setInnerHTML(const char* html) { DOMHTMLElement::setInnerHTML(html); }
        
    virtual const char* innerText() { return DOMHTMLElement::innerText(); }
        
    virtual void setInnerText(const char* text) { DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLFormElement
    virtual DOMHTMLCollection* elements();
    
    virtual int length();
    
    virtual const char* name();
    
    virtual void setName(const char* name);
    
    virtual const char* acceptCharset();
    
    virtual void setAcceptCharset(const char* acceptCharset);
    
    virtual const char* action();
    
    virtual void setAction(const char* action);
    
    virtual const char* encType();
    
    virtual const char* setEnctype();
    
    virtual const char* method();
    
    virtual void setMethod(const char* method);
    
    virtual const char* target();
    
    virtual void setTarget(const char* target);
    
    virtual void submit();
    
    virtual void reset();
};

class DOMHTMLSelectElement : public DOMHTMLElement
{
protected:
    DOMHTMLSelectElement();
public:
    DOMHTMLSelectElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMHTMLElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMHTMLElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMHTMLElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMHTMLElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMHTMLElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMHTMLElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMHTMLElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMHTMLElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMHTMLElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMHTMLElement::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMHTMLElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMHTMLElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMHTMLElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMHTMLElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMHTMLElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMHTMLElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMHTMLElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMHTMLElement::cloneNode(deep); }
    
    virtual void normalize() { DOMHTMLElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMHTMLElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMHTMLElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMHTMLElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMHTMLElement::localName(); }
    
    virtual bool hasAttributes() { return DOMHTMLElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMHTMLElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMHTMLElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMHTMLElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMHTMLElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMHTMLElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMHTMLElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMHTMLElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMHTMLElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMHTMLElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMHTMLElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMHTMLElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMHTMLElement::focus(); }
    
    virtual void blur() { DOMHTMLElement::blur(); }


    // IDOMHTMLElement
    virtual const char* idName() { return DOMHTMLElement::idName(); }
    
    virtual void setIdName(const char* idName) { DOMHTMLElement::setIdName(idName); }
    
    virtual const char* title() { return DOMHTMLElement::title(); }
    
    virtual void setTitle(const char* title) { DOMHTMLElement::setTitle(title); }
    
    virtual const char* lang() { return DOMHTMLElement::lang(); }
    
    virtual void setLang(const char* lang) { DOMHTMLElement::setLang(lang); }
    
    virtual const char* dir() { return DOMHTMLElement::dir(); }
    
    virtual void setDir(const char* dir) { DOMHTMLElement::setDir(dir); }
    
    virtual const char* className() { return DOMHTMLElement::className(); }
    
    virtual void setClassName(const char* className) { DOMHTMLElement::setClassName(className); }

    virtual const char* innerHTML() { return DOMHTMLElement::innerHTML(); }
        
    virtual void setInnerHTML(const char* html) { DOMHTMLElement::setInnerHTML(html); }
        
    virtual const char* innerText() { return DOMHTMLElement::innerText(); }
        
    virtual void setInnerText(const char* text) { DOMHTMLElement::setInnerText(text); }
    
    // DOMHTMLSelectElement
    virtual const char* type();
    
    virtual int selectedIndex();
    
    virtual void setSelectedIndx(int selectedIndex);
    
    virtual const char* value();
    
    virtual void setValue(const char* value);
    
    virtual int length();
    
    virtual DOMHTMLFormElement* form();
    
    virtual DOMHTMLOptionsCollection* options();
    
    virtual bool disabled();
    
    virtual void setDisabled(bool);
    
    virtual bool multiple();
    
    virtual void setMultiple(bool multiple);
    
    virtual const char* name();
    
    virtual void setName(const char* name);
    
    virtual int size();
    
    virtual void setSize(int size);
    
    virtual int tabIndex();
    
    virtual void setTabIndex(int tabIndex);
    
    virtual void add(DOMHTMLElement *element, DOMHTMLElement *before);
    
    virtual void remove(int index);
    
    // IFormsAutoFillTransitionSelect
    virtual void activateItemAtIndex(int index);
};

class DOMHTMLOptionElement : public DOMHTMLElement
{
protected:
    DOMHTMLOptionElement();
public:
    DOMHTMLOptionElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMHTMLElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMHTMLElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMHTMLElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMHTMLElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMHTMLElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMHTMLElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMHTMLElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMHTMLElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMHTMLElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMHTMLElement::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMHTMLElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMHTMLElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMHTMLElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMHTMLElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMHTMLElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMHTMLElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMHTMLElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMHTMLElement::cloneNode(deep); }
    
    virtual void normalize() { DOMHTMLElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMHTMLElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMHTMLElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMHTMLElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMHTMLElement::localName(); }
    
    virtual bool hasAttributes() { return DOMHTMLElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMHTMLElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMHTMLElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMHTMLElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMHTMLElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMHTMLElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMHTMLElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMHTMLElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMHTMLElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMHTMLElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMHTMLElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMHTMLElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMHTMLElement::focus(); }
    
    virtual void blur() { DOMHTMLElement::blur(); }


    // IDOMHTMLElement
    virtual const char* idName() { return DOMHTMLElement::idName(); }
    
    virtual void setIdName(const char* idName) { DOMHTMLElement::setIdName(idName); }
    
    virtual const char* title() { return DOMHTMLElement::title(); }
    
    virtual void setTitle(const char* title) { DOMHTMLElement::setTitle(title); }
    
    virtual const char* lang() { return DOMHTMLElement::lang(); }
    
    virtual void setLang(const char* lang) { DOMHTMLElement::setLang(lang); }
    
    virtual const char* dir() { return DOMHTMLElement::dir(); }
    
    virtual void setDir(const char* dir) { DOMHTMLElement::setDir(dir); }
    
    virtual const char* className() { return DOMHTMLElement::className(); }
    
    virtual void setClassName(const char* className) { DOMHTMLElement::setClassName(className); }

    virtual const char* innerHTML() { return DOMHTMLElement::innerHTML(); }
        
    virtual void setInnerHTML(const char* html) { DOMHTMLElement::setInnerHTML(html); }
        
    virtual const char* innerText() { return DOMHTMLElement::innerText(); }
        
    virtual void setInnerText(const char* text) { DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLOptionElement
    virtual DOMHTMLFormElement* form();
    
    virtual bool defaultSelected();
    
    virtual void setDefaultSelected(bool defaultSelected);
    
    virtual const char* text();
    
    virtual int index();
    
    virtual bool disabled();
    
    virtual void setDisabled(bool disabled);
    
    virtual const char* label();
    
    virtual void setLabel(const char* label);
    
    virtual bool selected();
    
    virtual void setSelected(bool selected);
    
    virtual const char* value();
    
    virtual void setValue(const char* value);
};

class DOMHTMLInputElement : public DOMHTMLElement
{
protected:
    DOMHTMLInputElement();
public:
    DOMHTMLInputElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }
    // IDOMNode
    virtual const char* nodeName() { return DOMHTMLElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMHTMLElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMHTMLElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMHTMLElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMHTMLElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMHTMLElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMHTMLElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMHTMLElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMHTMLElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMHTMLElement::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMHTMLElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMHTMLElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMHTMLElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMHTMLElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMHTMLElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMHTMLElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMHTMLElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMHTMLElement::cloneNode(deep); }
    
    virtual void normalize() { DOMHTMLElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMHTMLElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMHTMLElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMHTMLElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMHTMLElement::localName(); }
    
    virtual bool hasAttributes() { return DOMHTMLElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMHTMLElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMHTMLElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMHTMLElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMHTMLElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMHTMLElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMHTMLElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMHTMLElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMHTMLElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMHTMLElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMHTMLElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMHTMLElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMHTMLElement::focus(); }
    
    virtual void blur() { DOMHTMLElement::blur(); }


    // IDOMHTMLElement
    virtual const char* idName() { return DOMHTMLElement::idName(); }
    
    virtual void setIdName(const char* idName) { DOMHTMLElement::setIdName(idName); }
    
    virtual const char* title() { return DOMHTMLElement::title(); }
    
    virtual void setTitle(const char* title) { DOMHTMLElement::setTitle(title); }
    
    virtual const char* lang() { return DOMHTMLElement::lang(); }
    
    virtual void setLang(const char* lang) { DOMHTMLElement::setLang(lang); }
    
    virtual const char* dir() { return DOMHTMLElement::dir(); }
    
    virtual void setDir(const char* dir) { DOMHTMLElement::setDir(dir); }
    
    virtual const char* className() { return DOMHTMLElement::className(); }
    
    virtual void setClassName(const char* className) { DOMHTMLElement::setClassName(className); }

    virtual const char* innerHTML() { return DOMHTMLElement::innerHTML(); }
        
    virtual void setInnerHTML(const char* html) { DOMHTMLElement::setInnerHTML(html); }
        
    virtual const char* innerText() { return DOMHTMLElement::innerText(); }
        
    virtual void setInnerText(const char* text) { DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLInputElement
    virtual const char* defaultValue();
    
    virtual void setDefaultValue(const char* val);
    
    virtual bool defaultChecked();
    
    virtual void setDefaultChecked(const char* checked);
    
    virtual DOMHTMLElement* form();
    
    virtual const char* accept();
    
    virtual void setAccept(const char* accept);
    
    virtual const char* accessKey();
    
    virtual void setAccessKey(const char* key);
    
    virtual const char* align();
    
    virtual void setAlign(const char* align);
    
    virtual const char* alt();
    
    virtual void setAlt(const char* alt);
    
    virtual bool checked();
    
    virtual void setChecked(bool checked);
    
    virtual bool disabled();
    
    virtual void setDisabled(bool disabled);
    
    virtual int maxLength();
    
    virtual void setMaxLength(int maxLength);
    
    virtual const char* name();
    
    virtual void setName(const char* name);
    
    virtual bool readOnly();
    
    virtual void setReadOnly(bool readOnly);
    
    virtual unsigned int size();
    
    virtual void setSize(unsigned int size);
    
    virtual const char* src();
    
    virtual void setSrc(const char* src);
    
    virtual int tabIndex();
    
    virtual void setTabIndex(int tabIndex);
    
    virtual const char* type();
    
    virtual void setType(const char* type);
    
    virtual const char* useMap();
    
    virtual void setUseMap(const char* useMap);
    
    virtual const char* value();
    
    virtual void setValue(const char* value);
        
    virtual void select();
    
    virtual void click();

    virtual void setSelectionStart(long start);
    
    virtual long selectionStart();
    
    virtual void setSelectionEnd(long end);
    
    virtual long selectionEnd();

    // FormsAutoFillTransition
    virtual bool isTextField();
    
    virtual BalRectangle rectOnScreen();
    
    virtual void replaceCharactersInRange(int startTarget, int endTarget, const char* replacementString, int index);
    
    virtual void selectedRange(int *start, int *end);
    
    virtual void setAutofilled(bool filled);
    
    virtual bool isAutofilled();

    // FormPromptAdditions
    virtual bool isUserEdited();
};

class DOMHTMLTextAreaElement : public DOMHTMLElement
{
protected:
    DOMHTMLTextAreaElement();
public:
    DOMHTMLTextAreaElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
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
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMNode
    virtual const char* nodeName() { return DOMHTMLElement::nodeName(); }
    
    virtual const char* nodeValue() { return DOMHTMLElement::nodeValue(); }
    
    virtual void setNodeValue(const char* value) { DOMHTMLElement::setNodeValue(value); }
    
    virtual unsigned short nodeType() { return DOMHTMLElement::nodeType(); }
    
    virtual DOMNode* parentNode() { return DOMHTMLElement::parentNode(); }
    
    virtual DOMNodeList* childNodes() { return DOMHTMLElement::childNodes(); }
    
    virtual DOMNode* firstChild() { return DOMHTMLElement::firstChild(); }
    
    virtual DOMNode* lastChild() { return DOMHTMLElement::lastChild(); }
    
    virtual DOMNode* previousSibling() { return DOMHTMLElement::previousSibling(); }
    
    virtual DOMNode* nextSibling() { return DOMHTMLElement::nextSibling(); }
    
    //virtual DOMNamedNodeMap* attributes() { return DOMHTMLElement::attributes(); }
    
    virtual DOMDocument* ownerDocument() { return DOMHTMLElement::ownerDocument(); }
    
    virtual DOMNode* insertBefore(DOMNode *newChild, DOMNode *refChild) { return DOMHTMLElement::insertBefore(newChild, refChild); }
    
    virtual DOMNode* replaceChild(DOMNode *newChild, DOMNode *oldChild) { return DOMHTMLElement::replaceChild(newChild, oldChild); }
    
    virtual DOMNode* removeChild(DOMNode *oldChild) { return DOMHTMLElement::removeChild(oldChild); }
    
    virtual DOMNode* appendChild(DOMNode *oldChild) { return DOMHTMLElement::appendChild(oldChild); }
    
    virtual bool hasChildNodes() { return DOMHTMLElement::hasChildNodes(); }
    
    virtual DOMNode* cloneNode(bool deep) { return DOMHTMLElement::cloneNode(deep); }
    
    virtual void normalize() { DOMHTMLElement::normalize(); }
    
    virtual bool isSupported(const char* feature, const char* version) { return DOMHTMLElement::isSupported(feature, version); }
    
    virtual const char* namespaceURI() { return DOMHTMLElement::namespaceURI(); }
    
    virtual const char* prefix() { return DOMHTMLElement::prefix(); }
    
    virtual void setPrefix(const char* prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual const char* localName() { return DOMHTMLElement::localName(); }
    
    virtual bool hasAttributes() { return DOMHTMLElement::hasAttributes(); }

    virtual bool isSameNode(DOMNode* other) { return DOMHTMLElement::isSameNode(other); }
    
    virtual bool isEqualNode(DOMNode* other) { return DOMHTMLElement::isEqualNode(other); }
    
    virtual const char* textContent() { return DOMHTMLElement::textContent(); }
    
    virtual void setTextContent(const char* text) { DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual const char* tagName() { return DOMHTMLElement::tagName(); }
    
    virtual const char* getAttribute(const char* name) { return DOMHTMLElement::getAttribute(name); }
    
    virtual void setAttribute(const char* name, const char* value) { DOMHTMLElement::setAttribute(name, value); }
    
    virtual void removeAttribute(const char* name) { DOMHTMLElement::removeAttribute(name); }
    
    //virtual DOMAttr* getAttributeNode(const char* name) { return DOMHTMLElement::getAttributeNode(name); }
    
    //virtual DOMAttr* setAttributeNode(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNode(newAttr); }
    
    //virtual DOMAttr* removeAttributeNode(DOMAttr *oldAttr) { return DOMHTMLElement::removeAttributeNode(oldAttr); }
    
    virtual DOMNodeList* getElementsByTagName(const char* name) { return DOMHTMLElement::getElementsByTagName(name); }
    
    virtual const char* getAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName); }
    
    virtual void setAttributeNS(const char* namespaceURI, const char* qualifiedName, const char* value) { DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual void removeAttributeNS(const char* namespaceURI, const char* localName) { DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* getAttributeNodeNS(const char* namespaceURI, WebCore:String localName) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName); }
    
    //virtual DOMAttr* setAttributeNodeNS(DOMAttr *newAttr) { return DOMHTMLElement::setAttributeNodeNS(newAttr); }
    
    virtual DOMNodeList* getElementsByTagNameNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName); }
    
    virtual bool hasAttribute(const char* name) { return DOMHTMLElement::hasAttribute(name); }
    
    virtual bool hasAttributeNS(const char* namespaceURI, const char* localName) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName); }

    virtual void focus() { DOMHTMLElement::focus(); }
    
    virtual void blur() { DOMHTMLElement::blur(); }


    // IDOMHTMLElement
    virtual const char* idName() { return DOMHTMLElement::idName(); }
    
    virtual void setIdName(const char* idName) { DOMHTMLElement::setIdName(idName); }
    
    virtual const char* title() { return DOMHTMLElement::title(); }
    
    virtual void setTitle(const char* title) { DOMHTMLElement::setTitle(title); }
    
    virtual const char* lang() { return DOMHTMLElement::lang(); }
    
    virtual void setLang(const char* lang) { DOMHTMLElement::setLang(lang); }
    
    virtual const char* dir() { return DOMHTMLElement::dir(); }
    
    virtual void setDir(const char* dir) { DOMHTMLElement::setDir(dir); }
    
    virtual const char* className() { return DOMHTMLElement::className(); }
    
    virtual void setClassName(const char* className) { DOMHTMLElement::setClassName(className); }

    virtual const char* innerHTML() { return DOMHTMLElement::innerHTML(); }
        
    virtual void setInnerHTML(const char* html) { DOMHTMLElement::setInnerHTML(html); }
        
    virtual const char* innerText() { return DOMHTMLElement::innerText(); }
        
    virtual void setInnerText(const char* text) { DOMHTMLElement::setInnerText(text); }
    
    // IDOMHTMLTextArea
    virtual const char* defaultValue();
    
    virtual void setDefaultValue(const char* val);
    
    virtual DOMHTMLElement* form();
    
    virtual const char* accessKey();
    
    virtual void setAccessKey(const char* key);
    
    virtual int cols();
    
    virtual void setCols(int cols);
    
    virtual bool disabled();
    
    virtual void setDisabled(bool disabled);
    
    virtual const char* name();
    
    virtual void setName(const char* name);
    
    virtual bool readOnly();
    
    virtual void setReadOnly(bool readOnly);
    
    virtual int rows();
    
    virtual void setRows(int rows);
    
    virtual int tabIndex();
    
    virtual void setTabIndex(int tabIndex);
    
    virtual const char* type();
    
    virtual const char* value();
    
    virtual void setValue(const char* value);
        
    virtual void select();

    // FormPromptAdditions
    virtual bool isUserEdited();
};

#endif
