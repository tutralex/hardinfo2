/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 L. A. F. Pereira <l@tia.mat.br>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */


#include <config.h>
#include <shell.h>

#include <report.h>
#include <hardinfo.h>
#include <iconcache.h>
#include <stock.h>
#include <vendor.h>
#include <syncmanager.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <stdbool.h>
#include "callbacks.h"
#include "dmi_util.h"

ProgramParameters params = { 0 };

int main(int argc, char **argv)
{
    int exit_code = 0;
    GSList *modules;
    gboolean cleanUserData;
    char *appver;
    gchar *path;
    GKeyFile *key_file = g_key_file_new();
    gchar *conf_path = g_build_filename(g_get_user_config_dir(), "hardinfo2", "settings.ini", NULL);

    DEBUG("Hardinfo2 version " VERSION ". Debug version.");

#if GLIB_CHECK_VERSION(2,32,5)
#else
    //if (!g_thread_supported ()) g_thread_init(NULL);
    g_type_init ();
#endif

    /* parse all command line parameters */
    parameters_init(&argc, &argv, &params);

    params.path_data=g_strdup(PREFIX);
    params.path_lib=g_strdup(LIBPREFIX);
    params.path_locale=g_strdup(LOCALEDIR);

    //scale from environment
    const char *s=getenv("GDK_DPI_SCALE");
    if(!s || (sscanf(s,"%f",&params.scale)!=1)) params.scale=1.0;

    setlocale(LC_ALL, "");
    bindtextdomain("hardinfo2", params.path_locale);
    textdomain("hardinfo2");

    //Remove ids and json files when starting new program version or first time
    g_key_file_load_from_file(
        key_file, conf_path,
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    cleanUserData=false;
    appver = g_key_file_get_string(key_file, "Application", "Version", NULL);
    if(appver){
        if(strcmp(appver,VERSION)) cleanUserData=true;
    } else {appver="OLD";cleanUserData=true;}

    if(cleanUserData){
        DEBUG("Cleaning User Data.... (%s<>%s)\n",appver,VERSION);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","blobs-update-version.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","benchmark.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","cpuflags.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","kernel-module-icons.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","arm.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","ieee_oui.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","edid.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","pci.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","sdcard.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","usb.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","vendor.ids", NULL);g_remove(path);g_free(path);
	//update settings.ini
	g_key_file_set_string(key_file, "Application", "Version", VERSION);
#if GLIB_CHECK_VERSION(2,40,0)
	g_key_file_save_to_file(key_file, conf_path, NULL);
#else
	g2_key_file_save_to_file(key_file, conf_path, NULL);
#endif
    }
    g_free(conf_path);
    g_key_file_free(key_file);

    /* show version information and quit */
    if (params.show_version) {
        g_print("Hardinfo2 version " VERSION "\n");
        g_print
            (_(/*!/ %d will be latest year of copyright*/ "Copyright (C) 2003-2023 L. A. F. Pereira. 2024-%d Hardinfo2 Project.\n\n"), RELEASE_YEAR );

	g_print(N_("Compile-time options:\n"
		"  Release version:  %s (%s)\n"
		"  LibSoup version:  %s\n"
		"  Data           :  %s\n"
		"  Library        :  %s\n"
		"  Locale         :  %s\n"
		"  Compiled for   :  %s\n"),
		RELEASE==1 ? "Yes (" VERSION ")" : (RELEASE==0?"No (" VERSION ")":"Debug (" VERSION ")"), ARCH,
		HARDINFO2_LIBSOUP3 ? _("3.0") : "2.4",
		params.path_data, params.path_lib, params.path_locale,
		PLATFORM);
        return 0;
    }

    if (!params.create_report && !params.run_benchmark) {
        /* we only try to open the UI if the user didn't ask for a report. */
        params.gui_running = ui_init(&argc, &argv);

        /* as a fallback, if GTK+ initialization failed, run in report
           generation mode. */
        if (!params.gui_running) {
            params.create_report = TRUE;
            /* ... it is possible to -f html without -r */
            if (params.report_format != REPORT_FORMAT_HTML)
                params.markup_ok = FALSE;
        }
    }

    /* load all modules */
    DEBUG("loading all modules");
    modules = modules_load_all();

    /* initialize vendor database */
    vendor_init();

    /* initialize moreinfo */
    moreinfo_init();

    if (params.run_benchmark) {
        gchar *result;

        result = module_call_method_param("benchmark::runBenchmark", params.run_benchmark);
        if (!result) {
          fprintf(stderr, _("Unknown benchmark ``%s''\n"), params.run_benchmark);
          exit_code = 1;
        } else {
          fprintf(stderr, "\n");
          g_print("%s\n", result);
          g_free(result);
        }
    } else if (params.gui_running) {
	/* initialize gui and start gtk+ main loop */
	icon_cache_init();
	stock_icons_init();

	shell_init(modules);

	DEBUG("entering gtk+ main loop");

	gtk_main();
    } else if (params.create_report) {
	/* generate report */
	gchar *report=NULL;

	if(params.bench_user_note) {//synchronize without sending benchmarks
	    sync_manager_update_on_startup(0);
	}

	DEBUG("generating report");

#define REPORTSIZE 1024*1024
	if(params.topiccached){
            gchar *file=g_build_filename(g_get_user_config_dir(), "hardinfo2", "cachedreport", NULL);
	    FILE *io=fopen(file,"r");
	    if(io){
	        report=g_malloc(REPORTSIZE);
		if(report){
		  if(fread(report, 1, REPORTSIZE, io)==0) {
		        g_free(report);
			report=NULL;
		    }
		}
	        fclose(io);
	    }
	}
	if(!report){
	    report = report_create_from_module_list_format(modules, params.report_format);

	    gchar *file=g_build_filename(g_get_user_config_dir(), "hardinfo2", "cachedreport", NULL);
	    FILE *io=fopen(file,"w");
	    fputs(report,io);
	    fclose(io);
	}

	if(params.topic){
	    int active=0;
	    size_t poslen,plen;
	    gchar *p=report,*pos=report,*header=NULL;
	    while(*pos!=0){
	        while( (*pos!=0) && (*pos!='\n') ) pos++;
	        if(*pos){
	            *pos=0;
		    poslen=strlen(pos+1);
		    plen=strlen(p);
		    //stop before printing new header/module
		    if((active==1) && (poslen>=2) && (((*(pos+1)=='*') && (*(pos+2)=='*'))) ) {active=-1;} //next is module
		    if((active>=2) && (poslen>=2) && (((*(pos+1)=='-') && (*(pos+2)=='-')) || ((*(pos+1)=='*') && (*(pos+2)=='*'))) ) {if(active==2) active=-1; else active=0;} //next is heading/module

		    if(strcmp(params.topic,"getlist")==0) {
		        if((*(p+0)==' ') && (*(p+3)=='-')) {active=4;}//subblock
		        if((*(p+0)=='-') && (*(p+1)!='-')) {active=3;}//subheading
		        if((*(p+0)!='-') && (*(p+0)>32) && (*(p+0)!='*')) {active=1;}//heading
		        if(active) g_print("%s\n",p);
		        active=0;
		    } else {
		        if((*(p+0)!='-') && (*(p+0)>32) && (*(p+0)!='*')) {if(header) g_free(header);header=g_strdup(p);}//heading
		        //if((*(p+0)=='-') && (*(p+1)!='-')) {if(header) header=g_strconcat(header,p,NULL); else header=g_strdup(p);}//subheading
		        //start
			if((active==0) && (plen>=(4+strlen(params.topic))) && (g_ascii_strncasecmp(p+4,params.topic,strlen(params.topic))==0) ) {active=4;if(header) g_print("%s\n",header);}//subblock
			if((active==0) && (plen>=(1+strlen(params.topic))) && (g_ascii_strncasecmp(p+1,params.topic,strlen(params.topic))==0) ) {active=3;if(header) g_print("%s\n",header);}//subheading
			if((active==0) && (plen>=(  strlen(params.topic))) && (g_ascii_strncasecmp(p  ,params.topic,strlen(params.topic))==0) && ((poslen>=1)&&(*(pos+1)=='-')) ) {active=2;}//heading
			if((active==0) && (plen>=(  strlen(params.topic))) && (g_ascii_strncasecmp(p  ,params.topic,strlen(params.topic))==0) && ((poslen>=1)&&(*(pos+1)=='*')) ) {active=1;}//module
			if((*params.topic=='*') && (strlen(params.topic)>=2) && (strstr(p,params.topic+1)!=0)) {active=1;}//text search
			//print
			if(active>0) g_print("%s\n",p);
			//Stop
			if(*params.topic=='*') {active=0;}//text search
			if((active>=4) && (poslen>=4) && (*(pos+4)=='-')) {active=0;} //active subblock and next is new subblock
			if((active>=3) && (poslen>=1) && (*(pos+1)=='-')) {active=0;} //active and next is subheader
		    }

		    pos++; p=pos;
	        }
	    }
	} else {
	    g_print("%s", report);
	}

	if(params.bench_user_note) {//synchronize
	    if(!params.skip_benchmarks)
	       sync_manager_update_on_startup(1);
	}

	g_free(report);
    } else {
        g_error(_("Don't know what to do. Exiting."));
    }
    moreinfo_shutdown();
    vendor_cleanup();
    dmidecode_cache_free();
    DEBUG("finished");
    return exit_code;
}
