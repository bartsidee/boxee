/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler 
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * The author can be reached at: handygewinnspiel AT gmx DOT de
 *
 * The project's page is http://wirbel.htpc-forum.de/w_scan/index2.html
 */

#ifndef __DUMP_KAFFEINE_H__
#define __DUMP_KAFFEINE_H__

/* 20090323 --wk */

#include <stdint.h>
#include <linux/dvb/frontend.h>
#include "scan.h"

extern void kaffeine_dump_service_parameter_set (FILE *f,
                                const char *service_name,
                                const char *provider_name,
                                struct extended_dvb_frontend_parameters *p,
                                int video_pid,
                                int pcr_pid,
                                uint16_t *audio_pid,
                                int audio_num,
                                int teletext_pid,
                                int scrambled,
                                uint16_t *ac3_pid,
                                int ac3_num,
                                int service_id,
                                struct transponder_ids tr_ids,
                                int ca_select,
                                int channel_num,
                                struct w_scan_flags * flags);

#endif

