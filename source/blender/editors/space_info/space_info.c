/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/editors/space_info/space_info.c
 *  \ingroup spinfo
 */


#include <string.h>
#include <stdio.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_utildefines.h"

#include "BLF_translation.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_screen.h"

#include "ED_space_api.h"
#include "ED_screen.h"

#include "BIF_gl.h"

#include "RNA_access.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_resources.h"
#include "UI_interface.h"
#include "UI_view2d.h"

#include "info_intern.h"  /* own include */
#include "BLO_readfile.h"

/* ******************** default callbacks for info space ***************** */


/** Called during instantiation of sorts (differs from initialisation/initial registration). Happens when switching views? */
/** Instantiate instances of the AreaRegions, wheras the init bit, more pre-defines their type (sort of class vs instance) */
static SpaceLink *info_new(const bContext *UNUSED(C))
{

	ARegion *ar;
	SpaceInfo *sinfo;
	
	sinfo = MEM_callocN(sizeof(SpaceInfo), "initinfo");
	sinfo->spacetype = SPACE_INFO;

	sinfo->rpt_mask = INFO_RPT_OP;

	/* header */
	ar = MEM_callocN(sizeof(ARegion), "header for info");
	
	BLI_addtail(&sinfo->regionbase, ar);
	ar->regiontype = RGN_TYPE_HEADER;
	ar->alignment = RGN_ALIGN_BOTTOM;







	/* ribbon */
	ar = MEM_callocN(sizeof(ARegion), "ribbon area for info");
	BLI_addtail(&sinfo->regionbase, ar);
	ar->regiontype = RGN_TYPE_UI;
	ar->alignment = RGN_ALIGN_TOP;
	ar->sizex = 1000;
	ar->sizey = 100;
	//ar->winrct
	//ar-> // regiondata, panels, do_draw, uiblocks







	
	/* main area */
	/*ar = MEM_callocN(sizeof(ARegion), "main area for info");
	
	BLI_addtail(&sinfo->regionbase, ar);
	ar->regiontype = RGN_TYPE_WINDOW;*/
	
	/* keep in sync with console */
	//ar->v2d.scroll |= (V2D_SCROLL_RIGHT);
	//ar->v2d.align |= V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_NEG_Y; /* align bottom left */
	//ar->v2d.keepofs |= V2D_LOCKOFS_X;
	//ar->v2d.keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
	//ar->v2d.keeptot = V2D_KEEPTOT_BOUNDS;
	//ar->v2d.minzoom = ar->v2d.maxzoom = 1.0f;


	ar->do_draw = 5;

	/* for now, aspect ratio should be maintained, and zoom is clamped within sane default limits */
	//ar->v2d.keepzoom = (V2D_KEEPASPECT|V2D_LIMITZOOM);
	
	return (SpaceLink *)sinfo;
}

/* not spacelink itself */
static void info_free(SpaceLink *UNUSED(sl))
{	
//	SpaceInfo *sinfo = (SpaceInfo *) sl;
	
}


/* spacetype; init callback */
static void info_init(struct wmWindowManager *UNUSED(wm), ScrArea *UNUSED(sa))
{

}

static SpaceLink *info_duplicate(SpaceLink *sl)
{
	SpaceInfo *sinfon = MEM_dupallocN(sl);
	
	/* clear or remove stuff from old */
	
	return (SpaceLink *)sinfon;
}



/* add handlers, stuff you only do once or on area/region changes */
static void info_main_area_init(wmWindowManager *wm, ARegion *ar)
{
	wmKeyMap *keymap;

	/* force it on init, for old files, until it becomes config */
	ar->v2d.scroll = (V2D_SCROLL_RIGHT);
	
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_CUSTOM, ar->winx, ar->winy);

	/* own keymap */
	keymap = WM_keymap_find(wm->defaultconf, "Info", SPACE_INFO, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
}

static void info_textview_update_rect(const bContext *C, ARegion *ar)
{
	SpaceInfo *sinfo = CTX_wm_space_info(C);
	View2D *v2d = &ar->v2d;

	UI_view2d_totRect_set(v2d, ar->winx - 1, info_textview_height(sinfo, ar, CTX_wm_reports(C)));
}

static void info_main_area_draw(const bContext *C, ARegion *ar)
{
	/* draw entirely, view changes should be handled here */
	SpaceInfo *sinfo = CTX_wm_space_info(C);
	View2D *v2d = &ar->v2d;
	View2DScrollers *scrollers;

	/* clear and setup matrix */
	UI_ThemeClearColor(TH_BACK);
	glClear(GL_COLOR_BUFFER_BIT);

	/* quick way to avoid drawing if not bug enough */
	if (ar->winy < 16)
		return;
		
	info_textview_update_rect(C, ar);

	/* worlks best with no view2d matrix set */
	UI_view2d_view_ortho(v2d);

	info_textview_main(sinfo, ar, CTX_wm_reports(C));

	/* reset view matrix */
	UI_view2d_view_restore(C);
	
	/* scrollers */
	scrollers = UI_view2d_scrollers_calc(C, v2d, V2D_ARG_DUMMY, V2D_ARG_DUMMY, V2D_ARG_DUMMY, V2D_GRID_CLAMP);
	UI_view2d_scrollers_draw(C, v2d, scrollers);
	UI_view2d_scrollers_free(scrollers);
}


static int fooButPoll(bContext *C, wmOperator *ot)
{
	//SpaceInfo *sinfo = CTX_wm_space_info(C);
	//ReportList *reports = CTX_wm_reports(C);
	//int report_mask = info_report_mask(sinfo);

	//Report *report;

	//DynStr *buf_dyn = BLI_dynstr_new();
	//char *buf_str;

	//for (report = reports->list.first; report; report = report->next) {
	//	if ((report->type & report_mask) && (report->flag & SELECT)) {
	//		BLI_dynstr_append(buf_dyn, report->message);
	//		BLI_dynstr_append(buf_dyn, "\n");
	//	}
	//}

	//buf_str = BLI_dynstr_get_cstring(buf_dyn);
	//BLI_dynstr_free(buf_dyn);

	//WM_clipboard_text_set(buf_str, 0);

	//MEM_freeN(buf_str);
	//return OPERATOR_FINISHED;

	return OPERATOR_RUNNING_MODAL;
}

static void fooButHandler(bContext *C, void *arg1, void *arg2) {
	printf("gkgklf");
}


static void INFO_OT_dummyOp(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Foo the bar";
	ot->description = "DDDDD";
	ot->idname = "INFO_OT_dummyOp";

	/* api callbacks */
	ot->poll = fooButPoll;
	ot->exec = fooButHandler;
}


static void info_operatortypes(void)
{
	WM_operatortype_append(FILE_OT_autopack_toggle);
	WM_operatortype_append(FILE_OT_pack_all);
	WM_operatortype_append(FILE_OT_pack_libraries);
	WM_operatortype_append(FILE_OT_unpack_all);
	WM_operatortype_append(FILE_OT_unpack_item);
	WM_operatortype_append(FILE_OT_unpack_libraries);
	
	WM_operatortype_append(FILE_OT_make_paths_relative);
	WM_operatortype_append(FILE_OT_make_paths_absolute);
	WM_operatortype_append(FILE_OT_report_missing_files);
	WM_operatortype_append(FILE_OT_find_missing_files);
	WM_operatortype_append(INFO_OT_reports_display_update);


	/* info_report.c */
	WM_operatortype_append(INFO_OT_select_pick);
	WM_operatortype_append(INFO_OT_select_all_toggle);
	WM_operatortype_append(INFO_OT_select_border);

	WM_operatortype_append(INFO_OT_report_replay);
	WM_operatortype_append(INFO_OT_report_delete);
	WM_operatortype_append(INFO_OT_report_copy);

	WM_operatortype_append(INFO_OT_dummyOp);
}

static void info_keymap(struct wmKeyConfig *keyconf)
{
	wmKeyMap *keymap = WM_keymap_find(keyconf, "Window", 0, 0);
	
	WM_keymap_verify_item(keymap, "INFO_OT_reports_display_update", TIMERREPORT, KM_ANY, KM_ANY, 0);

	/* info space */
	keymap = WM_keymap_find(keyconf, "Info", SPACE_INFO, 0);
	
	
	/* report selection */
	WM_keymap_add_item(keymap, "INFO_OT_select_pick", SELECTMOUSE, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "INFO_OT_select_all_toggle", AKEY, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "INFO_OT_select_border", BKEY, KM_PRESS, 0, 0);

	WM_keymap_add_item(keymap, "INFO_OT_report_replay", RKEY, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "INFO_OT_report_delete", XKEY, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "INFO_OT_report_delete", DELKEY, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "INFO_OT_report_copy", CKEY, KM_PRESS, KM_CTRL, 0);
#ifdef __APPLE__
	WM_keymap_add_item(keymap, "INFO_OT_report_copy", CKEY, KM_PRESS, KM_OSKEY, 0);
#endif
}

/* add handlers, stuff you only do once or on area/region changes */
static void info_header_area_init(wmWindowManager *UNUSED(wm), ARegion *ar)
{
	ED_region_header_init(ar);
}

static void info_header_area_draw(const bContext *C, ARegion *ar)
{
	ED_region_header(C, ar);
}

static void info_main_area_listener(bScreen *UNUSED(sc), ScrArea *UNUSED(sa), ARegion *ar, wmNotifier *wmn)
{
	// SpaceInfo *sinfo = sa->spacedata.first;

	/* context changes */
	switch (wmn->category) {
		case NC_SPACE:
			if (wmn->data == ND_SPACE_INFO_REPORT) {
				/* redraw also but only for report view, could do less redraws by checking the type */
				ED_region_tag_redraw(ar);
			}
			break;
	}
}

static void info_header_listener(bScreen *UNUSED(sc), ScrArea *UNUSED(sa), ARegion *ar, wmNotifier *wmn)
{
	/* context changes */
	switch (wmn->category) {
		case NC_SCREEN:
			if (ELEM(wmn->data, ND_SCREENCAST, ND_ANIMPLAY))
				ED_region_tag_redraw(ar);
			break;
		case NC_WM:
			if (wmn->data == ND_JOB)
				ED_region_tag_redraw(ar);
			break;
		case NC_SCENE:
			if (wmn->data == ND_RENDER_RESULT)
				ED_region_tag_redraw(ar);
			break;
		case NC_SPACE:
			if (wmn->data == ND_SPACE_INFO)
				ED_region_tag_redraw(ar);
			break;
		case NC_ID:
			if (wmn->action == NA_RENAME)
				ED_region_tag_redraw(ar);
			break;
	}
	
}

static void recent_files_menu_draw(const bContext *UNUSED(C), Menu *menu)
{
	struct RecentFile *recent;
	char file[FILE_MAX];
	uiLayout *layout = menu->layout;
	uiLayoutSetOperatorContext(layout, WM_OP_EXEC_REGION_WIN);
	if (G.recent_files.first) {
		for (recent = G.recent_files.first; (recent); recent = recent->next) {
			BLI_split_file_part(recent->filepath, file, sizeof(file));
			if (BLO_has_bfile_extension(file))
				uiItemStringO(layout, BLI_path_basename(recent->filepath), ICON_FILE_BLEND, "WM_OT_open_mainfile", "filepath", recent->filepath);
			else
				uiItemStringO(layout, BLI_path_basename(recent->filepath), ICON_FILE_BACKUP, "WM_OT_open_mainfile", "filepath", recent->filepath);
		}
	}
	else {
		uiItemL(layout, IFACE_("No Recent Files"), ICON_NONE);
	}
}

static void recent_files_menu_register(void)
{
	MenuType *mt;

	mt = MEM_callocN(sizeof(MenuType), "spacetype info menu recent files");
	strcpy(mt->idname, "INFO_MT_file_open_recent");
	strcpy(mt->label, N_("Open Recent..."));
	strcpy(mt->translation_context, BLF_I18NCONTEXT_DEFAULT_BPYRNA);
	mt->draw = recent_files_menu_draw;
	WM_menutype_add(mt);
}


#define IMASEL_BUTTONS_HEIGHT (UI_UNIT_Y * 2)
#define IMASEL_BUTTONS_MARGIN (UI_UNIT_Y / 6)

typedef struct uiBlock uiBlock;
//
//static void ribbon_panel_draw(const struct bContext *C, struct Panel *pt) {
//
//}
//
//static void ribbon_panel_register(ARegionType *art)
//{
//	PanelType *pt;
//
//	pt = MEM_callocN(sizeof(PanelType), "spacetype file system directories");
//	strcpy(pt->idname, "FILE_PT_system");
//	strcpy(pt->label, N_("System"));
//	strcpy(pt->translation_context, BLF_I18NCONTEXT_DEFAULT_BPYRNA);
//	pt->draw = ribbon_panel_draw;
//	BLI_addtail(&art->paneltypes, pt);
//}


static void block_func_draw_check(bContext *C, void *UNUSED(arg1), void *UNUSED(arg2))
{
	//SpaceFile *sfile = CTX_wm_space_file(C);
	SpaceInfo *ssss = CTX_wm_space_info(C);
	//wmOperator *op = sfile->op;
	//if (op) { /* fail on reload */
	//	if (op->type->check) {
	//		char filepath[FILE_MAX];
	//		file_sfile_to_operator(op, sfile, filepath);

	//		/* redraw */
	//		if (op->type->check(C, op)) {
	//			file_operator_to_sfile(sfile, op);

	//			/* redraw, else the changed settings wont get updated */
	//			ED_area_tag_redraw(CTX_wm_area(C));
	//		}
	//	}
	//}
}





/* Note: This function uses pixelspace (0, 0, winx, winy), not view2d.
* The controls are laid out as follows:
*
* -------------------------------------------
* | Directory input               | execute |
* -------------------------------------------
* | Filename input        | + | - | cancel  |
* -------------------------------------------
*
* The input widgets will stretch to fill any excess space.
* When there isn't enough space for all controls to be shown, they are
* hidden in this order: x/-, execute/cancel, input widgets.
*/
static void info_draw_ribbon_buttons(const bContext *C, ARegion *ar)
{


	ED_region_panels(C, ar, 1, CTX_data_mode_string(C), -1);

	return;




	/* Button layout. */
	//const int max_x = ar->winx - 10;
	//const int line1_y = ar->winy - (IMASEL_BUTTONS_HEIGHT / 2 + IMASEL_BUTTONS_MARGIN);
	//const int line2_y = line1_y - (IMASEL_BUTTONS_HEIGHT / 2 + IMASEL_BUTTONS_MARGIN);
	//const int input_minw = 20;
	const int btn_h = UI_UNIT_Y;
	const int btn_fn_w = UI_UNIT_X;
	const int btn_minw = 80;
	//const int btn_margin = 20;
	//const int separator = 4;

	/* Additional locals. */
	char uiblockstr[32];
	int loadbutton;
	int fnumbuttons;
	int min_x = 10;
	int chan_offs = 0;
	//int available_w = max_x - min_x;
	//int line1_w = available_w;
	//int line2_w = available_w;

	uiBut *but;
	uiBlock *block;
	ARegion *artmp;

	/* Initialize UI block. */
	BLI_snprintf(uiblockstr, sizeof(uiblockstr), "win %p", (void *)ar);
	block = UI_block_begin(C, ar, uiblockstr, UI_EMBOSS);
	

	/* exception to make space for collapsed region icon */
	/*for (artmp = CTX_wm_area(C)->regionbase.first; artmp; artmp = artmp->next) {
		if (artmp->regiontype == RGN_TYPE_CHANNELS && artmp->flag & RGN_FLAG_HIDDEN) {
			chan_offs = 16;
			min_x += chan_offs;
			available_w -= chan_offs;
		}
	}*/

	/* Is there enough space for the execute / cancel buttons? */


	//const uiFontStyle *fstyle = UI_FSTYLE_WIDGET;
	//loadbutton = UI_fontstyle_string_width(fstyle, "foooooo000") + btn_margin;
	//CLAMP_MIN(loadbutton, btn_minw);
	//if (available_w <= loadbutton + separator + input_minw) {
	//	loadbutton = 0;
	//}

	//if (loadbutton) {
	//	line1_w -= (loadbutton + separator);
	//	line2_w = line1_w;
	//}

	///* Is there enough space for file number increment/decrement buttons? */
	//fnumbuttons = 2 * btn_fn_w;
	//if (!loadbutton || line2_w <= fnumbuttons + separator + input_minw) {
	//	fnumbuttons = 0;
	//}
	//else {
	//	line2_w -= (fnumbuttons + separator);
	//}

	/* Text input fields for directory and file. */
	//if (available_w > 0) {
	//	int overwrite_alert = file_draw_check_exists(sfile);
	//	/* callbacks for operator check functions */
	UI_block_func_set(block, block_func_draw_check, NULL, NULL);

	//	but = uiDefBut(block, UI_BTYPE_TEXT, -1, "",
	//		min_x, line1_y, line1_w - chan_offs, btn_h,
	//		params->dir, 0.0, (float)FILE_MAX, 0, 0,
	//		TIP_("File path"));
	//	UI_but_func_complete_set(but, autocomplete_directory, NULL);
	//	UI_but_flag_enable(but, UI_BUT_NO_UTF8);
	//	UI_but_flag_disable(but, UI_BUT_UNDO);
	//	UI_but_funcN_set(but, file_directory_enter_handle, NULL, but);

	//	/* TODO, directory editing is non-functional while a library is loaded
	//	* until this is properly supported just disable it. */
	//	if (sfile->files && filelist_lib(sfile->files))
	//		UI_but_flag_enable(but, UI_BUT_DISABLED);

	//	if ((params->flag & FILE_DIRSEL_ONLY) == 0) {
	//		but = uiDefBut(block, UI_BTYPE_TEXT, -1, "",
	//			min_x, line2_y, line2_w - chan_offs, btn_h,
	//			params->file, 0.0, (float)FILE_MAXFILE, 0, 0,
	//			TIP_(overwrite_alert ? N_("File name, overwrite existing") : N_("File name")));
	//		UI_but_func_complete_set(but, autocomplete_file, NULL);
	//		UI_but_flag_enable(but, UI_BUT_NO_UTF8);
	//		UI_but_flag_disable(but, UI_BUT_UNDO);
	//		/* silly workaround calling NFunc to ensure this does not get called
	//		* immediate ui_apply_but_func but only after button deactivates */
	//		UI_but_funcN_set(but, file_filename_enter_handle, NULL, but);

	//		/* check if this overrides a file and if the operator option is used */
	//		if (overwrite_alert) {
	//			UI_but_flag_enable(but, UI_BUT_REDALERT);
	//		}
	//	}




	//UI_panel_category_draw_all




	//	/* clear func */
	UI_block_func_set(block, NULL, NULL, NULL);

	//}



	UI_block_align_begin(block);


	/*but = uiDefIconBut(block, UI_BTYPE_BUT, 5, ICON_OOPS, 20, 100, 300, 40, "don't know what this is supposed to be a pointer to.....", 30, 50, 33, 33, "my info tooltip");
	UI_but_func_set(but, fooButHandler, NULL, NULL);
	ui_but_update(but);*/



	//but = uiDefButO(block, UI_BTYPE_BUT, "FILE_OT_execute", WM_OP_INVOKE_DEFAULT, "title", 50, 50, 500, 40, "tipppppy");
	but = uiDefButO(block, UI_BTYPE_BUT, "INFO_OT_dummyOp", WM_OP_INVOKE_DEFAULT, "title", 50, 50, 500, 40, "tipppppy");
	UI_but_func_set(but, fooButHandler, NULL, NULL);
	UI_but_funcN_set(but, fooButHandler, NULL, NULL);
	UI_but_func_complete_set(but, fooButHandler, NULL);
	UI_but_flag_enable(but, UI_BUT_UNDO);
	UI_but_flag_enable(but, UI_BUT_NO_UTF8);
	
	// UI_but_func_complete_set(but, autocomplete_directory, NULL);
	//	UI_but_flag_enable(but, UI_BUT_NO_UTF8);
	//	UI_but_flag_disable(but, UI_BUT_UNDO);
	//	UI_but_funcN_set(but, file_directory_enter_handle, NULL, but);
	


	//

	/* Filename number increment / decrement buttons. */
	//if (fnumbuttons && (params->flag & FILE_DIRSEL_ONLY) == 0) {
		//
		but = uiDefIconButO(block, UI_BTYPE_BUT, "INFO_OT_dummyOp", 0, ICON_ZOOMOUT,
			10, 10,
			70, 30,
			TIP_("Decrement the filename number"));
		RNA_int_set(UI_but_operator_ptr_get(but), "increment", -1);
		//UI_but_func_set(but, fooButHandler, NULL, NULL);

		but = uiDefIconButO(block, UI_BTYPE_BUT, "INFO_OT_dummyOp", 0, ICON_ZOOMIN,
			80, 10,
			70, 30,
			TIP_("Increment the filename number"));
		RNA_int_set(UI_but_operator_ptr_get(but), "increment", 1);
	
		
		UI_block_align_end(block);

		//BLI_addhead(&ar->uiblocks, block);
	//}

	/* Execute / cancel buttons. */
	//if (loadbutton) {
	//	/* params->title is already translated! */
	//	uiDefButO(block, UI_BTYPE_BUT, "FILE_OT_execute", WM_OP_EXEC_REGION_WIN, params->title,
	//		max_x - loadbutton, line1_y, loadbutton, btn_h, "");
	//	uiDefButO(block, UI_BTYPE_BUT, "FILE_OT_cancel", WM_OP_EXEC_REGION_WIN, IFACE_("Cancel"),
	//		max_x - loadbutton, line2_y, loadbutton, btn_h, "");
	//}

	UI_block_end(C, block);
	UI_block_draw(C, block);
}


static void info_ribbon_draw(const bContext *C, ARegion *ar)
{
	float col[3];
	/* clear */
	UI_GetThemeColor3fv(TH_BACK, col);
	glClearColor(col[0], col[1], col[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	/* scrolling here is just annoying, disable it */
	ar->v2d.cur.ymax = BLI_rctf_size_y(&ar->v2d.cur);
	ar->v2d.cur.ymin = 0;

	/* set view2d view matrix for scrolling (without scrollers) */
	UI_view2d_view_ortho(&ar->v2d);




	info_draw_ribbon_buttons(C, ar);



	/* Boilerplate */
	UI_view2d_view_restore(C);
}






/** I DON'T EVEN KNOW WHAT THIS DOES!!!!!! I added it as a listener for the region, hoping that makes my buttons get picked up!!!!!!!!!!!!! */
static void file_ui_area_listener(bScreen *UNUSED(sc), ScrArea *UNUSED(sa), ARegion *ar, wmNotifier *wmn)
{
	/* context changes */
	switch (wmn->category) {
	case NC_SPACE:
		switch (wmn->data) {
		case ND_SPACE_FILE_LIST:
			ED_region_tag_redraw(ar);
			break;
		}
		break;
	}
}









static void info_ribbon_area_init(wmWindowManager *wm, ARegion *ar) {

	ED_region_panels_init(wm, ar);

	wmKeyMap *keymap;

	/* force it on init, for old files, until it becomes config */
	ar->v2d.scroll = (V2D_SCROLL_RIGHT);

	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_CUSTOM, ar->winx, ar->winy);

	/* own keymap */
	keymap = WM_keymap_find(wm->defaultconf, "Info", SPACE_INFO, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
}









/* tabbed panel stuff */
static void view3d_buttons_area_listener(bScreen *UNUSED(sc), ScrArea *UNUSED(sa), ARegion *ar, wmNotifier *wmn) {

}

static void view3d_tools_area_init(wmWindowManager *wm, ARegion *ar)
{
	wmKeyMap *keymap;



	ar->regiontype = RGN_TYPE_TOOLS;			// When the region is constructed, setting this on it forces its panels to be drawn as tabs





	ED_region_panels_init(wm, ar);

	keymap = WM_keymap_find(wm->defaultconf, "3D View Generic", SPACE_VIEW3D, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
}

static void view3d_tools_area_draw(const bContext *C, ARegion *ar)
{
	ED_region_panels(C, ar, 1, CTX_data_mode_string(C), -1);
}


static void drawPanel(const struct bContext *C, struct Panel *p) 
{
	// draw the contents of the tab
}









/* only called once, from space/spacetypes.c */
void ED_spacetype_info(void)
{
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "spacetype info");
	ARegionType *art;
	
	st->spaceid = SPACE_INFO;
	strncpy(st->name, "Info", BKE_ST_MAXNAME);
	
	st->new = info_new;
	st->free = info_free;
	st->init = info_init;
	st->duplicate = info_duplicate;
	st->operatortypes = info_operatortypes;
	st->keymap = info_keymap;
	
	/* regions: main window */
	/*art = MEM_callocN(sizeof(ARegionType), "spacetype info region");
	art->regionid = RGN_TYPE_WINDOW;
	art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_FRAMES;

	art->init = info_main_area_init;
	art->draw = info_main_area_draw;
	art->listener = info_main_area_listener;

	BLI_addhead(&st->regiontypes, art);*/
	
	/* regions: header */
	art = MEM_callocN(sizeof(ARegionType), "spacetype info region");
	art->regionid = RGN_TYPE_HEADER;
	art->prefsizey = HEADERY;
	
	art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_FRAMES | ED_KEYMAP_HEADER;
	art->listener = info_header_listener;
	art->init = info_header_area_init;
	art->draw = info_header_area_draw;
	
	BLI_addhead(&st->regiontypes, art);













	/* tabbed toolbar (that's what I hope) */
	art = MEM_callocN(sizeof(ARegionType), "spacetype view3d tools region");
	art->regionid = RGN_TYPE_TOOLS;
	art->prefsizex = 160; /* XXX */
	art->prefsizey = 50; /* XXX */
	art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_FRAMES;
	art->listener = view3d_buttons_area_listener;
	art->init = view3d_tools_area_init;
	art->draw = view3d_tools_area_draw;
	BLI_addhead(&st->regiontypes, art);





	//art = MEM_callocN(sizeof(ARegionType), "spacetype file region");
	////art->regionid = RGN_TYPE_UI;
	////art->regionid = RGN_TYPE_TOOLS;
	//art->regionid = RGN_TYPE_WINDOW;
	////art->regi
	//art->prefsizey = 90;
	////art->keymapflag = ED_KEYMAP_UI;
	//art->listener = file_ui_area_listener;
	//art->init = info_ribbon_area_init;
	//art->draw = info_ribbon_draw;
	//BLI_addhead(&st->regiontypes, art);
	//art->minsizey = 80;
	//art->minsizex = 1000;


	//ribbon_panel_register(art);
	

	PanelType *pt;
	pt = MEM_callocN(sizeof(PanelType), "ribbon home tab panelType");
	strcpy(pt->idname, "INFO_PT_Home");
	strcpy(pt->label, N_("Home"));
	strcpy(pt->translation_context, BLF_I18NCONTEXT_DEFAULT_BPYRNA);
	pt->draw = drawPanel;
	BLI_addtail(&art->paneltypes, pt);


	pt = MEM_callocN(sizeof(PanelType), "ribbon insert tab panelType");
	strcpy(pt->idname, "INFO_PT_Insert");
	strcpy(pt->label, N_("Insert"));
	strcpy(pt->translation_context, BLF_I18NCONTEXT_DEFAULT_BPYRNA);
	pt->draw = drawPanel;
	BLI_addtail(&art->paneltypes, pt)


	pt = MEM_callocN(sizeof(PanelType), "ribbon pageLayout tab panelType");
	strcpy(pt->idname, "INFO_PT_PageLayout");
	strcpy(pt->label, N_("Page layout"));
	strcpy(pt->translation_context, BLF_I18NCONTEXT_DEFAULT_BPYRNA);
	pt->draw = drawPanel;
	BLI_addtail(&art->paneltypes, pt);




	recent_files_menu_register();

	BKE_spacetype_register(st);
}
