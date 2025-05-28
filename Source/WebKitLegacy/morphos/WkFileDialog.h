#import <ob/OBObject.h>

@class OBArray;

@protocol WkFileDialogSettings <OBObject>

- (BOOL)allowsDirectories;
- (BOOL)allowsMultipleFiles;
- (OBArray /* OBString */ *)acceptedMimeTypes;
- (OBArray /* OBString */ *)acceptedFileExtensions;
- (OBArray /* OBString */ *)selectedFiles;

@end

@protocol WkFileDialogResponseHandler <OBObject>

- (void)selectedFile:(OBString *)file;
- (void)selectedFiles:(OBArray /* OBString */ *)files;
- (void)cancelled;

@end
