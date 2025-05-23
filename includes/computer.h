/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include "hardinfo.h"
#include "x_util.h"

typedef struct _Computer	Computer;
typedef struct _OperatingSystem	OperatingSystem;
typedef struct _MemoryInfo	MemoryInfo;
typedef struct _UptimeInfo	UptimeInfo;
typedef struct _LoadInfo	LoadInfo;
typedef struct _DisplayInfo	DisplayInfo;
typedef struct _Distro		Distro;

typedef struct _AlsaInfo	AlsaInfo;
typedef struct _AlsaCard	AlsaCard;

typedef struct _FileSystem	FileSystem;
typedef struct _FileSystemEntry	FileSystemEntry;

struct _AlsaCard {
    gchar *alsa_name;
    gchar *friendly_name;
/*
  gchar   *board;
  gchar    revision, compat_class;
  gint     subsys_vendorid, subsys_id;

  gint     cap_dac_res, cap_adc_res;
  gboolean cap_3d_enh;

  gint     curr_mic_gain;
  gboolean curr_3d_enh,
           curr_loudness,
           curr_simstereo;
  gchar   *curr_mic_select;
*/
};

struct _AlsaInfo {
    GSList *cards;
};

struct _DisplayInfo {
    /* old stuff */
    gint width, height;

    /* new stuff */
    xinfo *xi; /* x info */
    wl_info *wl; /* wayland info */

    gchar *display_server;

    /* don't free */
    const gchar *vendor; /* X vendor; points to xrr->xi->vendor */
    const gchar *session_type; /* points to wl->xdg_session_type */
};

struct _LoadInfo {
    float load1, load5, load15;
};

struct _UptimeInfo {
    int days, hours, minutes;
};

struct _Computer {
    MemoryInfo *memory;
    OperatingSystem *os;
    DisplayInfo *display;
    AlsaInfo *alsa;

    gchar *date_time;
};

struct _OperatingSystem {
    gchar *kernel;
    gchar *kcmdline;
    gchar *libc;
    gchar *distrocode;
    gchar *distroid;
    gchar *distro;
    gchar *hostname;
    gchar *language;
    gchar *homedir;
    gchar *kernel_version;

    gchar *languages;

    gchar *desktop;
    gchar *username;

    gchar *boots;

    gchar *entropy_avail;
};

struct _MemoryInfo {
    gint total, used, free, cached;
    gfloat ratio;
};

struct _Distro {
    gchar *distro;
    gchar *codename;
    gchar *id;
};


extern gchar *users;
extern gchar *groups;
extern gchar *fs_list;
extern GHashTable *_module_hash_table;
extern Computer *computer;
extern gchar *module_list;

gchar *computer_get_formatted_loadavg();
gchar *computer_get_formatted_uptime();
gchar *computer_get_alsacards(Computer * computer);
gchar *computer_get_entropy_avail(void);
gchar *computer_get_aslr(void);
gchar *computer_get_dmesg_status(void);
const gchar *computer_get_selinux(void);
gchar *computer_get_lsm(void);

OperatingSystem *computer_get_os(void);
AlsaInfo *computer_get_alsainfo(void);
MemoryInfo *computer_get_memory(void);
UptimeInfo *computer_get_uptime(void);
DisplayInfo *computer_get_display(void);
void computer_free_display(DisplayInfo *di);

void scan_modules_do(void);
void scan_filesystems(void);
void scan_users_do(void);

/* Memory Usage */
extern GHashTable *memlabels;
void init_memory_labels(void);
void scan_memory_do(void);
void scan_boots_real(void);
void scan_languages(OperatingSystem *os);
void scan_groups_do(void);

void kernel_module_icon_init(void);

#endif				/* __COMPUTER_H__ */
