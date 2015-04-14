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
#include "DOMHTMLClasses.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <Document.h>
#include <Element.h>
#include <FrameView.h>
#include <HTMLCollection.h>
#include <HTMLDocument.h>
#include <HTMLElement.h>
#include <HTMLBodyElement.h>
#include <HTMLFormElement.h>
#include <HTMLInputElement.h>
#include <HTMLNames.h>
#include <HTMLOptionElement.h>
#include <HTMLSelectElement.h>
#include <HTMLTextAreaElement.h>
#include <IntRect.h>
#include <RenderObject.h>
#include <RenderTextControl.h>

using namespace WebCore;
using namespace HTMLNames;

// DOMHTMLCollection

DOMHTMLCollection::DOMHTMLCollection(WebCore::HTMLCollection* c)
: m_collection(c)
{
}

DOMHTMLCollection* DOMHTMLCollection::createInstance(WebCore::HTMLCollection* c)
{
    if (!c)
        return 0;

    return new DOMHTMLCollection(c);
}

// DOMHTMLCollection ----------------------------------------------------------

unsigned DOMHTMLCollection::length()
{
    if (!m_collection)
        return 0;

    return m_collection->length();
}

DOMNode* DOMHTMLCollection::item(unsigned index)
{
    if (!m_collection)
        return 0;

    return DOMNode::createInstance(m_collection->item(index));
}

DOMNode* DOMHTMLCollection::namedItem(const char* /*name*/)
{
    return 0;
}

// DOMHTMLOptionsCollection ---------------------------------------------------

unsigned int DOMHTMLOptionsCollection::length()
{
    return 0;
}
 
void DOMHTMLOptionsCollection::setLength(unsigned int /*length*/)
{
}

DOMNode* DOMHTMLOptionsCollection::item(unsigned int /*index*/)
{
    return 0;
}

DOMNode* DOMHTMLOptionsCollection::namedItem(const char* /*name*/)
{
    return 0;
}

// DOMHTMLDocument ------------------------------------------------------------

const char* DOMHTMLDocument::title()
{
    return "";
}
    
void DOMHTMLDocument::setTitle(const char* /*title*/)
{
}
    
const char* DOMHTMLDocument::referrer()
{
    return "";
}
    
const char* DOMHTMLDocument::domain()
{
    return "";
}
    
const char* DOMHTMLDocument::URL()
{
    return strdup(static_cast<HTMLDocument*>(m_document)->url().string().utf8().data());
}
    
DOMHTMLElement* DOMHTMLDocument::body()
{
    if (!m_document || !m_document->isHTMLDocument())
        return 0;

    HTMLDocument* htmlDoc = static_cast<HTMLDocument*>(m_document);
    return DOMHTMLElement::createInstance(htmlDoc->body());
}
    
void DOMHTMLDocument::setBody(DOMHTMLElement* /*body*/)
{
}
    
DOMHTMLCollection* DOMHTMLDocument::images()
{
    return 0;
}
    
DOMHTMLCollection* DOMHTMLDocument::applets()
{
    return 0;
}
    
DOMHTMLCollection* DOMHTMLDocument::links()
{
    return 0;
}
    
DOMHTMLCollection* DOMHTMLDocument::forms()
{
    if (!m_document || !m_document->isHTMLDocument())
        return 0;

    HTMLDocument* htmlDoc = static_cast<HTMLDocument*>(m_document);
    RefPtr<HTMLCollection> forms = htmlDoc->forms();
    return DOMHTMLCollection::createInstance(forms.get());
}
    
DOMHTMLCollection* DOMHTMLDocument::anchors()
{
    return 0;
}
    
const char* DOMHTMLDocument::cookie()
{
    if (!m_document)
        return "";
    WebCore::ExceptionCode ex = 0;
    return strdup(m_document->cookie(ex).utf8().data());
}
    
void DOMHTMLDocument::setCookie(const char* cookie)
{
    if (!m_document)
        return;
    WebCore::ExceptionCode ex = 0;
    m_document->setCookie(cookie, ex);
}
    
void DOMHTMLDocument::open()
{
}
    
void DOMHTMLDocument::close()
{
}
    
void DOMHTMLDocument::write(const char* /*text*/)
{
}
    
void DOMHTMLDocument::writeln(const char* /*text*/)
{
}
    
DOMElement* DOMHTMLDocument::getElementById_(const char* /*elementId*/)
{
    return 0;
}
    
DOMNodeList* DOMHTMLDocument::getElementsByName(const char* /*elementName*/)
{
    return 0;
}

// DOMHTMLElement -------------------------------------------------------------
//
DOMHTMLElement* DOMHTMLElement::createInstance(WebCore::Element* e)
{
    if (!e)
        return 0;

    return new DOMHTMLElement(e);
}

const char* DOMHTMLElement::idName()
{
    return "";
}
    
void DOMHTMLElement::setIdName(const char* /*idName*/)
{
}
    
const char* DOMHTMLElement::title()
{
    return "";
}
    
void DOMHTMLElement::setTitle(const char* /*title*/)
{
}
    
const char* DOMHTMLElement::lang()
{
    return "";
}
    
void DOMHTMLElement::setLang(const char* /*lang*/)
{
}
    
const char* DOMHTMLElement::dir()
{
    return "";
}
    
void DOMHTMLElement::setDir(const char* /*dir*/)
{
}
    
const char* DOMHTMLElement::className()
{
    return strdup(static_cast<HTMLElement*>(element())->getAttribute(HTMLNames::classAttr).string().utf8().data());
}
    
void DOMHTMLElement::setClassName(const char* /*className*/)
{
}

const char* DOMHTMLElement::innerHTML()
{
    return "";
}
        
void DOMHTMLElement::setInnerHTML(const char* /*html*/)
{
}
        
const char* DOMHTMLElement::innerText()
{
    ASSERT(m_element && m_element->isHTMLElement());
    return strdup(static_cast<HTMLElement*>(m_element)->innerText().utf8().data());
}
        
void DOMHTMLElement::setInnerText(const char* text)
{
    ASSERT(m_element && m_element->isHTMLElement());
    HTMLElement* htmlEle = static_cast<HTMLElement*>(m_element);
    WebCore::ExceptionCode ec = 0;
    htmlEle->setInnerText(text, ec);
}

// DOMHTMLFormElement ---------------------------------------------------------

DOMHTMLCollection* DOMHTMLFormElement::elements()
{
    return 0;
}
    
int DOMHTMLFormElement::length()
{
    return 0;
}
    
const char* DOMHTMLFormElement::name()
{
    return "";
}
    
void DOMHTMLFormElement::setName(const char* /*name*/)
{
}
    
const char* DOMHTMLFormElement::acceptCharset()
{
    return "";
}
    
void DOMHTMLFormElement::setAcceptCharset(const char* /*acceptCharset*/)
{
}
    
const char* DOMHTMLFormElement::action()
{
    ASSERT(m_element && m_element->hasTagName(formTag));
    return strdup(static_cast<HTMLFormElement*>(m_element)->action().utf8().data());
}
    
void DOMHTMLFormElement::setAction(const char* /*action*/)
{
}
    
const char* DOMHTMLFormElement::encType()
{
    return "";
}
    
const char* DOMHTMLFormElement::setEnctype()
{
    return "";
}
    
const char* DOMHTMLFormElement::method()
{
    ASSERT(m_element && m_element->hasTagName(formTag));
    return strdup(static_cast<HTMLFormElement*>(m_element)->method().utf8().data());
}
    
void DOMHTMLFormElement::setMethod(const char* /*method*/)
{
}
    
const char* DOMHTMLFormElement::target()
{
    return "";
}
    
void DOMHTMLFormElement::setTarget(const char* /*target*/)
{
}
    
void DOMHTMLFormElement::submit()
{
}
    
void DOMHTMLFormElement::reset()
{
}

// DOMHTMLSelectElement -------------------------------------------------------

const char* DOMHTMLSelectElement::type()
{
    return "";
}
    
int DOMHTMLSelectElement::selectedIndex()
{
    return 0;
}
    
void DOMHTMLSelectElement::setSelectedIndx(int /*selectedIndex*/)
{
}
    
const char* DOMHTMLSelectElement::value()
{
    return "";
}
    
void DOMHTMLSelectElement::setValue(const char* /*value*/)
{
}
    
int DOMHTMLSelectElement::length()
{
    return 0;
}
    
DOMHTMLFormElement* DOMHTMLSelectElement::form()
{
    return 0;
}
    
DOMHTMLOptionsCollection* DOMHTMLSelectElement::options()
{
    return 0;
}
    
bool DOMHTMLSelectElement::disabled()
{
    return false;
}
    
void DOMHTMLSelectElement::setDisabled(bool /*disabled*/)
{
}
    
bool DOMHTMLSelectElement::multiple()
{
    return false;
}
    
void DOMHTMLSelectElement::setMultiple(bool /*multiple*/)
{
}
    
const char* DOMHTMLSelectElement::name()
{
    return "";
}
    
void DOMHTMLSelectElement::setName(const char* /*name*/)
{
}
    
int DOMHTMLSelectElement::size()
{
    return 0;
}
    
void DOMHTMLSelectElement::setSize(int /*size*/)
{
}
    
int DOMHTMLSelectElement::tabIndex()
{
    return 0;
}
    
void DOMHTMLSelectElement::setTabIndex(int /*tabIndex*/)
{
}
    
void DOMHTMLSelectElement::add(DOMHTMLElement* /*element*/, DOMHTMLElement* /*before*/)
{
}
    
void DOMHTMLSelectElement::remove(int /*index*/)
{
}
    
// DOMHTMLSelectElement - FormsAutoFillTransitionSelect ----------------------

void DOMHTMLSelectElement::activateItemAtIndex(int /*index*/)
{
}

// DOMHTMLOptionElement -------------------------------------------------------

DOMHTMLFormElement* DOMHTMLOptionElement::form()
{
    return 0;
}
    
bool DOMHTMLOptionElement::defaultSelected()
{
    return false;
}
    
void DOMHTMLOptionElement::setDefaultSelected(bool /*defaultSelected*/)
{
}
    
const char* DOMHTMLOptionElement::text()
{
    return "";
}
    
int DOMHTMLOptionElement::index()
{
    return 0;
}
    
bool DOMHTMLOptionElement::disabled()
{
    return false;
}
    
void DOMHTMLOptionElement::setDisabled(bool /*disabled*/)
{
}
    
const char* DOMHTMLOptionElement::label()
{
    return "";
}
    
void DOMHTMLOptionElement::setLabel(const char* /*label*/)
{
}
    
bool DOMHTMLOptionElement::selected()
{
    return false;
}
    
void DOMHTMLOptionElement::setSelected(bool /*selected*/)
{
}
    
const char* DOMHTMLOptionElement::value()
{
    return "";
}
    
void DOMHTMLOptionElement::setValue(const char* /*value*/)
{
}

// DOMHTMLInputElement --------------------------------------------------------

const char* DOMHTMLInputElement::defaultValue()
{
    return "";
}
    
void DOMHTMLInputElement::setDefaultValue(const char* /*val*/)
{
}
    
bool DOMHTMLInputElement::defaultChecked()
{
    return false;
}
    
void DOMHTMLInputElement::setDefaultChecked(const char* /*checked*/)
{
}
    
DOMHTMLElement* DOMHTMLInputElement::form()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    return DOMHTMLElement::createInstance(static_cast<HTMLInputElement*>(m_element));
}
    
const char* DOMHTMLInputElement::accept()
{
    return "";
}
    
void DOMHTMLInputElement::setAccept(const char* /*accept*/)
{
}
    
const char* DOMHTMLInputElement::accessKey()
{
    return "";
}
    
void DOMHTMLInputElement::setAccessKey(const char* /*key*/)
{
}
    
const char* DOMHTMLInputElement::align()
{
    return "";
}
    
void DOMHTMLInputElement::setAlign(const char* /*align*/)
{
}
    
const char* DOMHTMLInputElement::alt()
{
    return "";
}
    
void DOMHTMLInputElement::setAlt(const char* /*alt*/)
{
}
    
bool DOMHTMLInputElement::checked()
{
    return false;
}
    
void DOMHTMLInputElement::setChecked(bool /*checked*/)
{
}
    
bool DOMHTMLInputElement::disabled()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->isDisabledFormControl() ? true : false;
}
    
void DOMHTMLInputElement::setDisabled(bool /*disabled*/)
{
}
    
int DOMHTMLInputElement::maxLength()
{
    return 0;
}
    
void DOMHTMLInputElement::setMaxLength(int /*maxLength*/)
{
}
    
const char* DOMHTMLInputElement::name()
{
    return "";
}
    
void DOMHTMLInputElement::setName(const char* /*name*/)
{
}
    
bool DOMHTMLInputElement::readOnly()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->isReadOnly() ? true : false;
}
    
void DOMHTMLInputElement::setReadOnly(bool /*readOnly*/)
{
}
    
unsigned int DOMHTMLInputElement::size()
{
    return 0;
}
    
void DOMHTMLInputElement::setSize(unsigned int /*size*/)
{
}
    
const char* DOMHTMLInputElement::src()
{
    return "";
}
    
void DOMHTMLInputElement::setSrc(const char* /*src*/)
{
}
    
int DOMHTMLInputElement::tabIndex()
{
    return 0;
}
    
void DOMHTMLInputElement::setTabIndex(int /*tabIndex*/)
{
}
    
const char* DOMHTMLInputElement::type()
{
    return "";
}
    
void DOMHTMLInputElement::setType(const char* type)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->setType(type);
}
    
const char* DOMHTMLInputElement::useMap()
{
    return "";
}
    
void DOMHTMLInputElement::setUseMap(const char* /*useMap*/)
{
}
    
const char* DOMHTMLInputElement::value()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return strdup(inputElement->value().utf8().data());
}
    
void DOMHTMLInputElement::setValue(const char* value)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->setValue(value);
}
    
void DOMHTMLInputElement::select()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->select();
}
    
void DOMHTMLInputElement::click()
{
}

void DOMHTMLInputElement::setSelectionStart(long start)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->setSelectionStart(start);
}

long DOMHTMLInputElement::selectionStart()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->selectionStart();
}

void DOMHTMLInputElement::setSelectionEnd(long end)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->setSelectionEnd(end);
}

long DOMHTMLInputElement::selectionEnd()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->selectionEnd();
}

// DOMHTMLInputElement -- FormsAutoFillTransition ----------------------------

bool DOMHTMLInputElement::isTextField()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->isTextField() ? true : false;
}

BalRectangle DOMHTMLInputElement::rectOnScreen()
{
    RenderObject* renderer = m_element->renderer();
    FrameView* view = m_element->document().view();
    if (!renderer || !view)
        return WebCore::IntRect();

    WebCore::IntRect coreRect = renderer->absoluteBoundingBoxRect();
    coreRect.setLocation(view->contentsToWindow(coreRect.location()));
    return coreRect;
}

void DOMHTMLInputElement::replaceCharactersInRange(int /*startTarget*/, int /*endTarget*/, const char* /*replacementString*/, int /*index*/)
{
}

void DOMHTMLInputElement::selectedRange(int* start, int* end)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    *start = inputElement->selectionStart();
    *end = inputElement->selectionEnd();
}

void DOMHTMLInputElement::setAutofilled(bool filled)
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    inputElement->setAutoFilled(!!filled);
}

bool DOMHTMLInputElement::isAutofilled()
{
	ASSERT(m_element);
    ASSERT(m_element->hasTagName(inputTag));
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_element);
    return inputElement->isAutoFilled();
}

// DOMHTMLInputElement -- FormPromptAdditions ------------------------------------

bool DOMHTMLInputElement::isUserEdited()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    bool textField = isTextField();
    if (!textField)
        return false;
    if (static_cast<HTMLInputElement*>(m_element)->lastChangeWasUserEdit())
        return true;
    return false;
}

// DOMHTMLTextAreaElement -----------------------------------------------------

const char* DOMHTMLTextAreaElement::defaultValue()
{
    return "";
}
    
void DOMHTMLTextAreaElement::setDefaultValue(const char* /*val*/)
{
}
    
DOMHTMLElement* DOMHTMLTextAreaElement::form()
{
    ASSERT(m_element && m_element->hasTagName(textareaTag));
    return DOMHTMLElement::createInstance(static_cast<HTMLTextAreaElement*>(m_element));
}
    
const char* DOMHTMLTextAreaElement::accessKey()
{
    return "";
}
    
void DOMHTMLTextAreaElement::setAccessKey(const char* /*key*/)
{
}
    
int DOMHTMLTextAreaElement::cols()
{
    return 0;
}
    
void DOMHTMLTextAreaElement::setCols(int /*cols*/)
{
}
    
bool DOMHTMLTextAreaElement::disabled()
{
    return false;
}
    
void DOMHTMLTextAreaElement::setDisabled(bool /*disabled*/)
{
}
    
const char* DOMHTMLTextAreaElement::name()
{
    return "";
}
    
void DOMHTMLTextAreaElement::setName(const char* /*name*/)
{
}
    
bool DOMHTMLTextAreaElement::readOnly()
{
    return false;
}
    
void DOMHTMLTextAreaElement::setReadOnly(bool /*readOnly*/)
{
}
    
int DOMHTMLTextAreaElement::rows()
{
    return 0;
}
    
void DOMHTMLTextAreaElement::setRows(int /*rows*/)
{
}
    
int DOMHTMLTextAreaElement::tabIndex()
{
    return 0;
}
    
void DOMHTMLTextAreaElement::setTabIndex(int /*tabIndex*/)
{
}
    
const char* DOMHTMLTextAreaElement::type()
{
    return "";
}
    
const char* DOMHTMLTextAreaElement::value()
{
    ASSERT(m_element && m_element->hasTagName(textareaTag));
    HTMLTextAreaElement* textareaElement = static_cast<HTMLTextAreaElement*>(m_element);
    return strdup(textareaElement->value().utf8().data());
}
    
void DOMHTMLTextAreaElement::setValue(const char* value)
{
    ASSERT(m_element && m_element->hasTagName(textareaTag));
    HTMLTextAreaElement* textareaElement = static_cast<HTMLTextAreaElement*>(m_element);
    textareaElement->setValue(value);
}
    
void DOMHTMLTextAreaElement::select()
{
    ASSERT(m_element && m_element->hasTagName(textareaTag));
    HTMLTextAreaElement* textareaElement = static_cast<HTMLTextAreaElement*>(m_element);
    textareaElement->select();
}

// DOMHTMLTextAreaElement -- FormPromptAdditions ------------------------------------

bool DOMHTMLTextAreaElement::isUserEdited()
{
    ASSERT(m_element && m_element->hasTagName(inputTag));
    if (static_cast<HTMLInputElement*>(m_element)->lastChangeWasUserEdit())
        return true;
    return false;
}
