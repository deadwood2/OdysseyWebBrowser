/*
 * Copyright (C) 2006, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "DragImage.h"
#include "URL.h"
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(IOS)
OBJC_CLASS NSString;
#endif

#if PLATFORM(COCOA)
OBJC_CLASS NSArray;
#endif

#if PLATFORM(WIN)
#include "COMPtr.h"
#include "WCDataObject.h"
#include <objidl.h>
#include <windows.h>
typedef struct HWND__* HWND;
#endif

// FIXME: This class uses the DOM and makes calls to Editor.
// It should be divested of its knowledge of the frame and editor.

namespace WebCore {

class DocumentFragment;
class DragData;
class Element;
class Frame;
class Range;
class SelectionData;
class SharedBuffer;

enum ShouldSerializeSelectedTextForDataTransfer { DefaultSelectedTextType, IncludeImageAltTextForDataTransfer };

// For writing to the pasteboard. Generally sorted with the richest formats on top.

struct PasteboardWebContent {
#if !(PLATFORM(GTK) || PLATFORM(WIN) || PLATFORM(WPE))
    WEBCORE_EXPORT PasteboardWebContent();
    WEBCORE_EXPORT ~PasteboardWebContent();
    bool canSmartCopyOrDelete;
    RefPtr<SharedBuffer> dataInWebArchiveFormat;
    RefPtr<SharedBuffer> dataInRTFDFormat;
    RefPtr<SharedBuffer> dataInRTFFormat;
    RefPtr<SharedBuffer> dataInAttributedStringFormat;
    String dataInHTMLFormat;
    String dataInStringFormat;
    Vector<String> clientTypes;
    Vector<RefPtr<SharedBuffer>> clientData;
#endif
#if PLATFORM(GTK)
    bool canSmartCopyOrDelete;
    String text;
    String markup;
#endif
#if PLATFORM(WPE)
    String text;
    String markup;
#endif
};

struct PasteboardURL {
    URL url;
    String title;
#if PLATFORM(MAC)
    String userVisibleForm;
#endif
#if PLATFORM(GTK)
    String markup;
#endif
};

struct PasteboardImage {
    WEBCORE_EXPORT PasteboardImage();
    WEBCORE_EXPORT ~PasteboardImage();
    RefPtr<Image> image;
#if PLATFORM(MAC)
    RefPtr<SharedBuffer> dataInWebArchiveFormat;
#endif
#if !PLATFORM(WIN)
    PasteboardURL url;
#endif
#if !(PLATFORM(GTK) || PLATFORM(WIN))
    RefPtr<SharedBuffer> resourceData;
    String resourceMIMEType;
    Vector<String> clientTypes;
    Vector<RefPtr<SharedBuffer>> clientData;
#endif
    String suggestedName;
    FloatSize imageSize;
};

// For reading from the pasteboard.

class PasteboardWebContentReader {
public:
    virtual ~PasteboardWebContentReader() { }

#if !(PLATFORM(GTK) || PLATFORM(WIN))
    virtual bool readWebArchive(SharedBuffer*) = 0;
    virtual bool readFilenames(const Vector<String>&) = 0;
    virtual bool readHTML(const String&) = 0;
    virtual bool readRTFD(SharedBuffer&) = 0;
    virtual bool readRTF(SharedBuffer&) = 0;
    virtual bool readImage(Ref<SharedBuffer>&&, const String& type) = 0;
    virtual bool readURL(const URL&, const String& title) = 0;
#endif
    virtual bool readPlainText(const String&) = 0;
};

struct PasteboardPlainText {
    String text;
#if PLATFORM(COCOA)
    bool isURL;
#endif
};

class Pasteboard {
    WTF_MAKE_NONCOPYABLE(Pasteboard); WTF_MAKE_FAST_ALLOCATED;
public:
    Pasteboard();
    virtual ~Pasteboard();

#if PLATFORM(GTK)
    explicit Pasteboard(const String& name);
    explicit Pasteboard(SelectionData&);
#endif

#if PLATFORM(WIN)
    explicit Pasteboard(IDataObject*);
    explicit Pasteboard(WCDataObject*);
    explicit Pasteboard(const DragDataMap&);
#endif

    WEBCORE_EXPORT static std::unique_ptr<Pasteboard> createForCopyAndPaste();
    static std::unique_ptr<Pasteboard> createPrivate(); // Temporary pasteboard. Can put data on this and then write to another pasteboard with writePasteboard.

    virtual bool hasData();
    virtual Vector<String> types();
    virtual String readString(const String& type);

    virtual void writeString(const String& type, const String& data);
    virtual void clear();
    virtual void clear(const String& type);

    virtual void read(PasteboardPlainText&);
    virtual void read(PasteboardWebContentReader&);

    virtual void write(const PasteboardURL&);
    virtual void writeTrustworthyWebURLsPboardType(const PasteboardURL&);
    virtual void write(const PasteboardImage&);
    virtual void write(const PasteboardWebContent&);

    virtual Vector<String> readFilenames();
    virtual bool canSmartReplace();

    virtual void writeMarkup(const String& markup);
    enum SmartReplaceOption { CanSmartReplace, CannotSmartReplace };
    virtual WEBCORE_EXPORT void writePlainText(const String&, SmartReplaceOption); // FIXME: Two separate functions would be clearer than one function with an argument.
    virtual void writePasteboard(const Pasteboard& sourcePasteboard);

#if ENABLE(DRAG_SUPPORT)
    WEBCORE_EXPORT static std::unique_ptr<Pasteboard> createForDragAndDrop();
    WEBCORE_EXPORT static std::unique_ptr<Pasteboard> createForDragAndDrop(const DragData&);

    virtual void setDragImage(DragImage, const IntPoint& hotSpot);
#endif

#if PLATFORM(WIN)
    RefPtr<DocumentFragment> documentFragment(Frame&, Range&, bool allowPlainText, bool& chosePlainText); // FIXME: Layering violation.
    void writeImage(Element&, const URL&, const String& title); // FIXME: Layering violation.
    void writeSelection(Range&, bool canSmartCopyOrDelete, Frame&, ShouldSerializeSelectedTextForDataTransfer = DefaultSelectedTextType); // FIXME: Layering violation.
#endif

#if PLATFORM(GTK)
    const SelectionData& selectionData() const;
    static std::unique_ptr<Pasteboard> createForGlobalSelection();
#endif

#if PLATFORM(IOS)
    explicit Pasteboard(long changeCount);

    static NSArray *supportedWebContentPasteboardTypes();
    static String resourceMIMEType(const NSString *mimeType);
#endif

#if PLATFORM(COCOA)
    explicit Pasteboard(const String& pasteboardName);

    WEBCORE_EXPORT static NSArray *supportedFileUploadPasteboardTypes();
    const String& name() const { return m_pasteboardName; }
#endif

#if PLATFORM(WIN)
    COMPtr<IDataObject> dataObject() const { return m_dataObject; }
    void setExternalDataObject(IDataObject*);
    const DragDataMap& dragDataMap() const { return m_dragDataMap; }
    void writeURLToWritableDataObject(const URL&, const String&);
    COMPtr<WCDataObject> writableDataObject() const { return m_writableDataObject; }
    void writeImageToDataObject(Element&, const URL&); // FIXME: Layering violation.
#endif

private:
#if PLATFORM(IOS)
    bool respectsUTIFidelities() const;
    void readRespectingUTIFidelities(PasteboardWebContentReader&);
#endif

#if PLATFORM(WIN)
    void finishCreatingPasteboard();
    void writeRangeToDataObject(Range&, Frame&); // FIXME: Layering violation.
    void writeURLToDataObject(const URL&, const String&);
    void writePlainTextToDataObject(const String&, SmartReplaceOption);
#endif

#if PLATFORM(GTK)
    void writeToClipboard();
    void readFromClipboard();
    Ref<SelectionData> m_selectionData;
    String m_name;
#endif

#if PLATFORM(COCOA)
    String m_pasteboardName;
    long m_changeCount;
#endif

#if PLATFORM(WIN)
    HWND m_owner;
    COMPtr<IDataObject> m_dataObject;
    COMPtr<WCDataObject> m_writableDataObject;
    DragDataMap m_dragDataMap;
#endif
};

#if PLATFORM(IOS)
extern NSString *WebArchivePboardType;
#endif

#if PLATFORM(MAC)
extern const char* const WebArchivePboardType;
extern const char* const WebURLNamePboardType;
#endif

#if !PLATFORM(GTK)

inline Pasteboard::~Pasteboard()
{
}

#endif

} // namespace WebCore
