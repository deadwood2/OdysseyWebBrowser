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

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE

NS_ASSUME_NONNULL_BEGIN

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
@class WebItemProviderRegistrationInfoList;
#endif

@protocol AbstractPasteboard <NSObject>
@required

@property (readonly, nonatomic) NSInteger numberOfItems;

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
@property (nonatomic, copy, nullable) NSArray<__kindof NSItemProvider *> *itemProviders;
#endif

- (NSArray<NSString *> *)pasteboardTypes;
- (NSData *)dataForPasteboardType:(NSString *)pasteboardType;
- (NSArray *)dataForPasteboardType:(NSString *)pasteboardType inItemSet:(NSIndexSet *)itemSet;
- (NSArray *)valuesForPasteboardType:(NSString *)pasteboardType inItemSet:(NSIndexSet *)itemSet;
- (NSInteger)changeCount;

@optional
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
- (void)stageRegistrationList:(nullable WebItemProviderRegistrationInfoList *)info;
- (nullable WebItemProviderRegistrationInfoList *)takeRegistrationList;
#endif
- (void)setItems:(NSArray<NSDictionary *> *)items;
- (NSArray<NSString *> *)pasteboardTypesByFidelityForItemAtIndex:(NSUInteger)index;
@property (readonly, nonatomic) NSInteger numberOfFiles;
@property (readonly, nonatomic) NSArray<NSURL *> *allDroppedFileURLs;
- (nullable NSURL *)preferredFileUploadURLAtIndex:(NSUInteger)index fileType:(NSString *_Nullable *_Nullable)outFileType;
- (void)updateSupportedTypeIdentifiers:(NSArray<NSString *> *)types;

@end

NS_ASSUME_NONNULL_END

#endif // TARGET_OS_IPHONE
