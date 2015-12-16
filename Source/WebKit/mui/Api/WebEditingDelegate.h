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

#ifndef WebEditingDelegate_h
#define WebEditingDelegate_h

#include "SharedObject.h"
#include <WebKitTypes.h>
#include "DOMRange.h"

class DOMCSSStyleDeclaration;
class DOMHTMLElement;
class DOMNode;
class WebView;

typedef enum {
    WebViewInsertActionTyped,
    WebViewInsertActionPasted,
    WebViewInsertActionDropped,
} WebViewInsertAction;

class WebEditingDelegate : public SharedObject<WebEditingDelegate> {

public:
    virtual ~WebEditingDelegate() {}

    virtual bool shouldBeginEditingInDOMRange(WebView *webView, DOMRange *range) = 0;
    
    virtual bool shouldEndEditingInDOMRange(WebView *webView, DOMRange *range) = 0;
    
    virtual bool shouldInsertNode(WebView *webView, DOMNode *node, DOMRange *range, WebViewInsertAction action) = 0;
    
    virtual bool shouldInsertText(WebView *webView, const char* text, DOMRange *range, WebViewInsertAction action) = 0;
    
    virtual bool shouldDeleteDOMRange(WebView *webView, DOMRange *range) = 0;
    
    virtual bool shouldChangeSelectedDOMRange(WebView *webView, DOMRange *currentRange, DOMRange *proposedRange, WebSelectionAffinity selectionAffinity, bool stillSelecting) = 0;
    
    virtual bool shouldApplyStyle(WebView *webView, DOMCSSStyleDeclaration *style, DOMRange *range) = 0;
    
    virtual bool shouldChangeTypingStyle(WebView *webView, DOMCSSStyleDeclaration *currentStyle, DOMCSSStyleDeclaration *proposedStyle) = 0;
    
    virtual bool doPlatformCommand(WebView *webView, const char* command) = 0;
    
    virtual void webViewDidBeginEditing() = 0;
    
    virtual void webViewDidChange() = 0;
    
    virtual void webViewDidEndEditing() = 0;
    
    virtual void webViewDidChangeTypingStyle() = 0;
    
    virtual void webViewDidChangeSelection() = 0;
    
    //virtual WebUndoManager* undoManagerForWebView(WebView *webView) = 0;

    virtual void ignoreWordInSpellDocument(WebView *view, const char* word) = 0;
        
    virtual void learnWord(const char* word) = 0;
       
    virtual void checkSpellingOfString(WebView *view, const char* text, int length, int *misspellingLocation, int *misspellingLength) = 0;
        
    //virtual void checkGrammarOfString(WebView *view, const char* text, int length, EnumWebGrammarDetails **grammarDetails, int *badGrammarLocation, int *badGrammarLength) = 0;
        
    virtual void updateSpellingUIWithGrammarString(const char* string, int location, int length, const char* userDescription, char** guesses, int guessesCount) = 0;
        
    virtual void updateSpellingUIWithMisspelledWord(const char* word) = 0;
        
    virtual void showSpellingUI(bool show) = 0;
        
    virtual bool spellingUIIsShowing() = 0;

    virtual bool shouldShowDeleteInterface(DOMHTMLElement*) = 0;
        
    //virtual EnumSpellingGuesses* guessesForWord(const char* word) = 0;
};

#endif // !defined(WebEditingDelegate_h)
