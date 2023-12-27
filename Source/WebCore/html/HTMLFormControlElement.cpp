/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "HTMLFormControlElement.h"

#include "ControlStates.h"
#include "ElementAncestorIterator.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLFieldSetElement.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLLegendElement.h"
#include "HTMLTextAreaElement.h"
#include "RenderBox.h"
#include "RenderTheme.h"
#include "ValidationMessage.h"
#include <wtf/Ref.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;

HTMLFormControlElement::HTMLFormControlElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
    : LabelableElement(tagName, document)
    , m_disabled(false)
    , m_isReadOnly(false)
    , m_isRequired(false)
    , m_valueMatchesRenderer(false)
    , m_disabledByAncestorFieldset(false)
    , m_dataListAncestorState(Unknown)
    , m_willValidateInitialized(false)
    , m_willValidate(true)
    , m_isValid(true)
    , m_wasChangedSinceLastFormControlChangeEvent(false)
    , m_hasAutofocused(false)
{
    setForm(form);
    setHasCustomStyleResolveCallbacks();
}

HTMLFormControlElement::~HTMLFormControlElement()
{
    // The calls willChangeForm() and didChangeForm() are virtual, we want the
    // form to be reset while this object still exists.
    setForm(nullptr);
}

String HTMLFormControlElement::formEnctype() const
{
    const AtomicString& formEnctypeAttr = fastGetAttribute(formenctypeAttr);
    if (formEnctypeAttr.isNull())
        return emptyString();
    return FormSubmission::Attributes::parseEncodingType(formEnctypeAttr);
}

void HTMLFormControlElement::setFormEnctype(const String& value)
{
    setAttribute(formenctypeAttr, value);
}

String HTMLFormControlElement::formMethod() const
{
    const AtomicString& formMethodAttr = fastGetAttribute(formmethodAttr);
    if (formMethodAttr.isNull())
        return emptyString();
    return FormSubmission::Attributes::methodString(FormSubmission::Attributes::parseMethodType(formMethodAttr));
}

void HTMLFormControlElement::setFormMethod(const String& value)
{
    setAttribute(formmethodAttr, value);
}

bool HTMLFormControlElement::formNoValidate() const
{
    return fastHasAttribute(formnovalidateAttr);
}

bool HTMLFormControlElement::computeIsDisabledByFieldsetAncestor() const
{
    Element* previousAncestor = nullptr;
    for (Element* ancestor = parentElement(); ancestor; ancestor = ancestor->parentElement()) {
        if (is<HTMLFieldSetElement>(*ancestor) && ancestor->fastHasAttribute(disabledAttr)) {
            HTMLFieldSetElement& fieldSetAncestor = downcast<HTMLFieldSetElement>(*ancestor);
            bool isInFirstLegend = is<HTMLLegendElement>(previousAncestor) && previousAncestor == fieldSetAncestor.legend();
            return !isInFirstLegend;
        }
        previousAncestor = ancestor;
    }
    return false;
}

void HTMLFormControlElement::setAncestorDisabled(bool isDisabled)
{
    ASSERT(computeIsDisabledByFieldsetAncestor() == isDisabled);
    bool oldValue = m_disabledByAncestorFieldset;
    m_disabledByAncestorFieldset = isDisabled;
    if (oldValue != m_disabledByAncestorFieldset)
        disabledStateChanged();
}

void HTMLFormControlElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == formAttr)
        formAttributeChanged();
    else if (name == disabledAttr) {
        bool oldDisabled = m_disabled;
        m_disabled = !value.isNull();
        if (oldDisabled != m_disabled)
            disabledAttributeChanged();
    } else if (name == readonlyAttr) {
        bool wasReadOnly = m_isReadOnly;
        m_isReadOnly = !value.isNull();
        if (wasReadOnly != m_isReadOnly)
            readOnlyAttributeChanged();
    } else if (name == requiredAttr) {
        bool wasRequired = m_isRequired;
        m_isRequired = !value.isNull();
        if (wasRequired != m_isRequired)
            requiredAttributeChanged();
    } else
        HTMLElement::parseAttribute(name, value);
}

void HTMLFormControlElement::disabledAttributeChanged()
{
    disabledStateChanged();
}

void HTMLFormControlElement::disabledStateChanged()
{
    setNeedsWillValidateCheck();
    setNeedsStyleRecalc();
    if (renderer() && renderer()->style().hasAppearance())
        renderer()->theme().stateChanged(*renderer(), ControlStates::EnabledState);
}

void HTMLFormControlElement::readOnlyAttributeChanged()
{
    setNeedsWillValidateCheck();
    setNeedsStyleRecalc();
}

void HTMLFormControlElement::requiredAttributeChanged()
{
    updateValidity();
    // Style recalculation is needed because style selectors may include
    // :required and :optional pseudo-classes.
    setNeedsStyleRecalc();
}

static bool shouldAutofocus(HTMLFormControlElement* element)
{
    if (!element->renderer())
        return false;
    if (!element->fastHasAttribute(autofocusAttr))
        return false;
    if (!element->inDocument() || !element->document().renderView())
        return false;
    if (element->document().isSandboxed(SandboxAutomaticFeatures)) {
        // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
        element->document().addConsoleMessage(MessageSource::Security, MessageLevel::Error, ASCIILiteral("Blocked autofocusing on a form control because the form's frame is sandboxed and the 'allow-scripts' permission is not set."));
        return false;
    }
    if (element->hasAutofocused())
        return false;

    // FIXME: Should this set of hasTagName checks be replaced by a
    // virtual member function?
    if (is<HTMLInputElement>(*element))
        return !downcast<HTMLInputElement>(*element).isInputTypeHidden();
    if (element->hasTagName(selectTag))
        return true;
    if (element->hasTagName(keygenTag))
        return true;
    if (element->hasTagName(buttonTag))
        return true;
    if (is<HTMLTextAreaElement>(*element))
        return true;

    return false;
}

void HTMLFormControlElement::didAttachRenderers()
{
    // The call to updateFromElement() needs to go after the call through
    // to the base class's attach() because that can sometimes do a close
    // on the renderer.
    if (renderer())
        renderer()->updateFromElement();

    if (shouldAutofocus(this)) {
        setAutofocused();

        RefPtr<HTMLFormControlElement> element = this;
        Style::queuePostResolutionCallback([element] {
            element->focus();
        });
    }
}

void HTMLFormControlElement::didMoveToNewDocument(Document* oldDocument)
{
    FormAssociatedElement::didMoveToNewDocument(oldDocument);
    HTMLElement::didMoveToNewDocument(oldDocument);
}

static void addInvalidElementToAncestorFromInsertionPoint(const HTMLFormControlElement& element, ContainerNode* insertionPoint)
{
    if (!is<Element>(insertionPoint))
        return;

    for (auto& ancestor : lineageOfType<HTMLFieldSetElement>(downcast<Element>(*insertionPoint)))
        ancestor.addInvalidDescendant(element);
}

static void removeInvalidElementToAncestorFromInsertionPoint(const HTMLFormControlElement& element, ContainerNode* insertionPoint)
{
    if (!is<Element>(insertionPoint))
        return;

    for (auto& ancestor : lineageOfType<HTMLFieldSetElement>(downcast<Element>(*insertionPoint)))
        ancestor.removeInvalidDescendant(element);
}

Node::InsertionNotificationRequest HTMLFormControlElement::insertedInto(ContainerNode& insertionPoint)
{
    if (willValidate() && !isValidFormControlElement())
        addInvalidElementToAncestorFromInsertionPoint(*this, &insertionPoint);

    if (document().hasDisabledFieldsetElement())
        setAncestorDisabled(computeIsDisabledByFieldsetAncestor());
    m_dataListAncestorState = Unknown;
    setNeedsWillValidateCheck();
    HTMLElement::insertedInto(insertionPoint);
    FormAssociatedElement::insertedInto(insertionPoint);

    return InsertionDone;
}

void HTMLFormControlElement::removedFrom(ContainerNode& insertionPoint)
{
    bool wasMatchingInvalidPseudoClass = willValidate() && !isValidFormControlElement();

    m_validationMessage = nullptr;
    if (m_disabledByAncestorFieldset)
        setAncestorDisabled(computeIsDisabledByFieldsetAncestor());
    m_dataListAncestorState = Unknown;
    HTMLElement::removedFrom(insertionPoint);
    FormAssociatedElement::removedFrom(insertionPoint);

    if (wasMatchingInvalidPseudoClass)
        removeInvalidElementToAncestorFromInsertionPoint(*this, &insertionPoint);
}

void HTMLFormControlElement::setChangedSinceLastFormControlChangeEvent(bool changed)
{
    m_wasChangedSinceLastFormControlChangeEvent = changed;
}

void HTMLFormControlElement::dispatchChangeEvent()
{
    dispatchScopedEvent(Event::create(eventNames().changeEvent, true, false));
}

void HTMLFormControlElement::dispatchFormControlChangeEvent()
{
    dispatchChangeEvent();
    setChangedSinceLastFormControlChangeEvent(false);
}

void HTMLFormControlElement::dispatchFormControlInputEvent()
{
    setChangedSinceLastFormControlChangeEvent(true);
    HTMLElement::dispatchInputEvent();
}

bool HTMLFormControlElement::isDisabledFormControl() const
{
    return m_disabled || m_disabledByAncestorFieldset;
}

bool HTMLFormControlElement::isRequired() const
{
    return m_isRequired;
}

void HTMLFormControlElement::didRecalcStyle(Style::Change)
{
    // updateFromElement() can cause the selection to change, and in turn
    // trigger synchronous layout, so it must not be called during style recalc.
    if (renderer()) {
        RefPtr<HTMLFormControlElement> element = this;
        Style::queuePostResolutionCallback([element]{
            if (auto* renderer = element->renderer())
                renderer->updateFromElement();
        });
    }
}

bool HTMLFormControlElement::supportsFocus() const
{
    return !isDisabledFormControl();
}

bool HTMLFormControlElement::isFocusable() const
{
    // If there's a renderer, make sure the size isn't empty, but if there's no renderer,
    // it might still be focusable if it's in a canvas subtree (handled in Node::isFocusable).
    if (renderer() && (!is<RenderBox>(*renderer()) || downcast<RenderBox>(*renderer()).size().isEmpty()))
        return false;
    // HTMLElement::isFocusable handles visibility and calls suportsFocus which
    // will cover the disabled case.
    return HTMLElement::isFocusable();
}

bool HTMLFormControlElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (isFocusable())
        if (document().frame())
            return document().frame()->eventHandler().tabsToAllFormControls(event);
    return false;
}

bool HTMLFormControlElement::isMouseFocusable() const
{
#if PLATFORM(GTK)
    return HTMLElement::isMouseFocusable();
#else
    return false;
#endif
}

bool HTMLFormControlElement::matchesValidPseudoClass() const
{
    return willValidate() && isValidFormControlElement();
}

bool HTMLFormControlElement::matchesInvalidPseudoClass() const
{
    return willValidate() && !isValidFormControlElement();
}

short HTMLFormControlElement::tabIndex() const
{
    // Skip the supportsFocus check in HTMLElement.
    return Element::tabIndex();
}

bool HTMLFormControlElement::computeWillValidate() const
{
    if (m_dataListAncestorState == Unknown) {
        for (ContainerNode* ancestor = parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor->hasTagName(datalistTag)) {
                m_dataListAncestorState = InsideDataList;
                break;
            }
        }
        if (m_dataListAncestorState == Unknown)
            m_dataListAncestorState = NotInsideDataList;
    }
    return m_dataListAncestorState == NotInsideDataList && !isDisabledOrReadOnly();
}

bool HTMLFormControlElement::willValidate() const
{
    if (!m_willValidateInitialized || m_dataListAncestorState == Unknown) {
        m_willValidateInitialized = true;
        bool newWillValidate = computeWillValidate();
        if (m_willValidate != newWillValidate)
            m_willValidate = newWillValidate;
    } else {
        // If the following assertion fails, setNeedsWillValidateCheck() is not
        // called correctly when something which changes computeWillValidate() result
        // is updated.
        ASSERT(m_willValidate == computeWillValidate());
    }
    return m_willValidate;
}

void HTMLFormControlElement::setNeedsWillValidateCheck()
{
    // We need to recalculate willValidate immediately because willValidate change can causes style change.
    bool newWillValidate = computeWillValidate();
    if (m_willValidateInitialized && m_willValidate == newWillValidate)
        return;

    bool wasValid = m_isValid;

    m_willValidateInitialized = true;
    m_willValidate = newWillValidate;

    updateValidity();
    setNeedsStyleRecalc();

    if (!m_willValidate && !wasValid) {
        removeInvalidElementToAncestorFromInsertionPoint(*this, parentNode());
        if (HTMLFormElement* form = this->form())
            form->removeInvalidAssociatedFormControlIfNeeded(*this);
    }

    if (!m_willValidate)
        hideVisibleValidationMessage();
}

void HTMLFormControlElement::updateVisibleValidationMessage()
{
    Page* page = document().page();
    if (!page)
        return;
    String message;
    if (renderer() && willValidate())
        message = validationMessage().stripWhiteSpace();
    if (!m_validationMessage)
        m_validationMessage = std::make_unique<ValidationMessage>(this);
    m_validationMessage->updateValidationMessage(message);
}

void HTMLFormControlElement::hideVisibleValidationMessage()
{
    if (m_validationMessage)
        m_validationMessage->requestToHideMessage();
}

bool HTMLFormControlElement::checkValidity(Vector<RefPtr<FormAssociatedElement>>* unhandledInvalidControls)
{
    if (!willValidate() || isValidFormControlElement())
        return true;
    // An event handler can deref this object.
    Ref<HTMLFormControlElement> protect(*this);
    Ref<Document> originalDocument(document());
    bool needsDefaultAction = dispatchEvent(Event::create(eventNames().invalidEvent, false, true));
    if (needsDefaultAction && unhandledInvalidControls && inDocument() && originalDocument.ptr() == &document())
        unhandledInvalidControls->append(this);
    return false;
}

inline bool HTMLFormControlElement::isValidFormControlElement() const
{
    // If the following assertion fails, updateValidity() is not called
    // correctly when something which changes validity is updated.
    ASSERT(m_isValid == valid());
    return m_isValid;
}

void HTMLFormControlElement::willChangeForm()
{
    if (HTMLFormElement* form = this->form())
        form->removeInvalidAssociatedFormControlIfNeeded(*this);
    FormAssociatedElement::willChangeForm();
}

void HTMLFormControlElement::didChangeForm()
{
    FormAssociatedElement::didChangeForm();
    if (HTMLFormElement* form = this->form()) {
        if (m_willValidateInitialized && m_willValidate && !isValidFormControlElement())
            form->registerInvalidAssociatedFormControl(*this);
    }
}

void HTMLFormControlElement::updateValidity()
{
    bool willValidate = this->willValidate();
    bool wasValid = m_isValid;

    m_isValid = valid();

    if (willValidate && m_isValid != wasValid) {
        // Update style for pseudo classes such as :valid :invalid.
        setNeedsStyleRecalc();

        if (!m_isValid) {
            addInvalidElementToAncestorFromInsertionPoint(*this, parentNode());
            if (HTMLFormElement* form = this->form())
                form->registerInvalidAssociatedFormControl(*this);
        } else {
            removeInvalidElementToAncestorFromInsertionPoint(*this, parentNode());
            if (HTMLFormElement* form = this->form())
                form->removeInvalidAssociatedFormControlIfNeeded(*this);
        }
    }

    // Updates only if this control already has a validtion message.
    if (m_validationMessage && m_validationMessage->isVisible()) {
        // Calls updateVisibleValidationMessage() even if m_isValid is not
        // changed because a validation message can be chagned.
        updateVisibleValidationMessage();
    }
}

void HTMLFormControlElement::setCustomValidity(const String& error)
{
    FormAssociatedElement::setCustomValidity(error);
    updateValidity();
}

bool HTMLFormControlElement::validationMessageShadowTreeContains(const Node& node) const
{
    return m_validationMessage && m_validationMessage->shadowTreeContains(node);
}

void HTMLFormControlElement::dispatchBlurEvent(RefPtr<Element>&& newFocusedElement)
{
    HTMLElement::dispatchBlurEvent(WTF::move(newFocusedElement));
    hideVisibleValidationMessage();
}

HTMLFormElement* HTMLFormControlElement::virtualForm() const
{
    return FormAssociatedElement::form();
}

bool HTMLFormControlElement::isDefaultButtonForForm() const
{
    return isSuccessfulSubmitButton() && form() && form()->defaultButton() == this;
}

#if ENABLE(IOS_AUTOCORRECT_AND_AUTOCAPITALIZE)
// FIXME: We should look to share these methods with class HTMLFormElement instead of duplicating them.

bool HTMLFormControlElement::autocorrect() const
{
    const AtomicString& autocorrectValue = fastGetAttribute(autocorrectAttr);
    if (!autocorrectValue.isEmpty())
        return !equalIgnoringCase(autocorrectValue, "off");
    if (HTMLFormElement* form = this->form())
        return form->autocorrect();
    return true;
}

void HTMLFormControlElement::setAutocorrect(bool autocorrect)
{
    setAttribute(autocorrectAttr, autocorrect ? AtomicString("on", AtomicString::ConstructFromLiteral) : AtomicString("off", AtomicString::ConstructFromLiteral));
}

WebAutocapitalizeType HTMLFormControlElement::autocapitalizeType() const
{
    WebAutocapitalizeType type = autocapitalizeTypeForAttributeValue(fastGetAttribute(autocapitalizeAttr));
    if (type == WebAutocapitalizeTypeDefault) {
        if (HTMLFormElement* form = this->form())
            return form->autocapitalizeType();
    }
    return type;
}

const AtomicString& HTMLFormControlElement::autocapitalize() const
{
    return stringForAutocapitalizeType(autocapitalizeType());
}

void HTMLFormControlElement::setAutocapitalize(const AtomicString& value)
{
    setAttribute(autocapitalizeAttr, value);
}
#endif

HTMLFormControlElement* HTMLFormControlElement::enclosingFormControlElement(Node* node)
{
    for (; node; node = node->parentNode()) {
        if (is<HTMLFormControlElement>(*node))
            return downcast<HTMLFormControlElement>(node);
    }
    return nullptr;
}

} // namespace Webcore
