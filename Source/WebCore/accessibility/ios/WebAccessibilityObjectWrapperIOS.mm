/*
 * Copyright (C) 2008, 2015 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebAccessibilityObjectWrapperIOS.h"

#if HAVE(ACCESSIBILITY) && PLATFORM(IOS_FAMILY)

#import "AccessibilityAttachment.h"
#import "AccessibilityMediaObject.h"
#import "AccessibilityRenderObject.h"
#import "AccessibilityScrollView.h"
#import "AccessibilityTable.h"
#import "AccessibilityTableCell.h"
#import "Chrome.h"
#import "ChromeClient.h"
#import "FontCascade.h"
#import "Frame.h"
#import "FrameSelection.h"
#import "FrameView.h"
#import "HitTestResult.h"
#import "HTMLFrameOwnerElement.h"
#import "HTMLInputElement.h"
#import "HTMLNames.h"
#import "IntRect.h"
#import "IntSize.h"
#import "LocalizedStrings.h"
#import "Page.h"
#import "Range.h"
#import "RenderView.h"
#import "RuntimeApplicationChecks.h"
#import "SVGNames.h"
#import "SVGElement.h"
#import "SelectionRect.h"
#import "TextIterator.h"
#import "WAKScrollView.h"
#import "WAKWindow.h"
#import "WebCoreThread.h"
#import "VisibleUnits.h"

#import <CoreText/CoreText.h>

enum {
    NSAttachmentCharacter = 0xfffc    /* To denote attachments. */
};

@interface NSObject (AccessibilityPrivate)
- (void)_accessibilityUnregister;
- (NSString *)accessibilityLabel;
- (NSString *)accessibilityValue;
- (BOOL)isAccessibilityElement;
- (NSInteger)accessibilityElementCount;
- (id)accessibilityElementAtIndex:(NSInteger)index;
- (NSInteger)indexOfAccessibilityElement:(id)element;
@end

@interface WebAccessibilityObjectWrapper (AccessibilityPrivate)
- (id)accessibilityContainer;
- (void)setAccessibilityLabel:(NSString *)label;
- (void)setAccessibilityValue:(NSString *)value;
- (BOOL)containsUnnaturallySegmentedChildren;
- (NSInteger)positionForTextMarker:(id)marker;
@end

@implementation WAKView (iOSAccessibility)

- (BOOL)accessibilityIsIgnored
{
    return YES;
}

@end

using namespace WebCore;
using namespace HTMLNames;

typedef NS_ENUM(NSInteger, UIAccessibilityScrollDirection) {
    UIAccessibilityScrollDirectionRight = 1,
    UIAccessibilityScrollDirectionLeft,
    UIAccessibilityScrollDirectionUp,
    UIAccessibilityScrollDirectionDown,
    UIAccessibilityScrollDirectionNext,
    UIAccessibilityScrollDirectionPrevious
};

// These are tokens accessibility uses to denote attributes. 
static NSString * const UIAccessibilityTokenBlockquoteLevel = @"UIAccessibilityTokenBlockquoteLevel";
static NSString * const UIAccessibilityTokenHeadingLevel = @"UIAccessibilityTokenHeadingLevel";
static NSString * const UIAccessibilityTokenFontName = @"UIAccessibilityTokenFontName";
static NSString * const UIAccessibilityTokenFontFamily = @"UIAccessibilityTokenFontFamily";
static NSString * const UIAccessibilityTokenFontSize = @"UIAccessibilityTokenFontSize";
static NSString * const UIAccessibilityTokenBold = @"UIAccessibilityTokenBold";
static NSString * const UIAccessibilityTokenItalic = @"UIAccessibilityTokenItalic";
static NSString * const UIAccessibilityTokenUnderline = @"UIAccessibilityTokenUnderline";
static NSString * const UIAccessibilityTokenLanguage = @"UIAccessibilityTokenLanguage";
static NSString * const UIAccessibilityTokenAttachment = @"UIAccessibilityTokenAttachment";

static NSString * const UIAccessibilityTextAttributeContext = @"UIAccessibilityTextAttributeContext";
static NSString * const UIAccessibilityTextualContextSourceCode = @"UIAccessibilityTextualContextSourceCode";

static AccessibilityObjectWrapper* AccessibilityUnignoredAncestor(AccessibilityObjectWrapper *wrapper)
{
    while (wrapper && ![wrapper isAccessibilityElement]) {
        AccessibilityObject* object = [wrapper accessibilityObject];
        if (!object)
            break;
        
        if ([wrapper isAttachment] && ![[wrapper attachmentView] accessibilityIsIgnored])
            break;
            
        AccessibilityObject* parentObject = object->parentObjectUnignored();
        if (!parentObject)
            break;

        wrapper = parentObject->wrapper();
    }
    return wrapper;
}

#pragma mark Accessibility Text Marker

@interface WebAccessibilityTextMarker : NSObject
{
    AXObjectCache* _cache;
    TextMarkerData _textMarkerData;
}

+ (WebAccessibilityTextMarker *)textMarkerWithVisiblePosition:(VisiblePosition&)visiblePos cache:(AXObjectCache*)cache;
+ (WebAccessibilityTextMarker *)textMarkerWithCharacterOffset:(CharacterOffset&)characterOffset cache:(AXObjectCache*)cache;
+ (WebAccessibilityTextMarker *)startOrEndTextMarkerForRange:(const RefPtr<Range>)range isStart:(BOOL)isStart cache:(AXObjectCache*)cache;

@end

@implementation WebAccessibilityTextMarker

- (id)initWithTextMarker:(TextMarkerData *)data cache:(AXObjectCache*)cache
{
    if (!(self = [super init]))
        return nil;
    
    _cache = cache;
    memcpy(&_textMarkerData, data, sizeof(TextMarkerData));
    return self;
}

- (id)initWithData:(NSData *)data cache:(AXObjectCache*)cache
{
    if (!(self = [super init]))
        return nil;
    
    _cache = cache;
    [data getBytes:&_textMarkerData length:sizeof(TextMarkerData)];
    
    return self;
}

// This is needed for external clients to be able to create a text marker without having a pointer to the cache.
- (id)initWithData:(NSData *)data accessibilityObject:(AccessibilityObjectWrapper *)wrapper
{
    WebCore::AccessibilityObject* axObject = [wrapper accessibilityObject]; 
    if (!axObject)
        return nil;
    
    return [self initWithData:data cache:axObject->axObjectCache()];
}

+ (WebAccessibilityTextMarker *)textMarkerWithVisiblePosition:(VisiblePosition&)visiblePos cache:(AXObjectCache*)cache
{
    auto textMarkerData = cache->textMarkerDataForVisiblePosition(visiblePos);
    if (!textMarkerData)
        return nil;
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData.value() cache:cache] autorelease];
}

+ (WebAccessibilityTextMarker *)textMarkerWithCharacterOffset:(CharacterOffset&)characterOffset cache:(AXObjectCache*)cache
{
    if (!cache)
        return nil;
    
    if (characterOffset.isNull())
        return nil;
    
    TextMarkerData textMarkerData;
    cache->textMarkerDataForCharacterOffset(textMarkerData, characterOffset);
    if (!textMarkerData.axID && !textMarkerData.ignored)
        return nil;
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData cache:cache] autorelease];
}

+ (WebAccessibilityTextMarker *)startOrEndTextMarkerForRange:(const RefPtr<Range>)range isStart:(BOOL)isStart cache:(AXObjectCache*)cache
{
    if (!cache)
        return nil;
    
    TextMarkerData textMarkerData;
    cache->startOrEndTextMarkerDataForRange(textMarkerData, range, isStart);
    if (!textMarkerData.axID)
        return nil;
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData cache:cache] autorelease];
}

- (NSData *)dataRepresentation
{
    return [NSData dataWithBytes:&_textMarkerData length:sizeof(TextMarkerData)];
}

- (VisiblePosition)visiblePosition
{
    return _cache->visiblePositionForTextMarkerData(_textMarkerData);
}

- (CharacterOffset)characterOffset
{
    return _cache->characterOffsetForTextMarkerData(_textMarkerData);
}

- (BOOL)isIgnored
{
    return _textMarkerData.ignored;
}

- (AccessibilityObject*)accessibilityObject
{
    if (_textMarkerData.ignored)
        return nullptr;
    return _cache->accessibilityObjectForTextMarkerData(_textMarkerData);
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"[AXTextMarker %p] = node: %p offset: %d", self, _textMarkerData.node, _textMarkerData.offset];
}

@end

@implementation WebAccessibilityObjectWrapper

- (id)initWithAccessibilityObject:(AccessibilityObject*)axObject
{
    self = [super initWithAccessibilityObject:axObject];
    if (!self)
        return nil;
    
    // Initialize to a sentinel value.
    m_accessibilityTraitsFromAncestor = ULLONG_MAX;
    m_isAccessibilityElement = -1;
    
    return self;
}

- (void)detach
{
    // rdar://8798960 Make sure the object is gone early, so that anything _accessibilityUnregister 
    // does can't call back into the render tree.
    m_object = nullptr;

    if ([self respondsToSelector:@selector(_accessibilityUnregister)])
        [self _accessibilityUnregister];
}

- (void)dealloc
{
    // We should have been detached before deallocated.
    ASSERT(!m_object);
    [super dealloc];
}

- (BOOL)_prepareAccessibilityCall
{
    // rdar://7980318 if we start a call, then block in WebThreadLock(), then we're dealloced on another, thread, we could
    // crash, so we should retain ourself for the duration of usage here.
    [[self retain] autorelease];

    WebThreadLock();
    
    // If we came back from our thread lock and we were detached, we will no longer have an m_object.
    if (!m_object)
        return NO;
    
    m_object->updateBackingStore();
    if (!m_object)
        return NO;
    
    return YES;
}

// These are here so that we don't have to import AXRuntime.
// The methods will be swizzled when the accessibility bundle is loaded.

- (uint64_t)_axLinkTrait { return (1 << 0); }
- (uint64_t)_axVisitedTrait { return (1 << 1); }
- (uint64_t)_axHeaderTrait { return (1 << 2); }
- (uint64_t)_axContainedByListTrait { return (1 << 3); }
- (uint64_t)_axContainedByTableTrait { return (1 << 4); }
- (uint64_t)_axContainedByLandmarkTrait { return (1 << 5); }
- (uint64_t)_axWebContentTrait { return (1 << 6); }
- (uint64_t)_axSecureTextFieldTrait { return (1 << 7); }
- (uint64_t)_axTextEntryTrait { return (1 << 8); }
- (uint64_t)_axHasTextCursorTrait { return (1 << 9); }
- (uint64_t)_axTextOperationsAvailableTrait { return (1 << 10); }
- (uint64_t)_axImageTrait { return (1 << 11); }
- (uint64_t)_axTabButtonTrait { return (1 << 12); }
- (uint64_t)_axButtonTrait { return (1 << 13); }
- (uint64_t)_axToggleTrait { return (1 << 14); }
- (uint64_t)_axPopupButtonTrait { return (1 << 15); }
- (uint64_t)_axStaticTextTrait { return (1 << 16); }
- (uint64_t)_axAdjustableTrait { return (1 << 17); }
- (uint64_t)_axMenuItemTrait { return (1 << 18); }
- (uint64_t)_axSelectedTrait { return (1 << 19); }
- (uint64_t)_axNotEnabledTrait { return (1 << 20); }
- (uint64_t)_axRadioButtonTrait { return (1 << 21); }
- (uint64_t)_axContainedByFieldsetTrait { return (1 << 22); }
- (uint64_t)_axSearchFieldTrait { return (1 << 23); }
- (uint64_t)_axTextAreaTrait { return (1 << 24); }
- (uint64_t)_axUpdatesFrequentlyTrait { return (1 << 25); }

- (BOOL)accessibilityCanFuzzyHitTest
{
    if (![self _prepareAccessibilityCall])
        return false;
    
    AccessibilityRole role = m_object->roleValue();
    // Elements that can be returned when performing fuzzy hit testing.
    switch (role) {
    case AccessibilityRole::Button:
    case AccessibilityRole::CheckBox:
    case AccessibilityRole::ColorWell:
    case AccessibilityRole::ComboBox:
    case AccessibilityRole::DisclosureTriangle:
    case AccessibilityRole::Heading:
    case AccessibilityRole::ImageMapLink:
    case AccessibilityRole::Image:
    case AccessibilityRole::Link:
    case AccessibilityRole::ListBox:
    case AccessibilityRole::ListBoxOption:
    case AccessibilityRole::MenuButton:
    case AccessibilityRole::MenuItem:
    case AccessibilityRole::MenuItemCheckbox:
    case AccessibilityRole::MenuItemRadio:
    case AccessibilityRole::PopUpButton:
    case AccessibilityRole::RadioButton:
    case AccessibilityRole::ScrollBar:
    case AccessibilityRole::SearchField:
    case AccessibilityRole::Slider:
    case AccessibilityRole::StaticText:
    case AccessibilityRole::Switch:
    case AccessibilityRole::Tab:
    case AccessibilityRole::TextField:
    case AccessibilityRole::ToggleButton:
        return !m_object->accessibilityIsIgnored();
    default:
        return false;
    }
}

- (AccessibilityObjectWrapper *)accessibilityPostProcessHitTest:(CGPoint)point
{
    UNUSED_PARAM(point);
    // The UIKit accessibility wrapper will override this and perform the post process hit test.
    return nil;
}

- (id)accessibilityHitTest:(CGPoint)point
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // Try a fuzzy hit test first to find an accessible element.
    RefPtr<AccessibilityObject> axObject;
    {
        AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
        axObject = m_object->accessibilityHitTest(IntPoint(point));
    }

    if (!axObject)
        return nil;
    
    // If this is a good accessible object to return, no extra work is required.
    if ([axObject->wrapper() accessibilityCanFuzzyHitTest])
        return AccessibilityUnignoredAncestor(axObject->wrapper());
    
    // Check to see if we can post-process this hit test to find a better candidate.
    AccessibilityObjectWrapper* wrapper = [axObject->wrapper() accessibilityPostProcessHitTest:point];
    if (wrapper)
        return AccessibilityUnignoredAncestor(wrapper);
    
    // Fall back to default behavior.
    return AccessibilityUnignoredAncestor(axObject->wrapper());    
}

- (void)enableAttributeCaching
{
    if (AXObjectCache* cache = m_object->axObjectCache())
        cache->startCachingComputedObjectAttributesUntilTreeMutates();
}

- (void)disableAttributeCaching
{
    if (AXObjectCache* cache = m_object->axObjectCache())
        cache->stopCachingComputedObjectAttributes();
}

- (NSInteger)accessibilityElementCount
{
    if (![self _prepareAccessibilityCall])
        return 0;

    if ([self isAttachment]) {
        if (id attachmentView = [self attachmentView])
            return [attachmentView accessibilityElementCount];
    }
    
    return m_object->children().size();
}

- (id)accessibilityElementAtIndex:(NSInteger)index
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if ([self isAttachment]) {
        if (id attachmentView = [self attachmentView])
            return [attachmentView accessibilityElementAtIndex:index];
    }
    
    const auto& children = m_object->children();
    size_t elementIndex = static_cast<size_t>(index);
    if (elementIndex >= children.size())
        return nil;
    
    AccessibilityObjectWrapper* wrapper = children[elementIndex]->wrapper();
    if (children[elementIndex]->isAttachment()) {
        if (id attachmentView = [wrapper attachmentView])
            return attachmentView;
    }

    return wrapper;
}

- (NSInteger)indexOfAccessibilityElement:(id)element
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;
    
    if ([self isAttachment]) {
        if (id attachmentView = [self attachmentView])
            return [attachmentView indexOfAccessibilityElement:element];
    }
    
    const auto& children = m_object->children();
    unsigned count = children.size();
    for (unsigned k = 0; k < count; ++k) {
        AccessibilityObjectWrapper* wrapper = children[k]->wrapper();
        if (wrapper == element || (children[k]->isAttachment() && [wrapper attachmentView] == element))
            return k;
    }
    
    return NSNotFound;
}

- (CGPathRef)_accessibilityPath
{
    if (![self _prepareAccessibilityCall])
        return NULL;

    if (!m_object->supportsPath())
        return NULL;
    
    Path path = m_object->elementPath();
    if (path.isEmpty())
        return NULL;
    
    return [self convertPathToScreenSpace:path];
}

- (BOOL)accessibilityHasPopup
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->hasPopup();
}

- (NSString *)accessibilityLanguage
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->language();
}

- (BOOL)accessibilityIsDialog
{
    if (![self _prepareAccessibilityCall])
        return NO;

    AccessibilityRole roleValue = m_object->roleValue();
    return roleValue == AccessibilityRole::ApplicationDialog || roleValue == AccessibilityRole::ApplicationAlertDialog;
}

- (BOOL)_accessibilityIsLandmarkRole:(AccessibilityRole)role
{
    switch (role) {
    case AccessibilityRole::Document:
    case AccessibilityRole::DocumentArticle:
    case AccessibilityRole::DocumentNote:
    case AccessibilityRole::Footer:
    case AccessibilityRole::LandmarkBanner:
    case AccessibilityRole::LandmarkComplementary:
    case AccessibilityRole::LandmarkContentInfo:
    case AccessibilityRole::LandmarkDocRegion:
    case AccessibilityRole::LandmarkMain:
    case AccessibilityRole::LandmarkNavigation:
    case AccessibilityRole::LandmarkRegion:
    case AccessibilityRole::LandmarkSearch:
        return YES;
    default:
        return NO;
    }    
}

- (AccessibilityObjectWrapper*)_accessibilityTreeAncestor
{
    auto matchFunc = [] (const AccessibilityObject& object) {
        AccessibilityRole role = object.roleValue();
        return role == AccessibilityRole::Tree;
    };
    
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, WTFMove(matchFunc)))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityListAncestor
{
    auto matchFunc = [] (const AccessibilityObject& object) {
        AccessibilityRole role = object.roleValue();
        return role == AccessibilityRole::List || role == AccessibilityRole::ListBox;
    };
    
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, WTFMove(matchFunc)))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityArticleAncestor
{
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, [] (const AccessibilityObject& object) {
        return object.roleValue() == AccessibilityRole::DocumentArticle;
    }))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityLandmarkAncestor
{
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, [self] (const AccessibilityObject& object) {
        return [self _accessibilityIsLandmarkRole:object.roleValue()];
    }))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityTableAncestor
{
    
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, [] (const AccessibilityObject& object) {
        return object.isTable();
    }))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityFieldsetAncestor
{
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*m_object, false, [] (const AccessibilityObject& object) {
        return object.isFieldset();
    }))
        return parent->wrapper();
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityFrameAncestor
{
    auto* parent = AccessibilityObject::matchedParent(*m_object, false, [] (const AccessibilityObject& object) {
        return object.isWebArea();
    });
    if (!parent)
        return nil;
    return parent->wrapper();
}

- (uint64_t)_accessibilityTraitsFromAncestors
{
    uint64_t traits = 0;
    AccessibilityRole role = m_object->roleValue();
    
    // Trait information also needs to be gathered from the parents above the object.
    // The parentObject is needed instead of the unignoredParentObject, because a table might be ignored, but information still needs to be gathered from it.    
    for (AccessibilityObject* parent = m_object->parentObject(); parent != nil; parent = parent->parentObject()) {
        AccessibilityRole parentRole = parent->roleValue();
        if (parentRole == AccessibilityRole::WebArea)
            break;
        
        switch (parentRole) {
        case AccessibilityRole::Link:
        case AccessibilityRole::WebCoreLink:
            traits |= [self _axLinkTrait];
            if (parent->isVisited())
                traits |= [self _axVisitedTrait];
            break;
        case AccessibilityRole::Heading: {
            traits |= [self _axHeaderTrait];
            // If this object has the header trait, we should set the value
            // to the heading level. If it was a static text element, we need to store
            // the value as the label, because the heading level needs to the value.
            AccessibilityObjectWrapper* wrapper = parent->wrapper();
            if (role == AccessibilityRole::StaticText) {
                // We should only set the text value as the label when there's no
                // alternate text on the heading parent.
                NSString *headingLabel = [wrapper baseAccessibilityDescription];
                if (![headingLabel length])
                    [self setAccessibilityLabel:m_object->stringValue()];
                else
                    [self setAccessibilityLabel:headingLabel];
            }
            [self setAccessibilityValue:[wrapper accessibilityValue]];
            break;
        }
        case AccessibilityRole::ListBox:
        case AccessibilityRole::List:
            traits |= [self _axContainedByListTrait];
            break;
        case AccessibilityRole::Grid:
        case AccessibilityRole::Table:
        case AccessibilityRole::TreeGrid:
            traits |= [self _axContainedByTableTrait];
            break;
        default:
            if ([self _accessibilityIsLandmarkRole:parentRole])
                traits |= [self _axContainedByLandmarkTrait];
            break;
        }
        
        // If this object has fieldset parent, we should add containedByFieldsetTrait to it.
        if (parent->isFieldset())
            traits |= [self _axContainedByFieldsetTrait];
    }
    
    return traits;
}

- (BOOL)accessibilityIsWebInteractiveVideo
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    // Only make the video object interactive if it plays inline and has no native controls.
    if (m_object->roleValue() != AccessibilityRole::Video || !is<AccessibilityMediaObject>(m_object))
        return NO;
    
    AccessibilityMediaObject* mediaObject = downcast<AccessibilityMediaObject>(m_object);
    return !mediaObject->isAutoplayEnabled() && mediaObject->isPlayingInline() && !downcast<AccessibilityMediaObject>(m_object)->hasControlsAttributeSet();
}

- (NSString *)interactiveVideoDescription
{
    if (!is<AccessibilityMediaObject>(m_object))
        return nil;
    return downcast<AccessibilityMediaObject>(m_object)->interactiveVideoDuration();
}

- (BOOL)accessibilityIsMediaPlaying
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    if (!is<AccessibilityMediaObject>(m_object))
        return NO;
    
    return downcast<AccessibilityMediaObject>(m_object)->isPlaying();
}

- (BOOL)accessibilityIsMediaMuted
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    if (!is<AccessibilityMediaObject>(m_object))
        return NO;
    
    return downcast<AccessibilityMediaObject>(m_object)->isMuted();
}

- (void)accessibilityToggleMuteForMedia
{
    if (![self _prepareAccessibilityCall])
        return;
    
    if (!is<AccessibilityMediaObject>(m_object))
        return;

    downcast<AccessibilityMediaObject>(m_object)->toggleMute();
}

- (void)accessibilityVideoEnterFullscreen
{
    if (![self _prepareAccessibilityCall])
        return;
    
    if (!is<AccessibilityMediaObject>(m_object))
        return;
    
    downcast<AccessibilityMediaObject>(m_object)->enterFullscreen();
}

- (uint64_t)_accessibilityTextEntryTraits
{
    uint64_t traits = [self _axTextEntryTrait];
    if (m_object->isFocused())
        traits |= ([self _axHasTextCursorTrait] | [self _axTextOperationsAvailableTrait]);
    if (m_object->isPasswordField())
        traits |= [self _axSecureTextFieldTrait];
    if (m_object->roleValue() == AccessibilityRole::SearchField)
        traits |= [self _axSearchFieldTrait];
    if (m_object->roleValue() == AccessibilityRole::TextArea)
        traits |= [self _axTextAreaTrait];
    return traits;
}

- (uint64_t)accessibilityTraits
{
    if (![self _prepareAccessibilityCall])
        return 0;
    
    AccessibilityRole role = m_object->roleValue();
    uint64_t traits = [self _axWebContentTrait];
    switch (role) {
    case AccessibilityRole::Link:
    case AccessibilityRole::WebCoreLink:
        traits |= [self _axLinkTrait];
        if (m_object->isVisited())
            traits |= [self _axVisitedTrait];
        break;
    case AccessibilityRole::TextField:
    case AccessibilityRole::SearchField:
    case AccessibilityRole::TextArea:
        traits |= [self _accessibilityTextEntryTraits];
        break;
    case AccessibilityRole::Image:
        traits |= [self _axImageTrait];
        break;
    case AccessibilityRole::Tab:
        traits |= [self _axTabButtonTrait];
        break;
    case AccessibilityRole::Button:
        traits |= [self _axButtonTrait];
        if (m_object->isPressed())
            traits |= [self _axToggleTrait];
        break;
    case AccessibilityRole::PopUpButton:
        traits |= [self _axPopupButtonTrait];
        break;
    case AccessibilityRole::RadioButton:
        traits |= [self _axRadioButtonTrait] | [self _axToggleTrait];
        break;
    case AccessibilityRole::ToggleButton:
    case AccessibilityRole::CheckBox:
    case AccessibilityRole::Switch:
        traits |= ([self _axButtonTrait] | [self _axToggleTrait]);
        break;
    case AccessibilityRole::Heading:
        traits |= [self _axHeaderTrait];
        break;
    case AccessibilityRole::StaticText:
        traits |= [self _axStaticTextTrait];
        break;
    case AccessibilityRole::Slider:
        traits |= [self _axAdjustableTrait];
        break;
    case AccessibilityRole::MenuButton:
    case AccessibilityRole::MenuItem:
        traits |= [self _axMenuItemTrait];
        break;
    case AccessibilityRole::MenuItemCheckbox:
    case AccessibilityRole::MenuItemRadio:
        traits |= ([self _axMenuItemTrait] | [self _axToggleTrait]);
        break;
    default:
        break;
    }

    if (m_object->isAttachmentElement())
        traits |= [self _axUpdatesFrequentlyTrait];
    
    if (m_object->isSelected())
        traits |= [self _axSelectedTrait];

    if (!m_object->isEnabled())
        traits |= [self _axNotEnabledTrait];
    
    if (m_accessibilityTraitsFromAncestor == ULLONG_MAX)
        m_accessibilityTraitsFromAncestor = [self _accessibilityTraitsFromAncestors];
    
    traits |= m_accessibilityTraitsFromAncestor;

    return traits;
}

- (BOOL)isSVGGroupElement
{
    // If an SVG group element has a title, it should be an accessible element on iOS.
    Node* node = m_object->node();
    if (node && node->hasTagName(SVGNames::gTag) && [[self accessibilityLabel] length] > 0)
        return YES;
    
    return NO;
}

- (BOOL)determineIsAccessibilityElement
{
    if (!m_object)
        return false;
    
    // Honor when something explicitly makes this an element (super will contain that logic) 
    if ([super isAccessibilityElement])
        return YES;
    
    m_object->updateBackingStore();
    
    switch (m_object->roleValue()) {
    case AccessibilityRole::TextField:
    case AccessibilityRole::TextArea:
    case AccessibilityRole::Button:
    case AccessibilityRole::ToggleButton:
    case AccessibilityRole::PopUpButton:
    case AccessibilityRole::CheckBox:
    case AccessibilityRole::ColorWell:
    case AccessibilityRole::RadioButton:
    case AccessibilityRole::Slider:
    case AccessibilityRole::MenuButton:
    case AccessibilityRole::ValueIndicator:
    case AccessibilityRole::Image:
    case AccessibilityRole::ImageMapLink:
    case AccessibilityRole::ProgressIndicator:
    case AccessibilityRole::MenuItem:
    case AccessibilityRole::MenuItemCheckbox:
    case AccessibilityRole::MenuItemRadio:
    case AccessibilityRole::Incrementor:
    case AccessibilityRole::ComboBox:
    case AccessibilityRole::DisclosureTriangle:
    case AccessibilityRole::ImageMap:
    case AccessibilityRole::ListMarker:
    case AccessibilityRole::ListBoxOption:
    case AccessibilityRole::Tab:
    case AccessibilityRole::DocumentMath:
    case AccessibilityRole::HorizontalRule:
    case AccessibilityRole::SliderThumb:
    case AccessibilityRole::Switch:
    case AccessibilityRole::SearchField:
    case AccessibilityRole::SpinButton:
        return true;
    case AccessibilityRole::StaticText: {
        // Many text elements only contain a space.
        if (![[[self accessibilityLabel] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length])
            return false;

        // Text elements that are just pieces of links or headers should not be exposed.
        if ([AccessibilityUnignoredAncestor([self accessibilityContainer]) containsUnnaturallySegmentedChildren])
            return false;
        return true;
    }
        
    // Don't expose headers as elements; instead expose their children as elements, with the header trait (unless they have no children)
    case AccessibilityRole::Heading:
        if (![self accessibilityElementCount])
            return true;
        return false;
        
    case AccessibilityRole::Video:
        return [self accessibilityIsWebInteractiveVideo];
        
    // Links can sometimes be elements (when they only contain static text or don't contain anything).
    // They should not be elements when containing text and other types.
    case AccessibilityRole::WebCoreLink:
    case AccessibilityRole::Link:
        if ([self containsUnnaturallySegmentedChildren] || ![self accessibilityElementCount])
            return true;
        return false;
    case AccessibilityRole::Group:
        if ([self isSVGGroupElement])
            return true;
        FALLTHROUGH;
    // All other elements are ignored on the iphone.
    case AccessibilityRole::Annotation:
    case AccessibilityRole::Application:
    case AccessibilityRole::ApplicationAlert:
    case AccessibilityRole::ApplicationAlertDialog:
    case AccessibilityRole::ApplicationDialog:
    case AccessibilityRole::ApplicationGroup:
    case AccessibilityRole::ApplicationLog:
    case AccessibilityRole::ApplicationMarquee:
    case AccessibilityRole::ApplicationStatus:
    case AccessibilityRole::ApplicationTextGroup:
    case AccessibilityRole::ApplicationTimer:
    case AccessibilityRole::Audio:
    case AccessibilityRole::Blockquote:
    case AccessibilityRole::Browser:
    case AccessibilityRole::BusyIndicator:
    case AccessibilityRole::Canvas:
    case AccessibilityRole::Caption:
    case AccessibilityRole::Cell:
    case AccessibilityRole::Column:
    case AccessibilityRole::ColumnHeader:
    case AccessibilityRole::Definition:
    case AccessibilityRole::DescriptionList:
    case AccessibilityRole::DescriptionListTerm:
    case AccessibilityRole::DescriptionListDetail:
    case AccessibilityRole::Details:
    case AccessibilityRole::Directory:
    case AccessibilityRole::Div:
    case AccessibilityRole::Document:
    case AccessibilityRole::DocumentArticle:
    case AccessibilityRole::DocumentNote:
    case AccessibilityRole::Drawer:
    case AccessibilityRole::EditableText:
    case AccessibilityRole::Feed:
    case AccessibilityRole::Figure:
    case AccessibilityRole::Footer:
    case AccessibilityRole::Footnote:
    case AccessibilityRole::Form:
    case AccessibilityRole::GraphicsDocument:
    case AccessibilityRole::GraphicsObject:
    case AccessibilityRole::GraphicsSymbol:
    case AccessibilityRole::Grid:
    case AccessibilityRole::GridCell:
    case AccessibilityRole::GrowArea:
    case AccessibilityRole::HelpTag:
    case AccessibilityRole::Ignored:
    case AccessibilityRole::Inline:
    case AccessibilityRole::Label:
    case AccessibilityRole::LandmarkBanner:
    case AccessibilityRole::LandmarkComplementary:
    case AccessibilityRole::LandmarkContentInfo:
    case AccessibilityRole::LandmarkDocRegion:
    case AccessibilityRole::LandmarkMain:
    case AccessibilityRole::LandmarkNavigation:
    case AccessibilityRole::LandmarkRegion:
    case AccessibilityRole::LandmarkSearch:
    case AccessibilityRole::Legend:
    case AccessibilityRole::List:
    case AccessibilityRole::ListBox:
    case AccessibilityRole::ListItem:
    case AccessibilityRole::Mark:
    case AccessibilityRole::MathElement:
    case AccessibilityRole::Matte:
    case AccessibilityRole::Menu:
    case AccessibilityRole::MenuBar:
    case AccessibilityRole::MenuListPopup:
    case AccessibilityRole::MenuListOption:
    case AccessibilityRole::Outline:
    case AccessibilityRole::Paragraph:
    case AccessibilityRole::Pre:
    case AccessibilityRole::Presentational:
    case AccessibilityRole::RadioGroup:
    case AccessibilityRole::RowHeader:
    case AccessibilityRole::Row:
    case AccessibilityRole::RubyBase:
    case AccessibilityRole::RubyBlock:
    case AccessibilityRole::RubyInline:
    case AccessibilityRole::RubyRun:
    case AccessibilityRole::RubyText:
    case AccessibilityRole::Ruler:
    case AccessibilityRole::RulerMarker:
    case AccessibilityRole::ScrollArea:
    case AccessibilityRole::ScrollBar:
    case AccessibilityRole::Sheet:
    case AccessibilityRole::SpinButtonPart:
    case AccessibilityRole::SplitGroup:
    case AccessibilityRole::Splitter:
    case AccessibilityRole::Summary:
    case AccessibilityRole::SystemWide:
    case AccessibilityRole::SVGRoot:
    case AccessibilityRole::SVGTextPath:
    case AccessibilityRole::SVGText:
    case AccessibilityRole::SVGTSpan:
    case AccessibilityRole::TabGroup:
    case AccessibilityRole::TabList:
    case AccessibilityRole::TabPanel:
    case AccessibilityRole::Table:
    case AccessibilityRole::TableHeaderContainer:
    case AccessibilityRole::Term:
    case AccessibilityRole::TextGroup:
    case AccessibilityRole::Time:
    case AccessibilityRole::Tree:
    case AccessibilityRole::TreeItem:
    case AccessibilityRole::TreeGrid:
    case AccessibilityRole::Toolbar:
    case AccessibilityRole::Unknown:
    case AccessibilityRole::UserInterfaceTooltip:
    case AccessibilityRole::WebApplication:
    case AccessibilityRole::WebArea:
    case AccessibilityRole::Window:
    case AccessibilityRole::RowGroup:
        return false;
    }
    
    ASSERT_NOT_REACHED();
    return false;
}

- (BOOL)isAccessibilityElement
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    if (m_isAccessibilityElement == -1)
        m_isAccessibilityElement = [self determineIsAccessibilityElement];
    
    return m_isAccessibilityElement;
}

- (BOOL)stringValueShouldBeUsedInLabel
{
    if (m_object->isTextControl())
        return NO;
    if (m_object->roleValue() == AccessibilityRole::PopUpButton)
        return NO;
    if (m_object->isFileUploadButton())
        return NO;
    if ([self accessibilityIsWebInteractiveVideo])
        return NO;

    return YES;
}

- (BOOL)fileUploadButtonReturnsValueInTitle
{
    return NO;
}

static void appendStringToResult(NSMutableString *result, NSString *string)
{
    ASSERT(result);
    if (![string length])
        return;
    if ([result length])
        [result appendString:@", "];
    [result appendString:string];
}

- (BOOL)_accessibilityHasTouchEventListener
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->hasTouchEventListener();
}

- (BOOL)_accessibilityValueIsAutofilled
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->isValueAutofilled();
}

- (BOOL)_accessibilityIsStrongPasswordField
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    if (!m_object->isPasswordField())
        return NO;
    
    return m_object->valueAutofillButtonType() == AutoFillButtonType::StrongPassword;
}

- (CGFloat)_accessibilityMinValue
{
    if (![self _prepareAccessibilityCall])
        return 0;
    
    return m_object->minValueForRange();
}

- (CGFloat)_accessibilityMaxValue
{
    if (![self _prepareAccessibilityCall])
        return 0;
    
    return m_object->maxValueForRange();
}

- (NSString *)accessibilityRoleDescription
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (m_object->isColorWell())
        return AXColorWellText();

    return m_object->roleDescription();
}

- (NSString *)accessibilityLabel
{
    if (![self _prepareAccessibilityCall])
        return nil;

    // check if the label was overridden
    NSString *label = [super accessibilityLabel];
    if (label)
        return label;

    // iOS doesn't distinguish between a title and description field,
    // so concatentation will yield the best result.
    NSString *axTitle = [self baseAccessibilityTitle];
    NSString *axDescription = [self baseAccessibilityDescription];
    NSString *landmarkDescription = [self ariaLandmarkRoleDescription];
    NSString *interactiveVideoDescription = [self interactiveVideoDescription];
    
    // We should expose the value of the input type date or time through AXValue instead of AXTitle.
    if (m_object->isInputTypePopupButton() && [axTitle isEqualToString:[self accessibilityValue]])
        axTitle = nil;

    // Footer is not considered a landmark, but we want the role description.
    if (m_object->roleValue() == AccessibilityRole::Footer)
        landmarkDescription = AXFooterRoleDescriptionText();

    NSMutableString *result = [NSMutableString string];
    if (m_object->roleValue() == AccessibilityRole::HorizontalRule)
        appendStringToResult(result, AXHorizontalRuleDescriptionText());

    appendStringToResult(result, axTitle);
    appendStringToResult(result, axDescription);
    if ([self stringValueShouldBeUsedInLabel]) {
        NSString *valueLabel = m_object->stringValue();
        valueLabel = [valueLabel stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
        appendStringToResult(result, valueLabel);
    }
    appendStringToResult(result, landmarkDescription);
    appendStringToResult(result, interactiveVideoDescription);
    
    return [result length] ? result : nil;
}

- (AccessibilityTableCell*)tableCellParent
{
    // Find if this element is in a table cell.
    if (AccessibilityObject* parent = const_cast<AccessibilityObject*>(AccessibilityObject::matchedParent(*m_object, true, [] (const AccessibilityObject& object) {
        return object.isTableCell();
    })))
        return static_cast<AccessibilityTableCell*>(parent);
    return nil;
}

- (AccessibilityTable*)tableParent
{
    // Find if the parent table for the table cell.
    if (AccessibilityObject* parent = const_cast<AccessibilityObject*>(AccessibilityObject::matchedParent(*m_object, true, [] (const AccessibilityObject& object) {
        return is<AccessibilityTable>(object) && downcast<AccessibilityTable>(object).isExposableThroughAccessibility();
    })))
        return static_cast<AccessibilityTable*>(parent);
    return nil;
}

- (id)accessibilityTitleElement
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityObject* titleElement = m_object->titleUIElement();
    if (titleElement)
        return titleElement->wrapper();

    return nil;
}

// Meant to return row or column headers (or other things as the future permits).
- (NSArray *)accessibilityHeaderElements
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return nil;
    
    AccessibilityTable* table = [self tableParent];
    if (!table)
        return nil;
    
    // Get the row and column range, so we can use them to find the headers.
    std::pair<unsigned, unsigned> rowRange;
    std::pair<unsigned, unsigned> columnRange;
    tableCell->rowIndexRange(rowRange);
    tableCell->columnIndexRange(columnRange);
    
    AccessibilityObject::AccessibilityChildrenVector rowHeaders;
    AccessibilityObject::AccessibilityChildrenVector columnHeaders;
    table->rowHeaders(rowHeaders);
    table->columnHeaders(columnHeaders);
    
    NSMutableArray *headers = [NSMutableArray array];
    
    unsigned columnRangeIndex = static_cast<unsigned>(columnRange.first);
    if (columnRangeIndex < columnHeaders.size()) {
        RefPtr<AccessibilityObject> columnHeader = columnHeaders[columnRange.first];
        AccessibilityObjectWrapper* wrapper = columnHeader->wrapper();
        if (wrapper)
            [headers addObject:wrapper];
    }

    unsigned rowRangeIndex = static_cast<unsigned>(rowRange.first);
    // We should consider the cases where the row number does NOT match the index in
    // rowHeaders, the most common case is when row0/col0 does not have a header.
    for (const auto& rowHeader : rowHeaders) {
        if (!is<AccessibilityTableCell>(*rowHeader))
            break;
        std::pair<unsigned, unsigned> rowHeaderRange;
        downcast<AccessibilityTableCell>(*rowHeader).rowIndexRange(rowHeaderRange);
        if (rowRangeIndex >= rowHeaderRange.first && rowRangeIndex < rowHeaderRange.first + rowHeaderRange.second) {
            if (AccessibilityObjectWrapper* wrapper = rowHeader->wrapper())
                [headers addObject:wrapper];
            break;
        }
    }

    return headers;
}

- (id)accessibilityElementForRow:(NSInteger)row andColumn:(NSInteger)column
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityTable* table = [self tableParent];
    if (!table)
        return nil;

    AccessibilityTableCell* cell = table->cellForColumnAndRow(column, row);
    if (!cell)
        return nil;
    return cell->wrapper();
}

- (NSUInteger)accessibilityRowCount
{
    if (![self _prepareAccessibilityCall])
        return 0;
    AccessibilityTable *table = [self tableParent];
    if (!table)
        return 0;
    
    return table->rowCount();
}

- (NSUInteger)accessibilityColumnCount
{
    if (![self _prepareAccessibilityCall])
        return 0;
    AccessibilityTable *table = [self tableParent];
    if (!table)
        return 0;
    
    return table->columnCount();
}

- (NSUInteger)accessibilityARIARowCount
{
    if (![self _prepareAccessibilityCall])
        return 0;
    AccessibilityTable *table = [self tableParent];
    if (!table)
        return 0;
    
    NSInteger rowCount = table->axRowCount();
    return rowCount > 0 ? rowCount : 0;
}

- (NSUInteger)accessibilityARIAColumnCount
{
    if (![self _prepareAccessibilityCall])
        return 0;
    AccessibilityTable *table = [self tableParent];
    if (!table)
        return 0;
    
    NSInteger colCount = table->axColumnCount();
    return colCount > 0 ? colCount : 0;
}

- (NSUInteger)accessibilityARIARowIndex
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;
    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSNotFound;
    
    NSInteger rowIndex = tableCell->axRowIndex();
    return rowIndex > 0 ? rowIndex : NSNotFound;
}

- (NSUInteger)accessibilityARIAColumnIndex
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;
    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSNotFound;
    
    NSInteger columnIndex = tableCell->axColumnIndex();
    return columnIndex > 0 ? columnIndex : NSNotFound;
}

- (NSRange)accessibilityRowRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    if (m_object->isRadioButton()) {
        AccessibilityObject::AccessibilityChildrenVector radioButtonSiblings;
        m_object->linkedUIElements(radioButtonSiblings);
        if (radioButtonSiblings.size() <= 1)
            return NSMakeRange(NSNotFound, 0);
        
        return NSMakeRange(radioButtonSiblings.find(m_object), radioButtonSiblings.size());
    }
    
    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSMakeRange(NSNotFound, 0);
    
    std::pair<unsigned, unsigned> rowRange;
    tableCell->rowIndexRange(rowRange);
    return NSMakeRange(rowRange.first, rowRange.second);
}

- (NSRange)accessibilityColumnRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSMakeRange(NSNotFound, 0);
    
    std::pair<unsigned, unsigned> columnRange;
    tableCell->columnIndexRange(columnRange);
    return NSMakeRange(columnRange.first, columnRange.second);
}

- (NSUInteger)accessibilityBlockquoteLevel
{
    if (![self _prepareAccessibilityCall])
        return 0;
    return m_object->blockquoteLevel();
}

- (NSString *)accessibilityDatetimeValue
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    if (auto parent = AccessibilityObject::matchedParent(*m_object, true, [] (const AccessibilityObject& object) { return object.supportsDatetimeAttribute(); }))
        return parent->datetimeAttributeValue();

    return nil;
}

- (NSString *)accessibilityPlaceholderValue
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->placeholderValue();
}

- (NSString *)accessibilityColorStringValue
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (m_object->isColorWell()) {
        int r, g, b;
        m_object->colorValue(r, g, b);
        return [NSString stringWithFormat:@"rgb %7.5f %7.5f %7.5f 1", r / 255., g / 255., b / 255.];
    }

    return nil;
}

- (NSString *)accessibilityValue
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // check if the value was overridden
    NSString *value = [super accessibilityValue];
    if (value)
        return value;
    
    AccessibilityRole role = m_object->roleValue();
    if (m_object->isCheckboxOrRadio() || role == AccessibilityRole::MenuItemCheckbox || role == AccessibilityRole::MenuItemRadio || role == AccessibilityRole::Switch) {
        switch (m_object->checkboxOrRadioValue()) {
        case AccessibilityButtonState::Off:
            return [NSString stringWithFormat:@"%d", 0];
        case AccessibilityButtonState::On:
            return [NSString stringWithFormat:@"%d", 1];
        case AccessibilityButtonState::Mixed:
            return [NSString stringWithFormat:@"%d", 2];
        }
        ASSERT_NOT_REACHED();
        return [NSString stringWithFormat:@"%d", 0];
    }
    
    if (m_object->isButton() && m_object->isPressed())
        return [NSString stringWithFormat:@"%d", 1];

    // rdar://8131388 WebKit should expose the same info as UIKit for its password fields.
    if (m_object->isPasswordField() && ![self _accessibilityIsStrongPasswordField]) {
        int passwordLength = m_object->accessibilityPasswordFieldLength();
        NSMutableString* string = [NSMutableString string];
        for (int k = 0; k < passwordLength; ++k)
            [string appendString:@"•"];
        return string;
    }
    
    // A text control should return its text data as the axValue (per iPhone AX API).
    if (![self stringValueShouldBeUsedInLabel])
        return m_object->stringValue();
    
    if (m_object->isRangeControl()) {
        // Prefer a valueDescription if provided by the author (through aria-valuetext).
        String valueDescription = m_object->valueDescription();
        if (!valueDescription.isEmpty())
            return valueDescription;

        return [NSString stringWithFormat:@"%.2f", m_object->valueForRange()];
    }

    if (is<AccessibilityAttachment>(m_object) && downcast<AccessibilityAttachment>(m_object)->hasProgress())
        return [NSString stringWithFormat:@"%.2f", m_object->valueForRange()];
    
    if (m_object->isHeading())
        return [NSString stringWithFormat:@"%d", m_object->headingLevel()];
    
    return nil;
}

- (BOOL)accessibilityIsAttachmentElement
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return is<AccessibilityAttachment>(m_object);
}

- (BOOL)accessibilityIsComboBox
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->roleValue() == AccessibilityRole::ComboBox;
}

- (NSString *)accessibilityHint
{
    if (![self _prepareAccessibilityCall])
        return nil;

    NSMutableString *result = [NSMutableString string];
    appendStringToResult(result, [self baseAccessibilityHelpText]);
    
    if ([self accessibilityIsShowingValidationMessage])
        appendStringToResult(result, m_object->validationMessage());
    
    return result;
}

- (NSURL *)accessibilityURL
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    URL url = m_object->url();
    if (url.isNull())
        return nil;
    return (NSURL*)url;
}

- (CGPoint)_accessibilityConvertPointToViewSpace:(CGPoint)point
{
    if (![self _prepareAccessibilityCall])
        return point;
    
    auto floatPoint = FloatPoint(point);
    auto floatRect = FloatRect(floatPoint, FloatSize());
    return [self convertRectToSpace:floatRect space:ScreenSpace].origin;
}

- (BOOL)accessibilityPerformEscape
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    m_object->dispatchAccessibilityEventWithType(AccessibilityEventType::Dismiss);
    
    // Return whether a listener received this event so it prevents other callers up the hierarchy chain.
    return m_object->shouldDispatchAccessibilityEvent() && m_object->hasAccessibleDismissEventListener();
}

- (BOOL)_accessibilityScrollToVisible
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    m_object->scrollToMakeVisible();
    return YES;
}


- (BOOL)accessibilityScroll:(UIAccessibilityScrollDirection)direction
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    AccessibilityObject::ScrollByPageDirection scrollDirection;
    switch (direction) {
    case UIAccessibilityScrollDirectionRight:
        scrollDirection = AccessibilityObject::ScrollByPageDirection::Right;
        break;
    case UIAccessibilityScrollDirectionLeft:
        scrollDirection = AccessibilityObject::ScrollByPageDirection::Left;
        break;
    case UIAccessibilityScrollDirectionUp:
        scrollDirection = AccessibilityObject::ScrollByPageDirection::Up;
        break;
    case UIAccessibilityScrollDirectionDown:
        scrollDirection = AccessibilityObject::ScrollByPageDirection::Down;
        break;
    default:
        return NO;
    }

    BOOL result = m_object->scrollByPage(scrollDirection);
    
    if (result) {
        [self postScrollStatusChangeNotification];

        CGPoint scrollPos = [self _accessibilityScrollPosition];
        NSString *testString = [NSString stringWithFormat:@"AXScroll [position: %.2f %.2f]", scrollPos.x, scrollPos.y];
        [self accessibilityPostedNotification:@"AXScrollByPage" userInfo:@{ @"status" : testString }];
    }
    
    // This means that this object handled the scroll and no other ancestor should attempt scrolling.
    return result;
}

- (CGRect)_accessibilityRelativeFrame
{
    auto rect = FloatRect(snappedIntRect(m_object->elementRect()));
    return [self convertRectToSpace:rect space:PageSpace];
}

// Used by UIKit accessibility bundle to help determine distance during a hit-test.
- (CGRect)accessibilityElementRect
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    LayoutRect rect = m_object->elementRect();
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

- (CGRect)accessibilityVisibleContentRect
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    auto document = m_object->document();
    if (!document || !document->view())
        return CGRectZero;
    auto rect = FloatRect(snappedIntRect(document->view()->unobscuredContentRect()));
    return [self convertRectToSpace:rect space:ScreenSpace];
}

// The "center point" is where VoiceOver will "press" an object. This may not be the actual
// center of the accessibilityFrame
- (CGPoint)accessibilityActivationPoint
{
    if (![self _prepareAccessibilityCall])
        return CGPointZero;
    
    auto rect = FloatRect(snappedIntRect(m_object->boundingBoxRect()));
    CGRect cgRect = [self convertRectToSpace:rect space:ScreenSpace];
    return CGPointMake(CGRectGetMidX(cgRect), CGRectGetMidY(cgRect));
}

- (CGRect)accessibilityFrame
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    auto rect = FloatRect(snappedIntRect(m_object->elementRect()));
    return [self convertRectToSpace:rect space:ScreenSpace];
}

// Checks whether a link contains only static text and images (and has been divided unnaturally by <spans> and other nefarious mechanisms).
- (BOOL)containsUnnaturallySegmentedChildren
{
    if (!m_object)
        return NO;
    
    AccessibilityRole role = m_object->roleValue();
    if (role != AccessibilityRole::Link && role != AccessibilityRole::WebCoreLink)
        return NO;
    
    const auto& children = m_object->children();
    unsigned childrenSize = children.size();

    // If there's only one child, then it doesn't have segmented children. 
    if (childrenSize == 1)
        return NO;
    
    for (unsigned i = 0; i < childrenSize; ++i) {
        AccessibilityRole role = children[i]->roleValue();
        if (role != AccessibilityRole::StaticText && role != AccessibilityRole::Image && role != AccessibilityRole::Group && role != AccessibilityRole::TextGroup)
            return NO;
    }
    
    return YES;
}

- (id)accessibilityContainer
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
    
    // As long as there's a parent wrapper, that's the correct chain to climb.
    AccessibilityObject* parent = m_object->parentObjectUnignored(); 
    if (parent)
        return parent->wrapper();

    // Mock objects can have their parents detached but still exist in the cache.
    if (m_object->isDetachedFromParent())
        return nil;
    
    // The only object without a parent wrapper at this point should be a scroll view.
    ASSERT(m_object->isAccessibilityScrollView());
    
    // Verify this is the top document. If not, we might need to go through the platform widget.
    FrameView* frameView = m_object->documentFrameView();
    Document* document = m_object->document();
    if (document && frameView && document != &document->topDocument())
        return frameView->platformWidget();
    
    // The top scroll view's parent is the web document view.
    return [self _accessibilityWebDocumentView];
}

- (id)accessibilityFocusedUIElement
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AccessibilityObject* focusedObj = m_object->focusedUIElement();
    
    if (!focusedObj)
        return nil;
    
    return focusedObj->wrapper();
}

- (id)_accessibilityWebDocumentView
{
    if (![self _prepareAccessibilityCall])
        return nil;

    // This method performs the crucial task of connecting to the UIWebDocumentView.
    // This is needed to correctly calculate the screen position of the AX object.
    static Class webViewClass = nil;
    if (!webViewClass)
        webViewClass = NSClassFromString(@"WebView");

    if (!webViewClass)
        return nil;
    
    FrameView* frameView = m_object->documentFrameView();

    if (!frameView)
        return nil;
    
    // If this is the top level frame, the UIWebDocumentView should be returned.
    id parentView = frameView->documentView();
    while (parentView && ![parentView isKindOfClass:webViewClass])
        parentView = [parentView superview];
    
    // The parentView should have an accessibilityContainer, if the UIKit accessibility bundle was loaded. 
    // The exception is DRT, which tests accessibility without the entire system turning accessibility on. Hence,
    // this check should be valid for everything except DRT.
    ASSERT([parentView accessibilityContainer] || IOSApplication::isDumpRenderTree());
    
    return [parentView accessibilityContainer];
}

- (NSArray *)_accessibilityNextElementsWithCount:(UInt32)count
{    
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [[self _accessibilityWebDocumentView] _accessibilityNextElementsWithCount:count];
}

- (NSArray *)_accessibilityPreviousElementsWithCount:(UInt32)count
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [[self _accessibilityWebDocumentView] _accessibilityPreviousElementsWithCount:count];
}

- (BOOL)accessibilityCanSetValue
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->canSetValueAttribute();
}

- (NSString *)accessibilityLinkRelationshipType
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->linkRelValue();
}

- (BOOL)accessibilityRequired
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->isRequired();
}

- (NSArray *)accessibilityFlowToElements
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AccessibilityObject::AccessibilityChildrenVector children;
    m_object->ariaFlowToElements(children);
    
    unsigned length = children.size();
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:length];
    for (unsigned i = 0; i < length; ++i) {
        AccessibilityObjectWrapper* wrapper = children[i]->wrapper();
        ASSERT(wrapper);
        if (!wrapper)
            continue;

        if (children[i]->isAttachment() && [wrapper attachmentView])
            [array addObject:[wrapper attachmentView]];
        else
            [array addObject:wrapper];
    }
    return array;
}

- (id)accessibilityLinkedElement
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // If this static text inside of a link, it should use its parent's linked element.
    AccessibilityObject* element = m_object;
    if (m_object->roleValue() == AccessibilityRole::StaticText && m_object->parentObjectUnignored()->isLink())
        element = m_object->parentObjectUnignored();
    
    AccessibilityObject::AccessibilityChildrenVector children;
    element->linkedUIElements(children);
    if (children.size() == 0)
        return nil;
    
    return children[0]->wrapper();
}


- (BOOL)isAttachment
{
    if (!m_object)
        return NO;
    
    return m_object->isAttachment();
}

- (NSString *)accessibilityTextualContext
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (m_object->node() && m_object->node()->hasTagName(codeTag))
        return UIAccessibilityTextualContextSourceCode;
    
    return nil;
}

- (void)_accessibilityActivate
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->press();
}

- (id)attachmentView
{
    if (![self _prepareAccessibilityCall])
        return nil;

    ASSERT([self isAttachment]);
    Widget* widget = m_object->widgetForAttachmentView();
    if (!widget)
        return nil;
    return widget->platformWidget();    
}

static RenderObject* rendererForView(WAKView* view)
{
    if (![view conformsToProtocol:@protocol(WebCoreFrameView)])
        return nil;
    
    WAKView<WebCoreFrameView>* frameView = (WAKView<WebCoreFrameView>*)view;
    Frame* frame = [frameView _web_frame];
    if (!frame)
        return nil;
    
    Node* node = frame->document()->ownerElement();
    if (!node)
        return nil;
    
    return node->renderer();
}

- (id)_accessibilityParentForSubview:(id)subview
{   
    RenderObject* renderer = rendererForView(subview);
    if (!renderer)
        return nil;
    
    AccessibilityObject* obj = renderer->document().axObjectCache()->getOrCreate(renderer);
    if (obj)
        return obj->parentObjectUnignored()->wrapper();
    return nil;
}

- (void)postFocusChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.
}

- (void)postSelectedTextChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postLayoutChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.        
}

- (void)postLiveRegionChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postLiveRegionCreatedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postLoadCompleteNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postChildrenChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postInvalidStatusChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.        
}

- (void)postValueChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.
}

- (void)postExpandedChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.
}

- (void)postScrollStatusChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.
}

// These will be used by the UIKit wrapper to calculate an appropriate description of scroll status.
- (CGPoint)_accessibilityScrollPosition
{
    if (![self _prepareAccessibilityCall])
        return CGPointZero;
    
    return m_object->scrollPosition();
}

- (CGSize)_accessibilityScrollSize
{
    if (![self _prepareAccessibilityCall])
        return CGSizeZero;
    
    return m_object->scrollContentsSize();
}

- (CGRect)_accessibilityScrollVisibleRect
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    return m_object->scrollVisibleContentRect();
}

- (AccessibilityObject*)detailParentForSummaryObject:(AccessibilityObject*)object
{
    // Use this to check if an object is the child of a summary object.
    // And return the summary's parent, which is the expandable details object.
    if (const AccessibilityObject* summary = AccessibilityObject::matchedParent(*object, true, [] (const AccessibilityObject& object) {
        return object.hasTagName(summaryTag);
    }))
        return summary->parentObject();
    return nil;
}

- (AccessibilityObject*)detailParentForObject:(AccessibilityObject*)object
{
    // Use this to check if an object is inside a details object.
    if (const AccessibilityObject* details = AccessibilityObject::matchedParent(*object, true, [] (const AccessibilityObject& object) {
        return object.hasTagName(detailsTag);
    }))
        return const_cast<AccessibilityObject*>(details);
    return nil;
}

- (AccessibilityObject*)treeItemParentForObject:(AccessibilityObject*)object
{
    // Use this to check if an object is inside a treeitem object.
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*object, true, [] (const AccessibilityObject& object) {
        return object.roleValue() == AccessibilityRole::TreeItem;
    }))
        return const_cast<AccessibilityObject*>(parent);
    return nil;
}

- (NSArray<WebAccessibilityObjectWrapper *> *)accessibilityFindMatchingObjects:(NSDictionary *)parameters
{
    AccessibilitySearchCriteria criteria = accessibilitySearchCriteriaForSearchPredicateParameterizedAttribute(parameters);
    AccessibilityObject::AccessibilityChildrenVector results;
    m_object->findMatchingObjects(&criteria, results);
    return convertToNSArray(results);
}

- (void)accessibilityElementDidBecomeFocused
{
    if (![self _prepareAccessibilityCall])
        return;
    
    m_object->dispatchAccessibilityEventWithType(AccessibilityEventType::Focus);
}

- (void)accessibilityModifySelection:(TextGranularity)granularity increase:(BOOL)increase
{
    if (![self _prepareAccessibilityCall])
        return;
    
    FrameSelection& frameSelection = m_object->document()->frame()->selection();
    VisibleSelection selection = m_object->selection();
    VisiblePositionRange range = m_object->visiblePositionRange();
    
    // Before a selection with length exists, the cursor position needs to move to the right starting place.
    // That should be the beginning of this element (range.start). However, if the cursor is already within the 
    // range of this element (the cursor is represented by selection), then the cursor does not need to move.
    if (frameSelection.isNone() && (selection.visibleStart() < range.start || selection.visibleEnd() > range.end))
        frameSelection.moveTo(range.start, UserTriggered);
    
    frameSelection.modify(FrameSelection::AlterationExtend, (increase) ? DirectionRight : DirectionLeft, granularity, UserTriggered);
}

- (void)accessibilityIncreaseSelection:(TextGranularity)granularity
{
    [self accessibilityModifySelection:granularity increase:YES];
}

- (void)accessibilityDecreaseSelection:(TextGranularity)granularity
{
    [self accessibilityModifySelection:granularity increase:NO];
}

- (void)accessibilityMoveSelectionToMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return;
    
    VisiblePosition visiblePosition = [marker visiblePosition];
    if (visiblePosition.isNull())
        return;

    FrameSelection& frameSelection = m_object->document()->frame()->selection();
    frameSelection.moveTo(visiblePosition, UserTriggered);
}

- (void)accessibilityIncrement
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->increment();
}

- (void)accessibilityDecrement
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->decrement();
}

#pragma mark Accessibility Text Marker Handlers

- (BOOL)_addAccessibilityObject:(AccessibilityObject*)axObject toTextMarkerArray:(NSMutableArray *)array
{
    if (!axObject)
        return NO;
    
    AccessibilityObjectWrapper* wrapper = axObject->wrapper();
    if (!wrapper)
        return NO;

    // Don't add the same object twice, but since this has already been added, we should return
    // YES because we want to inform that it's in the array
    if ([array containsObject:wrapper])
        return YES;
    
    // Explicity set that this is now an element (in case other logic tries to override).
    [wrapper setValue:@YES forKey:@"isAccessibilityElement"];    
    [array addObject:wrapper];
    return YES;
}

- (void)_accessibilitySetValue:(NSString *)string
{
    if (![self _prepareAccessibilityCall])
        return;
    m_object->setValue(string);
}

- (NSString *)stringForTextMarkers:(NSArray *)markers
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    RefPtr<Range> range = [self rangeForTextMarkers:markers];
    if (!range)
        return nil;
    
    return m_object->stringForRange(range);
}

static int blockquoteLevel(RenderObject* renderer)
{
    if (!renderer)
        return 0;
    
    int result = 0;
    for (Node* node = renderer->node(); node; node = node->parentNode()) {
        if (node->hasTagName(blockquoteTag))
            result += 1;
    }
    
    return result;
}

static void AXAttributeStringSetLanguage(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    if (!renderer)
        return;
    
    AccessibilityObject* axObject = renderer->document().axObjectCache()->getOrCreate(renderer);
    NSString *language = axObject->language();
    if ([language length])
        [attrString addAttribute:UIAccessibilityTokenLanguage value:language range:range];
    else
        [attrString removeAttribute:UIAccessibilityTokenLanguage range:range];
}

static void AXAttributeStringSetBlockquoteLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    int quoteLevel = blockquoteLevel(renderer);
    
    if (quoteLevel)
        [attrString addAttribute:UIAccessibilityTokenBlockquoteLevel value:[NSNumber numberWithInt:quoteLevel] range:range];
    else
        [attrString removeAttribute:UIAccessibilityTokenBlockquoteLevel range:range];
}

static void AXAttributeStringSetHeadingLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    if (!renderer)
        return;
    
    AccessibilityObject* parentObject = renderer->document().axObjectCache()->getOrCreate(renderer->parent());
    int parentHeadingLevel = parentObject->headingLevel();
    
    if (parentHeadingLevel)
        [attrString addAttribute:UIAccessibilityTokenHeadingLevel value:[NSNumber numberWithInt:parentHeadingLevel] range:range];
    else
        [attrString removeAttribute:UIAccessibilityTokenHeadingLevel range:range];
}

static void AXAttributeStringSetFont(NSMutableAttributedString* attrString, CTFontRef font, NSRange range)
{
    if (!font)
        return;
    
    RetainPtr<CFStringRef> fullName = adoptCF(CTFontCopyFullName(font));
    RetainPtr<CFStringRef> familyName = adoptCF(CTFontCopyFamilyName(font));

    NSNumber* size = [NSNumber numberWithFloat:CTFontGetSize(font)];
    CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(font);
    NSNumber* bold = [NSNumber numberWithBool:(traits & kCTFontTraitBold)];
    if (fullName)
        [attrString addAttribute:UIAccessibilityTokenFontName value:(NSString*)fullName.get() range:range];
    if (familyName)
        [attrString addAttribute:UIAccessibilityTokenFontFamily value:(NSString*)familyName.get() range:range];
    if ([size boolValue])
        [attrString addAttribute:UIAccessibilityTokenFontSize value:size range:range];
    if ([bold boolValue] || (traits & kCTFontTraitBold))
        [attrString addAttribute:UIAccessibilityTokenBold value:@YES range:range];
    if (traits & kCTFontTraitItalic)
        [attrString addAttribute:UIAccessibilityTokenItalic value:@YES range:range];

}

static void AXAttributeStringSetNumber(NSMutableAttributedString* attrString, NSString* attribute, NSNumber* number, NSRange range)
{
    if (number)
        [attrString addAttribute:attribute value:number range:range];
    else
        [attrString removeAttribute:attribute range:range];
}

static void AXAttributeStringSetStyle(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    auto& style = renderer->style();
    
    // set basic font info
    AXAttributeStringSetFont(attrString, style.fontCascade().primaryFont().getCTFont(), range);
                
    auto decor = style.textDecorationsInEffect();
    if (decor & TextDecoration::Underline)
        AXAttributeStringSetNumber(attrString, UIAccessibilityTokenUnderline, @YES, range);

    // Add code context if this node is within a <code> block.
    AccessibilityObject* axObject = renderer->document().axObjectCache()->getOrCreate(renderer);
    auto matchFunc = [] (const AccessibilityObject& object) {
        return object.node() && object.node()->hasTagName(codeTag);
    };
    
    if (const AccessibilityObject* parent = AccessibilityObject::matchedParent(*axObject, true, WTFMove(matchFunc)))
        [attrString addAttribute:UIAccessibilityTextAttributeContext value:UIAccessibilityTextualContextSourceCode range:range];
}

static void AXAttributedStringAppendText(NSMutableAttributedString* attrString, Node* node, NSString *text)
{
    // skip invisible text
    if (!node->renderer())
        return;
    
    // easier to calculate the range before appending the string
    NSRange attrStringRange = NSMakeRange([attrString length], [text length]);
    
    // append the string from this node
    [[attrString mutableString] appendString:text];
    
    // set new attributes
    AXAttributeStringSetStyle(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetHeadingLevel(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetBlockquoteLevel(attrString, node->renderer(), attrStringRange);    
    AXAttributeStringSetLanguage(attrString, node->renderer(), attrStringRange);
}


// This method is intended to return an array of strings and accessibility elements that 
// represent the objects on one line of rendered web content. The array of markers sent
// in should be ordered and contain only a start and end marker.
- (NSArray *)arrayOfTextForTextMarkers:(NSArray *)markers attributed:(BOOL)attributed
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if ([markers count] != 2)
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [markers objectAtIndex:0];
    WebAccessibilityTextMarker* endMarker = [markers objectAtIndex:1];
    if (![startMarker isKindOfClass:[WebAccessibilityTextMarker class]] || ![endMarker isKindOfClass:[WebAccessibilityTextMarker class]])
        return nil;
    
    // extract the start and end VisiblePosition
    VisiblePosition startVisiblePosition = [startMarker visiblePosition];
    if (startVisiblePosition.isNull())
        return nil;
    
    VisiblePosition endVisiblePosition = [endMarker visiblePosition];
    if (endVisiblePosition.isNull())
        return nil;
    
    // iterate over the range to build the AX attributed string
    NSMutableArray* array = [[NSMutableArray alloc] init];
    TextIterator it(makeRange(startVisiblePosition, endVisiblePosition).get());
    for (; !it.atEnd(); it.advance()) {
        // locate the node and starting offset for this range
        Node& node = it.range()->startContainer();
        ASSERT(&node == &it.range()->endContainer());
        int offset = it.range()->startOffset();
        
        // non-zero length means textual node, zero length means replaced node (AKA "attachments" in AX)
        if (it.text().length() != 0) {
            if (!attributed) {
                // First check if this is represented by a link.
                AccessibilityObject* linkObject = AccessibilityObject::anchorElementForNode(&node);
                if ([self _addAccessibilityObject:linkObject toTextMarkerArray:array])
                    continue;
                
                // Next check if this region is represented by a heading.
                AccessibilityObject* headingObject = AccessibilityObject::headingElementForNode(&node);
                if ([self _addAccessibilityObject:headingObject toTextMarkerArray:array])
                    continue;
                
                String listMarkerText = m_object->listMarkerTextForNodeAndPosition(&node, VisiblePosition(it.range()->startPosition()));
                
                if (!listMarkerText.isEmpty()) 
                    [array addObject:listMarkerText];
                // There was not an element representation, so just return the text.
                [array addObject:it.text().createNSString().get()];
            }
            else
            {
                String listMarkerText = m_object->listMarkerTextForNodeAndPosition(&node, VisiblePosition(it.range()->startPosition()));

                if (!listMarkerText.isEmpty()) {
                    NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] init];
                    AXAttributedStringAppendText(attrString, &node, listMarkerText);
                    [array addObject:attrString];
                    [attrString release];
                }
                
                NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
                AXAttributedStringAppendText(attrString, &node, it.text().createNSStringWithoutCopying().get());
                [array addObject:attrString];
                [attrString release];
            }
        } else {
            Node* replacedNode = node.traverseToChildAt(offset);
            if (replacedNode) {
                AccessibilityObject* obj = m_object->axObjectCache()->getOrCreate(replacedNode->renderer());
                if (obj && !obj->accessibilityIsIgnored())
                    [self _addAccessibilityObject:obj toTextMarkerArray:array];
            }
        }
    }
    
    return [array autorelease];
}

- (NSRange)_convertToNSRange:(Range *)range
{
    if (!range)
        return NSMakeRange(NSNotFound, 0);

    Document* document = m_object->document();
    Element* selectionRoot = document->frame()->selection().selection().rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : document->documentElement();

    // Mouse events may cause TSM to attempt to create an NSRange for a portion of the view
    // that is not inside the current editable region.  These checks ensure we don't produce
    // potentially invalid data when responding to such requests.
    if (&range->startContainer() != scope && !range->startContainer().isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    if (&range->endContainer() != scope && !range->endContainer().isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);

    auto testRange = Range::create(scope->document(), scope, 0, &range->startContainer(), range->startOffset());
    ASSERT(&testRange->startContainer() == scope);
    int startPosition = TextIterator::rangeLength(testRange.ptr());
    testRange->setEnd(range->endContainer(), range->endOffset());
    ASSERT(&testRange->startContainer() == scope);
    int endPosition = TextIterator::rangeLength(testRange.ptr());
    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (RefPtr<Range>)_convertToDOMRange:(NSRange)nsrange
{
    if (nsrange.location > INT_MAX)
        return nullptr;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;
        
    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    Document* document = m_object->document();
    Element* selectionRoot = document->frame()->selection().selection().rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : document->documentElement();
    return TextIterator::rangeFromLocationAndLength(scope, nsrange.location, nsrange.length);
}

// This method is intended to take a text marker representing a VisiblePosition and convert it
// into a normalized location within the document.
- (NSInteger)positionForTextMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;

    if (!marker)
        return NSNotFound;    

    if (AXObjectCache* cache = m_object->axObjectCache()) {
        CharacterOffset characterOffset = [marker characterOffset];
        // Create a collapsed range from the CharacterOffset object.
        RefPtr<Range> range = cache->rangeForUnorderedCharacterOffsets(characterOffset, characterOffset);
        NSRange nsRange = [self _convertToNSRange:range.get()];
        return nsRange.location;
    }
    return NSNotFound;
}

- (NSArray *)textMarkerRange
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    RefPtr<Range> range = m_object->elementRange();
    return [self textMarkersForRange:range];
}

// A method to get the normalized text cursor range of an element. Used in DumpRenderTree.
- (NSRange)elementTextRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    NSArray *markers = [self textMarkerRange];
    if ([markers count] != 2)
        return NSMakeRange(NSNotFound, 0);
    
    WebAccessibilityTextMarker *startMarker = [markers objectAtIndex:0];
    WebAccessibilityTextMarker *endMarker = [markers objectAtIndex:1];
    
    NSInteger startPosition = [self positionForTextMarker:startMarker];
    NSInteger endPosition = [self positionForTextMarker:endMarker];
    
    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (AccessibilityObjectWrapper *)accessibilityObjectForTextMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    AccessibilityObject* obj = [marker accessibilityObject];
    if (!obj)
        return nil;
    
    return AccessibilityUnignoredAncestor(obj->wrapper());
}

- (NSArray *)textMarkerRangeForSelection
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisibleSelection selection = m_object->selection();
    if (selection.isNone())
        return nil;
    
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    
    RefPtr<Range> range = selection.toNormalizedRange();
    if (!range)
        return nil;

    CharacterOffset start = cache->startOrEndCharacterOffsetForRange(range, true);
    CharacterOffset end = cache->startOrEndCharacterOffsetForRange(range, false);

    WebAccessibilityTextMarker* startMarker = [WebAccessibilityTextMarker textMarkerWithCharacterOffset:start cache:cache];
    WebAccessibilityTextMarker* endMarker = [WebAccessibilityTextMarker textMarkerWithCharacterOffset:end cache:cache];
    if (!startMarker || !endMarker)
        return nil;
    
    return [NSArray arrayWithObjects:startMarker, endMarker, nil];
}

- (WebAccessibilityTextMarker *)textMarkerForPosition:(NSInteger)position
{
    if (![self _prepareAccessibilityCall])
        return nil;

    RefPtr<Range> range = [self _convertToDOMRange:NSMakeRange(position, 0)];
    if (!range)
        return nil;

    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    
    CharacterOffset characterOffset = cache->startOrEndCharacterOffsetForRange(range, true);
    return [WebAccessibilityTextMarker textMarkerWithCharacterOffset:characterOffset cache:cache];
}

- (id)_stringFromStartMarker:(WebAccessibilityTextMarker*)startMarker toEndMarker:(WebAccessibilityTextMarker*)endMarker attributed:(BOOL)attributed
{
    if (!startMarker || !endMarker)
        return nil;
    
    NSArray* array = [self arrayOfTextForTextMarkers:[NSArray arrayWithObjects:startMarker, endMarker, nil] attributed:attributed];
    Class returnClass = attributed ? [NSMutableAttributedString class] : [NSMutableString class];
    id returnValue = [[(NSString *)[returnClass alloc] init] autorelease];
    
    const unichar attachmentChar = NSAttachmentCharacter;
    NSInteger count = [array count];
    for (NSInteger k = 0; k < count; ++k) {
        id object = [array objectAtIndex:k];

        if (attributed && [object isKindOfClass:[WebAccessibilityObjectWrapper class]])
            object = [[[NSMutableAttributedString alloc] initWithString:[NSString stringWithCharacters:&attachmentChar length:1] attributes:@{ UIAccessibilityTokenAttachment : object }] autorelease];
        
        if (![object isKindOfClass:returnClass])
            continue;
        
        if (attributed)
            [(NSMutableAttributedString *)returnValue appendAttributedString:object];
        else
            [(NSMutableString *)returnValue appendString:object];
    }
    return returnValue;
}

- (id)_stringForRange:(NSRange)range attributed:(BOOL)attributed
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [self textMarkerForPosition:range.location];
    WebAccessibilityTextMarker* endMarker = [self textMarkerForPosition:NSMaxRange(range)];
    
    // Clients don't always know the exact range, rather than force them to compute it,
    // allow clients to overshoot and use the max text marker range.
    if (!startMarker || !endMarker) {
        NSArray *markers = [self textMarkerRange];
        if ([markers count] != 2)
            return nil;
        if (!startMarker)
            startMarker = [markers objectAtIndex:0];
        if (!endMarker)
            endMarker = [markers objectAtIndex:1];
    }
    
    return [self _stringFromStartMarker:startMarker toEndMarker:endMarker attributed:attributed];
}

// A convenience method for getting the text of a NSRange.
- (NSString *)stringForRange:(NSRange)range
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->stringForRange([self _convertToDOMRange:range]);
}

- (NSAttributedString *)attributedStringForRange:(NSRange)range
{
    return [self _stringForRange:range attributed:YES];
}

- (NSAttributedString *)attributedStringForElement
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    NSArray *markers = [self textMarkerRange];
    if ([markers count] != 2)
        return nil;
    
    return [self _stringFromStartMarker:markers.firstObject toEndMarker:markers.lastObject attributed:YES];
}

- (NSRange)_accessibilitySelectedTextRange
{
    if (![self _prepareAccessibilityCall] || !m_object->isTextControl())
        return NSMakeRange(NSNotFound, 0);

    PlainTextRange textRange = m_object->selectedTextRange();
    if (textRange.isNull())
        return NSMakeRange(NSNotFound, 0);
    return NSMakeRange(textRange.start, textRange.length);
}

- (void)_accessibilitySetSelectedTextRange:(NSRange)range
{
    if (![self _prepareAccessibilityCall] || !m_object->isTextControl())
        return;
    
    m_object->setSelectedTextRange(PlainTextRange(range.location, range.length));
}

- (BOOL)accessibilityReplaceRange:(NSRange)range withText:(NSString *)string
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->replaceTextInRange(string, PlainTextRange(range));
}

// A convenience method for getting the accessibility objects of a NSRange. Currently used only by DRT.
- (NSArray *)elementsForRange:(NSRange)range
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [self textMarkerForPosition:range.location];
    WebAccessibilityTextMarker* endMarker = [self textMarkerForPosition:NSMaxRange(range)];
    if (!startMarker || !endMarker)
        return nil;
    
    NSArray* array = [self arrayOfTextForTextMarkers:[NSArray arrayWithObjects:startMarker, endMarker, nil] attributed:NO];
    NSMutableArray* elements = [NSMutableArray array];
    for (id element in array) {
        if (![element isKindOfClass:[AccessibilityObjectWrapper class]])
            continue;
        [elements addObject:element];
    }
    return elements;
}

- (NSString *)selectionRangeString
{
    NSArray *markers = [self textMarkerRangeForSelection];
    return [self stringForTextMarkers:markers];
}

- (WebAccessibilityTextMarker *)selectedTextMarker
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisibleSelection selection = m_object->selection();
    VisiblePosition position = selection.visibleStart();
    
    // if there's no selection, start at the top of the document
    if (position.isNull())
        position = startOfDocument(m_object->document());
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:position cache:m_object->axObjectCache()];
}

// This method is intended to return the marker at the end of the line starting at
// the marker that is passed into the method.
- (WebAccessibilityTextMarker *)lineEndMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition lineEnd = m_object->nextLineEndPosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:lineEnd cache:m_object->axObjectCache()];
}

// This method is intended to return the marker at the start of the line starting at
// the marker that is passed into the method.
- (WebAccessibilityTextMarker *)lineStartMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition lineStart = m_object->previousLineStartPosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:lineStart cache:m_object->axObjectCache()];
}

- (WebAccessibilityTextMarker *)nextMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    CharacterOffset start = [marker characterOffset];
    return [self nextMarkerForCharacterOffset:start];
}

- (WebAccessibilityTextMarker *)previousMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    CharacterOffset start = [marker characterOffset];
    return [self previousMarkerForCharacterOffset:start];
}

// This method is intended to return the bounds of a text marker range in screen coordinates.
- (CGRect)frameForTextMarkers:(NSArray *)array
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;

    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return CGRectZero;
    RefPtr<Range> range = [self rangeForTextMarkers:array];
    if (!range)
        return CGRectZero;
    
    auto rect = FloatRect(m_object->boundsForRange(range));
    return [self convertRectToSpace:rect space:ScreenSpace];
}

- (RefPtr<Range>)rangeFromMarkers:(NSArray *)markers withText:(NSString *)text
{
    RefPtr<Range> originalRange = [self rangeForTextMarkers:markers];
    if (!originalRange)
        return nil;
    
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    
    return cache->rangeMatchesTextNearRange(originalRange, text);
}

// This is only used in the layout test.
- (NSArray *)textMarkerRangeFromMarkers:(NSArray *)markers withText:(NSString *)text
{
    return [self textMarkersForRange:[self rangeFromMarkers:markers withText:text]];
}

- (NSArray *)textRectsFromMarkers:(NSArray *)markers withText:(NSString *)text
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    RefPtr<Range> range = [self rangeFromMarkers:markers withText:text];
    if (!range || range->collapsed())
        return nil;
    
    Vector<WebCore::SelectionRect> selectionRects;
    range->collectSelectionRectsWithoutUnionInteriorLines(selectionRects);
    return [self rectsForSelectionRects:selectionRects];
}

- (NSArray *)rectsForSelectionRects:(const Vector<WebCore::SelectionRect>&)selectionRects
{
    unsigned size = selectionRects.size();
    if (!size)
        return nil;
    
    NSMutableArray *rects = [NSMutableArray arrayWithCapacity:size];
    for (unsigned i = 0; i < size; i++) {
        const WebCore::SelectionRect& coreRect = selectionRects[i];
        auto selectionRect = FloatRect(coreRect.rect());
        CGRect rect = [self convertRectToSpace:selectionRect space:ScreenSpace];
        [rects addObject:[NSValue valueWithRect:rect]];
    }
    
    return rects;
}

- (WebAccessibilityTextMarker *)textMarkerForPoint:(CGPoint)point
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    CharacterOffset characterOffset = cache->characterOffsetForPoint(IntPoint(point), m_object);
    return [WebAccessibilityTextMarker textMarkerWithCharacterOffset:characterOffset cache:cache];
}

- (WebAccessibilityTextMarker *)nextMarkerForCharacterOffset:(CharacterOffset&)characterOffset
{
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    
    TextMarkerData textMarkerData;
    cache->textMarkerDataForNextCharacterOffset(textMarkerData, characterOffset);
    if (!textMarkerData.axID)
        return nil;
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData cache:cache] autorelease];
}

- (WebAccessibilityTextMarker *)previousMarkerForCharacterOffset:(CharacterOffset&)characterOffset
{
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nil;
    
    TextMarkerData textMarkerData;
    cache->textMarkerDataForPreviousCharacterOffset(textMarkerData, characterOffset);
    if (!textMarkerData.axID)
        return nil;
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData cache:cache] autorelease];
}

- (RefPtr<Range>)rangeForTextMarkers:(NSArray *)textMarkers
{
    if ([textMarkers count] != 2)
        return nullptr;
    
    WebAccessibilityTextMarker *startMarker = [textMarkers objectAtIndex:0];
    WebAccessibilityTextMarker *endMarker = [textMarkers objectAtIndex:1];
    
    if (![startMarker isKindOfClass:[WebAccessibilityTextMarker class]] || ![endMarker isKindOfClass:[WebAccessibilityTextMarker class]])
        return nullptr;
    
    AXObjectCache* cache = m_object->axObjectCache();
    if (!cache)
        return nullptr;
    
    CharacterOffset startCharacterOffset = [startMarker characterOffset];
    CharacterOffset endCharacterOffset = [endMarker characterOffset];
    return cache->rangeForUnorderedCharacterOffsets(startCharacterOffset, endCharacterOffset);
}

- (NSInteger)lengthForTextMarkers:(NSArray *)textMarkers
{
    if (![self _prepareAccessibilityCall])
        return 0;
    
    RefPtr<Range> range = [self rangeForTextMarkers:textMarkers];
    int length = AXObjectCache::lengthForRange(range.get());
    return length < 0 ? 0 : length;
}

- (WebAccessibilityTextMarker *)startOrEndTextMarkerForTextMarkers:(NSArray *)textMarkers isStart:(BOOL)isStart
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    RefPtr<Range> range = [self rangeForTextMarkers:textMarkers];
    if (!range)
        return nil;
    
    return [WebAccessibilityTextMarker startOrEndTextMarkerForRange:range isStart:isStart cache:m_object->axObjectCache()];
}

- (NSArray *)textMarkerRangeForMarkers:(NSArray *)textMarkers
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    RefPtr<Range> range = [self rangeForTextMarkers:textMarkers];
    return [self textMarkersForRange:range];
}

- (NSArray *)textMarkersForRange:(RefPtr<Range>)range
{
    if (!range)
        return nil;
    
    WebAccessibilityTextMarker* start = [WebAccessibilityTextMarker startOrEndTextMarkerForRange:range isStart:YES cache:m_object->axObjectCache()];
    WebAccessibilityTextMarker* end = [WebAccessibilityTextMarker startOrEndTextMarkerForRange:range isStart:NO cache:m_object->axObjectCache()];
    if (!start || !end)
        return nil;
    return [NSArray arrayWithObjects:start, end, nil];
}

- (NSString *)accessibilityExpandedTextValue
{
    if (![self _prepareAccessibilityCall])
        return nil;
    return m_object->expandedTextValue();
}

- (NSString *)accessibilityIdentifier
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->getAttribute(HTMLNames::idAttr);
}

- (NSArray<NSString *> *)accessibilitySpeechHint
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return [self baseAccessibilitySpeechHint];
}

- (BOOL)accessibilityARIAIsBusy
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->isBusy();
}

- (NSString *)accessibilityARIALiveRegionStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->liveRegionStatus();
}

- (NSString *)accessibilityARIARelevantStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->liveRegionRelevant();
}

- (BOOL)accessibilityARIALiveRegionIsAtomic
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->liveRegionAtomic();
}

- (BOOL)accessibilitySupportsARIAPressed
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->supportsPressed();
}

- (BOOL)accessibilityIsPressed
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->isPressed();
}

- (BOOL)accessibilitySupportsARIAExpanded
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    // Since details element is ignored on iOS, we should expose the expanded status on its
    // summary's accessible children.
    if (AccessibilityObject* detailParent = [self detailParentForSummaryObject:m_object])
        return detailParent->supportsExpanded();
    
    if (AccessibilityObject* treeItemParent = [self treeItemParentForObject:m_object])
        return treeItemParent->supportsExpanded();
    
    return m_object->supportsExpanded();
}

- (BOOL)accessibilityIsExpanded
{
    if (![self _prepareAccessibilityCall])
        return NO;

    // Since details element is ignored on iOS, we should expose the expanded status on its
    // summary's accessible children.
    if (AccessibilityObject* detailParent = [self detailParentForSummaryObject:m_object])
        return detailParent->isExpanded();
    
    if (AccessibilityObject* treeItemParent = [self treeItemParentForObject:m_object])
        return treeItemParent->isExpanded();
    
    return m_object->isExpanded();
}

- (BOOL)accessibilityIsShowingValidationMessage
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    return m_object->isShowingValidationMessage();
}

- (NSString *)accessibilityInvalidStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->invalidStatus();
}

- (NSString *)accessibilityARIACurrentStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    switch (m_object->currentState()) {
    case AccessibilityCurrentState::False:
        return @"false";
    case AccessibilityCurrentState::Page:
        return @"page";
    case AccessibilityCurrentState::Step:
        return @"step";
    case AccessibilityCurrentState::Location:
        return @"location";
    case AccessibilityCurrentState::Time:
        return @"time";
    case AccessibilityCurrentState::Date:
        return @"date";
    case AccessibilityCurrentState::True:
        return @"true";
    }
}

- (NSString *)accessibilitySortDirection
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    switch (m_object->sortDirection()) {
    case AccessibilitySortDirection::Ascending:
        return @"ascending";
    case AccessibilitySortDirection::Descending:
        return @"descending";
    case AccessibilitySortDirection::Other:
        return @"other";
    case AccessibilitySortDirection::Invalid:
    case AccessibilitySortDirection::None:
        return nil;
    }
}

- (WebAccessibilityObjectWrapper *)accessibilityMathRootIndexObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathRootIndexObject() ? m_object->mathRootIndexObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathRadicandObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathRadicandObject() ? m_object->mathRadicandObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathNumeratorObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathNumeratorObject() ? m_object->mathNumeratorObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathDenominatorObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathDenominatorObject() ? m_object->mathDenominatorObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathBaseObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathBaseObject() ? m_object->mathBaseObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathSubscriptObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathSubscriptObject() ? m_object->mathSubscriptObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathSuperscriptObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathSuperscriptObject() ? m_object->mathSuperscriptObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathUnderObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathUnderObject() ? m_object->mathUnderObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathOverObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathOverObject() ? m_object->mathOverObject()->wrapper() : 0;
}

- (NSString *)accessibilityPlatformMathSubscriptKey
{
    return @"AXMSubscriptObject";
}

- (NSString *)accessibilityPlatformMathSuperscriptKey
{
    return @"AXMSuperscriptObject";
}

- (NSArray *)accessibilityMathPostscripts
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [self accessibilityMathPostscriptPairs];
}

- (NSArray *)accessibilityMathPrescripts
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [self accessibilityMathPrescriptPairs];
}

- (NSString *)accessibilityMathFencedOpenString
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathFencedOpenString();
}

- (NSString *)accessibilityMathFencedCloseString
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathFencedCloseString();
}

- (BOOL)accessibilityIsMathTopObject
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->roleValue() == AccessibilityRole::DocumentMath;
}

- (NSInteger)accessibilityMathLineThickness
{
    if (![self _prepareAccessibilityCall])
        return 0;

    return m_object->mathLineThickness();
}

- (NSString *)accessibilityMathType
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (m_object->roleValue() == AccessibilityRole::MathElement) {
        if (m_object->isMathFraction())
            return @"AXMathFraction";
        if (m_object->isMathFenced())
            return @"AXMathFenced";
        if (m_object->isMathSubscriptSuperscript())
            return @"AXMathSubscriptSuperscript";
        if (m_object->isMathRow())
            return @"AXMathRow";
        if (m_object->isMathUnderOver())
            return @"AXMathUnderOver";
        if (m_object->isMathSquareRoot())
            return @"AXMathSquareRoot";
        if (m_object->isMathRoot())
            return @"AXMathRoot";
        if (m_object->isMathText())
            return @"AXMathText";
        if (m_object->isMathNumber())
            return @"AXMathNumber";
        if (m_object->isMathIdentifier())
            return @"AXMathIdentifier";
        if (m_object->isMathTable())
            return @"AXMathTable";
        if (m_object->isMathTableRow())
            return @"AXMathTableRow";
        if (m_object->isMathTableCell())
            return @"AXMathTableCell";
        if (m_object->isMathFenceOperator())
            return @"AXMathFenceOperator";
        if (m_object->isMathSeparatorOperator())
            return @"AXMathSeparatorOperator";
        if (m_object->isMathOperator())
            return @"AXMathOperator";
        if (m_object->isMathMultiscript())
            return @"AXMathMultiscript";
    }
    
    return nil;
}

- (CGPoint)accessibilityClickPoint
{
    return m_object->clickPoint();
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@: %@", [self class], [self accessibilityLabel]];
}

@end

#endif // HAVE(ACCESSIBILITY) && PLATFORM(IOS_FAMILY)
