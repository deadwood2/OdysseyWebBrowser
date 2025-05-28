#import <ob/OBString.h>
#import <mui/MUIArea.h>
#import <mui/MUIGroup.h>

@class WkWebView, WkPrintingState;

// Notification on layout recalcs
#define kWkPrintingStateRecalculated @"wkPrintingStateRecalculated"

@interface WkPrintingPage : OBObject

- (OBString *)name;
- (OBString *)key;

// page sizes in inches
- (float)width;
- (float)height;

// without margins
- (float)contentWidth;
- (float)contentHeight;

// margin sizes in inches
- (float)marginLeft;
- (float)marginRight;
- (float)marginTop;
- (float)marginBottom;

@end

@interface WkPrintingProfile : OBObject

- (OBString *)name;

- (OBArray /* WkPrintingPage */*)pageFormats;
- (WkPrintingPage *)defaultPageFormat;
- (WkPrintingPage *)selectedPageFormat;

- (OBString *)printerModel;
- (OBString *)manufacturer;
- (LONG)psLevel;

- (BOOL)canSelectPageFormat;
- (void)setSelectedPageFormat:(WkPrintingPage *)page;

- (BOOL)isPDFFilePrinter;

@end

@interface WkPrintingRange : OBObject

// Do note that page numbering begins from 1!

+ (WkPrintingRange *)rangeWithPage:(LONG)pageNo;
+ (WkPrintingRange *)rangeFromPage:(LONG)pageStart toPage:(LONG)pageEnd;
+ (WkPrintingRange *)rangeFromPage:(LONG)pageStart count:(LONG)count;

- (LONG)pageStart;
- (LONG)pageEnd;
- (LONG)count;

@end

@protocol WkPrintingStateDelegate <OBObject>

- (void)printingState:(WkPrintingState *)state updatedProgress:(float)progress;

@end

@interface WkPrintingState : OBObject

// Associated WkWebView
- (WkWebView *)webView;

// --- Printer Selection

// Selected printer
- (WkPrintingProfile *)profile;
- (void)setProfile:(WkPrintingProfile *)profile;

// All available printer profiles
- (OBArray * /* WkPrintingProfile */)allProfiles;

// ---- Page and layout setup

// Orientation of the page
- (void)setLandscape:(BOOL)landscape;
- (BOOL)landscape;

// How many pages to layout on 1 sheet (accepted values: 1, 2, 4, 6, 9)
- (LONG)pagesPerSheet;
- (void)setPagesPerSheet:(LONG)pps;

// 1.0 for 100% scales text on the page
- (float)userScalingFactor;
- (void)setUserScalingFactor:(float)scaling;

// margin sizes in inches
- (float)marginLeft;
- (float)marginTop;
- (float)marginRight;
- (float)marginBottom;

- (void)setMarginLeft:(float)left top:(float)top right:(float)right bottom:(float)bottom;
- (void)resetMarginsToPaperDefaults;

// ---- Print job setup

// Which sheets to print
- (WkPrintingRange *)printingRange;
- (void)setPrintingRange:(WkPrintingRange *)range;

typedef enum
{
	WkPrintingState_Parity_AllSheets,
	WkPrintingState_Parity_OddSheets,
	WkPrintingState_Parity_EvenSheets,
} WkPrintingState_Parity;

// whether to print odd/even sheets or all sheets (within the printingRange)
- (WkPrintingState_Parity)parity;
- (void)setParity:(WkPrintingState_Parity)parity;

- (BOOL)shouldPrintBackgrounds;
- (void)setShouldPrintBackgrounds:(BOOL)printBackgrounds;

- (LONG)copies;
- (void)setCopies:(LONG)numCopies;

// ---- Info and preview

// Number of pages, disregarding pages per sheet and odd/even page printing
- (LONG)pages;

// Calculated # of sheets of paper, affected by pages per sheet
- (LONG)sheets;

// Total # of sheets to print (affected by parity)
- (LONG)printJobSheets;

// Previewed sheet number - this is affected by all page layout setup attributes
// This is a value between 1 and -sheets
- (LONG)previevedSheet;
- (void)setPrevievedSheet:(LONG)sheet;

// ---- Settings

// Obtain and apply settings in JSON format
- (OBDictionary *)settings;
- (void)setSettings:(OBDictionary *)settings;

@end
