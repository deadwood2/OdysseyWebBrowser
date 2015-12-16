/*
 * Copyright 2011 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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

#include "Document.h"
#include "FrameView.h"
#include "HostWindow.h"

#include <Api/WebFrame.h>
#include <Api/WebView.h>

#include <proto/dos.h>
#include <cairo/cairo-ps.h>

#include "OWBPrintContext.h"
#include "gui.h"

#define D(x)
#define USE_THREAD 0

using namespace WebCore;

struct printerjob
{
	int first;
	int last;
	int step;
	char mode;
	cairo_ps_level_t pslevel;
	float scale;
	char output[1024];
};

struct Data
{
	Object *st_output;
	Object *sl_first;
	Object *sl_last;
	Object *sl_scale;
	Object *txt_status;
	Object *bt_print;
	Object *bt_cancel;
	Object *rd_mode;
	Object *bt_prefs;
	Object *gr_pslevel;
	Object *cy_pslevel;
#if USE_THREAD
	Object *proc;
#endif

	Object *browser;
	Frame *frame;
	TEXT  status[256];
	ULONG printing;
	ULONG quit;
	ULONG close;
	
	struct printerjob pj;
};

static char *print_modes[] = {"Postscript", "Turboprint", NULL};
static char *ps_levels[] = {"Level 2", "Level 3", NULL};

DEFNEW
{
	Object *st_output, *sl_first, *sl_last, *sl_scale, *bt_print, *bt_cancel, *txt_status, *rd_mode, *bt_prefs, *gr_pslevel, *cy_pslevel;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','P','R','I'),
			MUIA_Window_Title, GSI(MSG_PRINTERWINDOW_TITLE),
			MUIA_Window_Width, MUIV_Window_Width_Screen(50),
			MUIA_Window_Height, MUIV_Window_Height_Screen(50),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				Child, ColGroup(2),

                    Child, HVSpace,
					//Child, (Object *) MakeLabel(GSI(MSG_PRINTERWINDOW_PRINT_MODE)),
					Child, HGroup,
						Child, rd_mode = RadioObject,
							   MUIA_CycleChain, TRUE,
							   MUIA_Radio_Active,  0,
							   MUIA_Radio_Entries, print_modes,
							   MUIA_Group_Horiz,   TRUE,
							   MUIA_ShowMe, FALSE,
							   End,

						//Child, HVSpace,

						Child, gr_pslevel = HGroup,
											Child, MakeLabel(GSI(MSG_PRINTERWINDOW_PSLEVEL)),
											Child, cy_pslevel = (Object *) MakeCycle(GSI(MSG_PRINTERWINDOW_PSLEVEL), ps_levels),
											End,

						Child, HVSpace,

						Child, bt_prefs = (Object *) MakeButton(GSI(MSG_PRINTERWINDOW_PREFERENCES)),
					    End,

					Child, (Object *) MakeLabel(GSI(MSG_PRINTERWINDOW_OUTPUT)),
					Child, st_output = StringObject,
						   MUIA_CycleChain, TRUE,
						   MUIA_Frame, MUIV_Frame_String,
						   MUIA_String_Contents, "PS:",
						   MUIA_String_MaxLen, 512,
						   End,

					Child, (Object *) MakeLabel(GSI(MSG_PRINTERWINDOW_FIRST_PAGE)),
					Child, sl_first = SliderObject,
						MUIA_CycleChain, TRUE,
						MUIA_Slider_Min, 1,
						MUIA_Slider_Max, 1, 
						MUIA_Frame, MUIV_Frame_Slider,
						End,

					Child, (Object *) MakeLabel(GSI(MSG_PRINTERWINDOW_LAST_PAGE)),
					Child, sl_last = SliderObject,
						MUIA_CycleChain, TRUE,
						MUIA_Slider_Min, 1,
						MUIA_Slider_Max, 1, 
						MUIA_Frame, MUIV_Frame_Slider,
						End,

					Child, (Object *) MakeLabel(GSI(MSG_PRINTERWINDOW_SCALE)),
					Child, sl_scale = SliderObject,
						MUIA_CycleChain, TRUE,
						MUIA_Slider_Min,   1,
						MUIA_Slider_Max,   100,
						MUIA_Slider_Level, 100,
						MUIA_Frame, MUIV_Frame_Slider,
						End,
				End,
									
				Child, txt_status = TextObject,
					   MUIA_Text_SetMin, FALSE,
					   End,

				Child, HGroup,
					   Child, bt_print = (Object *) MakeButton(GSI(MSG_PRINTERWINDOW_PRINT)),
					   Child, HSpace(0),
					   Child, bt_cancel = (Object *) MakeButton(GSI(MSG_PRINTERWINDOW_CANCEL)),
					   End,
				End,

			TAG_MORE, INITTAGS);

	if (obj != NULL)
	{
		GETDATA;
	
		data->sl_first   = sl_first;
		data->sl_last    = sl_last;
		data->sl_scale   = sl_scale;
		data->bt_print   = bt_print;
		data->bt_cancel  = bt_cancel;
		data->st_output  = st_output;
		data->txt_status = txt_status;
		data->rd_mode    = rd_mode;
		data->gr_pslevel = gr_pslevel;
		data->cy_pslevel = cy_pslevel;

		DoMethod(rd_mode, MUIM_Notify, MUIA_Radio_Active, 0, gr_pslevel, 3, MUIM_Set, MUIA_ShowMe, TRUE);
		DoMethod(rd_mode, MUIM_Notify, MUIA_Radio_Active, 1, gr_pslevel, 3, MUIM_Set, MUIA_ShowMe, FALSE);
		DoMethod(sl_scale, MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1, MM_PrinterWindow_UpdateParameters);
		
		DoMethod(bt_print,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_PrinterWindow_Start);
//		  DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_PrinterWindow_Stop);
		DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_PrinterWindow_Close);
		DoMethod(bt_prefs,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_PrinterWindow_PrinterPrefs);
		
		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, MUIV_EveryTime, obj, 1, MM_PrinterWindow_Close);
	}          

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;
	
	data->quit = TRUE;

#if USE_THREAD
	if(data->proc)
	{
		MUI_DisposeObject(data->proc);
		data->proc = NULL;
	}
#endif
	
	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Printer;
		}
		return TRUE;

		case MA_OWB_Browser:
		{
			*msg->opg_Storage = (ULONG) data->browser;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFTMETHOD(PrinterWindow_UpdateParameters)
{
	GETDATA;

	if(data->frame)
	{
		// Surface size in points (1pt = 1/72 inch)
		float surfaceWidth = 8.27*72;
		float surfaceHeight = 11.73*72;

		// A4 Papersize in pixels
		float pageWidth = surfaceWidth;
		float pageHeight = surfaceHeight;

		FloatRect printRect = FloatRect(0, 0, pageWidth, pageHeight);

		// Margins
		float headerHeight = 0;
		float footerHeight = 0;

		float userscale = 1.0;
		int	pageCount = 1;

		OWBPrintContext printContext(data->frame);

		printContext.begin(pageWidth, pageHeight);

		printContext.computeAutomaticScaleFactor(FloatSize(pageWidth, pageHeight));
		userscale = (float) 1.0 * getv(data->sl_scale, MUIA_Slider_Level) / 100.0;

		printContext.computePageRects(printRect, headerHeight, footerHeight, userscale, pageHeight);
		pageCount = printContext.pageCount();
		printContext.end();

		set(data->sl_first, MUIA_Slider_Max, pageCount);
		set(data->sl_last,  MUIA_Slider_Max, pageCount);
		set(data->sl_last,  MUIA_Slider_Level, pageCount);
	}

	return 0;
}

DEFSMETHOD(PrinterWindow_PrintDocument)
{
	GETDATA;

	data->frame   = (Frame *)msg->frame;
	data->browser = NULL;
	data->close   = FALSE;
	data->quit    = FALSE;

	if(data->frame)
	{
		FrameView* frameView = data->frame->view();

		if(frameView)
		{
			BalWidget *widget = frameView->hostWindow()->platformPageClient();

			if(widget)
			{
				data->browser = widget->browser;
			}
		}
	}

#if USE_THREAD
	data->proc = MUI_NewObject(MUIC_Process,
					MUIA_Process_SourceClass , cl,
					MUIA_Process_SourceObject, obj,
					MUIA_Process_Name        , "[OWB] Printing Thread",
					MUIA_Process_Priority    , -1,
					MUIA_Process_AutoLaunch  , FALSE,
					MUIA_Process_StackSize   , 1000000,
					TAG_DONE);

	DoMethod(data->proc, MUIM_Process_Launch);
#endif
	
	DoMethod(obj, MM_PrinterWindow_UpdateParameters);

	set(obj, MUIA_Window_Open, TRUE);
	
	return 0;
}

cairo_status_t pswritefunc(void *closure, const unsigned char *data, unsigned int length)
{
	Write((BPTR) closure, (APTR) data, (LONG) length);
	return CAIRO_STATUS_SUCCESS;
}

DEFTMETHOD(PrinterWindow_Start)
{
	GETDATA;
	
	if(!data->browser)
		return 0;

	set(data->st_output,  MUIA_Disabled, TRUE);
	set(data->bt_print,   MUIA_Disabled, TRUE);
	set(data->sl_first,   MUIA_Disabled, TRUE);
	set(data->sl_last,    MUIA_Disabled, TRUE);
	set(data->sl_scale,   MUIA_Disabled, TRUE);
	set(data->gr_pslevel, MUIA_Disabled, TRUE);

	data->pj.mode    = getv(data->rd_mode, MUIA_Radio_Active);  // PS or Turboprint
	data->pj.step    = 1;
	data->pj.first   = getv(data->sl_first, MUIA_Slider_Level);
	data->pj.last    = getv(data->sl_last,   MUIA_Slider_Level);
	data->pj.pslevel = getv(data->cy_pslevel, MUIA_Cycle_Active) == 1 ? CAIRO_PS_LEVEL_3 : CAIRO_PS_LEVEL_2;
	data->pj.scale   = (float) 1.0 * getv(data->sl_scale, MUIA_Slider_Level) / 100.0;
	
	strncpy(data->pj.output, (char*)getv(data->st_output, MUIA_String_Contents), sizeof(data->pj.output));

#if USE_THREAD
	set(_win(data->browser), MUIA_Window_Sleep, TRUE);
	set(data->browser, MA_OWBBrowser_ForbidEvents, TRUE);

	// assuming printer thread is in ready status siting on Wait()
	DoMethod(data->proc, MUIM_Process_Signal, SIGBREAKF_CTRL_D);
#else
	struct printerjob *pj = &data->pj;

	data->printing = TRUE;

	if(pj->mode == 0) // PS Mode
	{
		if(data->frame)
		{
			// Surface size in points (1pt = 1/72 inch)
			float surfaceWidth = 8.27*72;
			float surfaceHeight = 11.73*72;

			// A4 Papersize in pixels
			float pageWidth = surfaceWidth;
			float pageHeight = surfaceHeight;

			FloatRect printRect = FloatRect(0, 0, pageWidth, pageHeight);

			// Margins
			float headerHeight = 0;
			float footerHeight = 0;

			float userscale = 1.0;

			OWBPrintContext printContext(data->frame);

			printContext.begin(pageWidth, pageHeight);

			printContext.computeAutomaticScaleFactor(FloatSize(pageWidth, pageHeight));
			userscale = pj->scale;

			printContext.computePageRects(printRect, headerHeight, footerHeight, userscale, pageHeight);
						
			BPTR lock = Open(pj->output, MODE_NEWFILE);

			if(lock)
			{
				_cairo_surface *surface = cairo_ps_surface_create_for_stream(pswritefunc, (void*) lock, surfaceWidth, surfaceHeight);

				if(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS)
				{
					cairo_ps_surface_restrict_to_level(surface, pj->pslevel);

					_cairo *cr = cairo_create(surface);

					if(cr)
					{
						if (data->frame->contentRenderer())
						{
							GraphicsContext ctx(cr);

							for(int i = pj->first - 1; i < pj->last; i++)
							{
                                float shrinkfactor = printContext.getPageShrink(i);

								snprintf((char *) data->status, sizeof(data->status), GSI(MSG_PRINTERWINDOW_STATUS), i + 1, pj->last - pj->first + 1);
								DoMethod(obj, MM_PrinterWindow_StatusUpdate);

								printContext.spoolPage(ctx, i, shrinkfactor);
								cairo_show_page(cr);
							}
						}

						cairo_destroy(cr);
					}

					cairo_surface_destroy(surface);
				}
				else
				{
					MUI_Request(app, obj, 0, GSI(MSG_REQUESTER_ERROR_TITLE), GSI(MSG_PRINTERWINDOW_ERROR), TAG_DONE);
				}

				printContext.end();

				Close(lock);
			}
		}
	}
#if 0
	else
	{
		if(data->frame)
		{
			struct MsgPort *PrinterMP;
			union printerIO *PIO;
			struct PrinterData *PD;
			struct TPExtIODRP ExtIoDrp;
			BOOL TP_Installed;
			UWORD TPVersion;

			if((PrinterMP = (struct MsgPort*)CreateMsgPort()))
			{
				if((PIO = (union printerIO *)CreateIORequest(PrinterMP, sizeof(union printerIO))))
				{
					if(!(OpenDevice((STRPTR)"printer.device", 0, (struct IORequest *)PIO, 0)))
					{
						PD = (struct PrinterData *)PIO->iodrp.io_Device;
						TP_Installed = (((ULONG *)(PD->pd_OldStk))[2] == TPMATCHWORD);
						TPVersion = PIO->iodrp.io_Device->dd_Library.lib_Version;

						if(TP_Installed && TPVersion >= 39)
						{
							// Cairo device init
							pctx->cairo_dev = new CairoOutputDev();
							pctx->cairo_dev->setPrinting(false);
							pctx->surface = NULL;
							Catalog *catalog = pctx->doc->getCatalog();
							pctx->cairo_dev->startDoc(pctx->doc->getXRef(), catalog);

							kprintf("PrinterName = '%s', Version=%u, Revision=%u\n",
							   PD->pd_SegmentData->ps_PED.ped_PrinterName, PD->pd_SegmentData->ps_Version,
							   PD->pd_SegmentData->ps_Revision);
						   kprintf("PrinterClass=%u, ColorClass=%u\n",
							   PD->pd_SegmentData->ps_PED.ped_PrinterClass, PD->pd_SegmentData->ps_PED.ped_ColorClass);
						   kprintf("MaxColumns=%u, NumCharSets=%u, NumRows=%u\n",
							   PD->pd_SegmentData->ps_PED.ped_MaxColumns, PD->pd_SegmentData->ps_PED.ped_NumCharSets, PD->pd_SegmentData->ps_PED.ped_NumRows);
						   kprintf("MaxXDots=%lu, MaxYDots=%lu, XDotsInch=%u, YDotsInch=%u\n",
							   PD->pd_SegmentData->ps_PED.ped_MaxXDots, PD->pd_SegmentData->ps_PED.ped_MaxYDots,
							   PD->pd_SegmentData->ps_PED.ped_XDotsInch, PD->pd_SegmentData->ps_PED.ped_YDotsInch);


							struct RastPort MyRastPort;
							struct BitMap MyBitMap;
							int dpi_x = PD->pd_SegmentData->ps_PED.ped_XDotsInch;
							int dpi_y = PD->pd_SegmentData->ps_PED.ped_YDotsInch;

							struct pdfBitmap bm;
							Page *pdfpage = pctx->doc->getCatalog()->getPage(page);
							int width = round((pdfpage->getMediaWidth()/72.0)*25.4)*dpi_x/25.4;
							int height = round((pdfpage->getMediaHeight()/72.0)*25.4)*dpi_y/25.4;

							//kprintf("CairoOutputDev init ok: %d %d\n", 4960, 7015);

							_cairo_surface *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

							//kprintf("cairo_image_surface_create init ok\n");
							pctx->cairo = cairo_create(pctx->surface);
							cairo_save(pctx->cairo);
							cairo_set_source_rgb(pctx->cairo,  1., 1., 1);
							cairo_paint(pctx->cairo);
							cairo_restore(pctx->cairo);
							pctx->cairo_dev->setCairo(pctx->cairo);

							//kprintf(" setCairo ok\n");
							pctx->doc->displayPage(pctx->cairo_dev, page, dpi_x, dpi_y, 0, gTrue, gFalse, gTrue);
							//kprintf(" displayPage ok\n");

							InitRastPort(&MyRastPort);
							MyRastPort.BitMap = &MyBitMap;
							// we need only one BitPlane, because it's chunky format
							InitBitMap(&MyBitMap, 1, width, height);
							MyBitMap.BytesPerRow = width * 3;
							MyBitMap.Planes[0] = cairo_image_surface_get_data(pctx->surface);

							int i, j;
							LONG pixel, *ARGB_ptr;
							char *tmp2;

							ARGB_ptr = (LONG *)cairo_image_surface_get_data(pctx->surface);
							tmp2 = (char *)cairo_image_surface_get_data(pctx->surface);

							// RGBA to RGB conversion
							for(j = 0; j < height; j++)
							{
								for(i = 0; i < width; i++)
								{
									pixel = *ARGB_ptr++;
									*tmp2++ = (pixel & 0xFF0000) >> 16;
									*tmp2++ = (pixel & 0xFF00) >> 8;
									*tmp2++ = (pixel & 0xFF);
								}
							}

							pctx->PIO->iodrp.io_Command = PRD_TPEXTDUMPRPORT;
							pctx->PIO->iodrp.io_RastPort = &MyRastPort;
							pctx->PIO->iodrp.io_SrcX = 0;  			// x offset in rastport to start printing from
							pctx->PIO->iodrp.io_SrcY = 0;  			// y offset in rastpoty to start printing from
							pctx->PIO->iodrp.io_SrcWidth = width;	// width of rastport
							pctx->PIO->iodrp.io_SrcHeight = height; // height of rastport
							pctx->PIO->iodrp.io_DestCols = (LONG)((round((pdfpage->getMediaWidth()/72.0)*25.4)/25.4) * 1000.);  // width in printer pixels format
							pctx->PIO->iodrp.io_DestRows = (LONG)((round((pdfpage->getMediaHeight()/72.0)*25.4)/25.4) * 1000.);	// height in printer pixel format
							pctx->PIO->iodrp.io_Special = SPECIAL_MILROWS | SPECIAL_MILCOLS; 	// save aspect ratio of picture,
																								// turn on inches input for above fields
							// new: io.Modes must point to a new Structure (ExtIoDrp)
							pctx->PIO->iodrp.io_Modes = (ULONG)&pctx->ExtIoDrp;

							// fill in the new structure
							pctx->ExtIoDrp.PixAspX = 1;   // for the correct aspect ratio
							pctx->ExtIoDrp.PixAspY = 1;   // normally the values of the monitor-structure

							pctx->PD->pd_Preferences.PrintFlags = (pctx->PD->pd_Preferences.PrintFlags & ~DIMENSIONS_MASK) | IGNORE_DIMENSIONS;

							//if(data->img.bpp == 24)
							{
								pctx->ExtIoDrp.Mode = TPFMT_RGB24;
								pctx->PIO->iodrp.io_ColorMap = 0;
								DoIO((struct IORequest *)pctx->PIO);
							}

							pctx->cairo_dev->setCairo(NULL);
							cairo_destroy(pctx->cairo);
							pctx->cairo = NULL;
						
						}

						CloseDevice((struct IORequest *)PIO);
					}

					DeleteIORequest((struct IORequest *)PIO);
				}

				DeleteMsgPort(PrinterMP);
			}

			
			// Surface size in points (1pt = 1/72 inch)
			float surfaceWidth = 8.27*72;
			float surfaceHeight = 11.73*72;

			// A4 Papersize in pixels
			float pageWidth = surfaceWidth;
			float pageHeight = surfaceHeight;

			FloatRect printRect = FloatRect(0, 0, pageWidth, pageHeight);

			// Margins
		    float headerHeight = 0;
		    float footerHeight = 0;

			float scale = 1.0;

			OWBPrintContext printContext(data->frame);

			printContext.begin(pageWidth, pageHeight);

			scale = printContext.computeAutomaticScaleFactor(FloatSize(pageWidth, pageHeight));
			scale *= pj->scale;

			printContext.computePageRects(printRect, headerHeight, footerHeight, scale, pageHeight);

		}
	}
#endif

	DoMethod(app, MUIM_Application_PushMethod, obj, 1, MM_PrinterWindow_Done);

	data->printing = FALSE;
#endif
	
	return 0;
}

DEFTMETHOD(PrinterWindow_Stop)
{
	GETDATA;
	if (data->printing)
	{
		data->quit = TRUE; // this will signal printing thread which will 'recall' this method later
	}
	else
	{
		data->quit = FALSE;
		DoMethod(obj, MM_PrinterWindow_Done);
	}
	return 0;
}


DEFTMETHOD(PrinterWindow_Done)
{
	GETDATA;
	
	data->printing = FALSE;
	data->quit     = FALSE;

	set(data->st_output,  MUIA_Disabled, FALSE);
	set(data->bt_print,   MUIA_Disabled, FALSE);
	set(data->sl_first,   MUIA_Disabled, FALSE);
	set(data->sl_last,    MUIA_Disabled, FALSE);
	set(data->sl_scale,   MUIA_Disabled, FALSE);
	set(data->gr_pslevel, MUIA_Disabled, FALSE);
	set(data->txt_status, MUIA_Text_Contents, "");

#if USE_THREAD
	set(data->browser, MA_OWBBrowser_ForbidEvents, FALSE);
	set(_win(data->browser), MUIA_Window_Sleep, FALSE);
    DoMethod(app, MM_OWBApp_WebKitEvents);
#endif
	
	if (data->close)
	{
		DoMethod(obj, MM_PrinterWindow_Close);
	}
	
	return 0;
}

DEFTMETHOD(PrinterWindow_Close)
{
	GETDATA;
	
	if (!data->printing)
	{
		data->close = FALSE;

#if USE_THREAD
		if(data->proc)
		{
			data->quit = TRUE;
			MUI_DisposeObject(data->proc);
			data->proc = NULL;
			data->quit = FALSE;
		}
#endif
		set(obj, MUIA_Window_Open, FALSE);
	}
	else
	{	
		data->quit  = TRUE;
		data->close = TRUE;
	}
	
	return 0;
}

#if USE_THREAD
DEFMMETHOD(Process_Process)
{
	GETDATA;

	while (!*msg->kill)
	{	
		Wait(SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_C);

		if(data->quit == FALSE)
		{
			struct printerjob *pj = &data->pj;

			data->printing = TRUE;

			if(pj->mode == 0) // PS Mode
			{
				if(data->frame)
				{
					// Surface size in points (1pt = 1/72 inch)
					float surfaceWidth = 8.27*72;
					float surfaceHeight = 11.73*72;

					//float surfaceWidth = 800;
					//float surfaceHeight = 11.73/8.27 * surfaceWidth;

					//float surfaceWidth = strtod(getenv("SURFACEWIDTH"), NULL);
					//float surfaceHeight = strtod(getenv("SURFACEHEIGHT"), NULL);

					// A4 Papersize in pixels
					float pageWidth = surfaceWidth;
					float pageHeight = surfaceHeight;

					//float pageWidth  = strtod(getenv("PAGEWIDTH"), NULL);
					//float pageHeight = strtod(getenv("PAGEHEIGHT"), NULL);

					FloatRect printRect = FloatRect(0, 0, pageWidth, pageHeight);

					// Margins
				    float headerHeight = 0;
				    float footerHeight = 0;

					float scale = 1.0;

					OWBPrintContext printContext(data->frame);

					printContext.begin(pageWidth, pageHeight);

					scale = printContext.computeAutomaticScaleFactor(FloatSize(pageWidth, pageHeight));
					scale *= pj->scale;

					printContext.computePageRects(printRect, headerHeight, footerHeight, scale, pageHeight);

					BPTR lock = Open(pj->output, MODE_NEWFILE);

					if(lock)
					{
						_cairo_surface *surface = cairo_ps_surface_create_for_stream(pswritefunc, (void*) lock, surfaceWidth, surfaceHeight);

						if(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS)
						{
							cairo_ps_surface_restrict_to_level(surface, pj->pslevel);

							_cairo *cr = cairo_create(surface);

							if(cr)
							{
								if (data->frame->contentRenderer())
								{
									GraphicsContext ctx(cr);

									for(int i = pj->first - 1; i < pj->last; i++)
									{
										snprintf((char *) data->status, sizeof(data->status), GSI(MSG_PRINTERWINDOW_STATUS), i - (pj->first - 1) + 1, pj->last - pj->first + 1);
										DoMethod(app, MUIM_Application_PushMethod, obj, 1, MM_PrinterWindow_StatusUpdate);

										printContext.spoolPage(ctx, i, scale);
										cairo_show_page(cr);

										if (data->quit)
											break;
									}
								}

								cairo_destroy(cr);
							}

							cairo_surface_destroy(surface);
						}
						else
						{
							MUI_Request(app, obj, 0, GSI(MSG_REQUESTER_ERROR_TITLE), GSI(MSG_PRINTERWINDOW_ERROR), TAG_DONE);
						}

						printContext.end();

						Close(lock);
					}
				}
			}

			DoMethod(app, MUIM_Application_PushMethod, obj, 1, MM_PrinterWindow_Done);
		}
	
		data->printing = FALSE;
	}

	return 0;
}
#endif

DEFTMETHOD(PrinterWindow_StatusUpdate)
{
	GETDATA;
	
	set(data->txt_status, MUIA_Text_Contents, data->status);
	return 0;
}

DEFTMETHOD(PrinterWindow_PrinterPrefs)
{
	SystemTagList("MOSSYS:Prefs/Preferences MOSSYS:Prefs/MPrefs/Printer.mprefs", TAG_DONE);
	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSMETHOD(PrinterWindow_PrintDocument)
DECTMETHOD(PrinterWindow_Start)
DECTMETHOD(PrinterWindow_Stop)
DECTMETHOD(PrinterWindow_Done)
DECTMETHOD(PrinterWindow_Close)
DECTMETHOD(PrinterWindow_StatusUpdate)
DECTMETHOD(PrinterWindow_PrinterPrefs)
DECTMETHOD(PrinterWindow_UpdateParameters)
#if USE_THREAD
DECMMETHOD(Process_Process)
#endif
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, printerwindowclass)
