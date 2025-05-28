/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "AutofillElements.h"
#include "HTMLFormElement.h"
#include "HTMLCollection.h"

#include "FocusController.h"
#include "Page.h"

namespace WebCore {

static inline bool isAutofillableElement(Element& node)
{
    if (!is<HTMLInputElement>(node))
        return false;

    auto inputElement = &downcast<HTMLInputElement>(node);
    return inputElement->isTextField() || inputElement->isEmailField();
}

static inline RefPtr<HTMLInputElement> nextAutofillableElement(Node* startNode, FocusController& focusController)
{
    if (!is<Element>(startNode))
        return nullptr;

    RefPtr<Element> nextElement = downcast<Element>(startNode);
    do {
        nextElement = focusController.nextFocusableElement(*nextElement.get());
    } while (nextElement && !isAutofillableElement(*nextElement.get()));

    if (!nextElement)
        return nullptr;

    return &downcast<HTMLInputElement>(*nextElement);
}

static inline RefPtr<HTMLInputElement> previousAutofillableElement(Node* startNode, FocusController& focusController)
{
    if (!is<Element>(startNode))
        return nullptr;

    RefPtr<Element> previousElement = downcast<Element>(startNode);
    do {
        previousElement = focusController.previousFocusableElement(*previousElement.get());
    } while (previousElement && !isAutofillableElement(*previousElement.get()));

    if (!previousElement)
        return nullptr;
    
    return &downcast<HTMLInputElement>(*previousElement);
}

AutofillElements::AutofillElements()
{
}

bool AutofillElements::computeAutofillElements(Ref<HTMLInputElement> start)
{
    if (!start->document().page())
        return false;
    FocusController& focusController = start->document().page()->focusController();
    if (start->isPasswordField()) {
        RefPtr<HTMLInputElement> previousElement = previousAutofillableElement(start.ptr(), focusController);
        RefPtr<HTMLInputElement> nextElement = nextAutofillableElement(start.ptr(), focusController);
        bool hasDuplicatePasswordElements = (nextElement && nextElement->isPasswordField()) || (previousElement && previousElement->isPasswordField());
        if (hasDuplicatePasswordElements)
            return false;

        if (previousElement && is<HTMLInputElement>(*previousElement)) {
            if (previousElement->isTextField())
            {
            	m_username = previousElement;
            	m_password = start.ptr();
            	return true;
			}
        }
    } else {
        RefPtr<HTMLInputElement> nextElement = nextAutofillableElement(start.ptr(), focusController);
        if (nextElement && is<HTMLInputElement>(*nextElement)) {
            if (nextElement->isPasswordField()) {
                RefPtr<HTMLInputElement> elementAfternextElement = nextAutofillableElement(nextElement.get(), focusController);
                bool hasDuplicatePasswordElements = elementAfternextElement && elementAfternextElement->isPasswordField();
                if (hasDuplicatePasswordElements)
                    return false;
            	m_username = start.ptr();
            	m_password = nextElement;
                return true;
            }
        }
    }

    if (start->isPasswordField()) {
        RefPtr<HTMLInputElement> previousElement = previousAutofillableElement(start.ptr(), focusController);
        RefPtr<HTMLInputElement> nextElement = nextAutofillableElement(start.ptr(), focusController);
        if (!previousElement && !nextElement)
        {
			m_username = nullptr;
			m_password = start.ptr();

			if (start.ptr() && start->form())
			{
				auto elements = start->form()->elementsForNativeBindings();
				for (unsigned i = 0; i < elements->length(); i++)
				{
					Element* e = elements->item(i);
					if (isAutofillableElement(*e) && e != start.ptr())
					{
						m_username = &downcast<HTMLInputElement>(*e);
						break;
					}
				}
			}

            return true;
		}
    }
    return false;
}

void AutofillElements::autofill(String username, String password)
{
    if (m_username)
        m_username->setValueForUser(username);
    if (m_password)
        m_password->setValueForUser(password);
}

} // namespace WebCore
