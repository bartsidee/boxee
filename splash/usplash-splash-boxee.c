/* usplash
 *
 * eft-theme.c - definition of eft theme
 *
 * Copyright © 2006 Dennis Kaarsemaker <dennis@kaarsemaker.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
  * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <usplash-theme.h>
/* Needed for the custom drawing functions */
#include <usplash_backend.h>
extern struct usplash_pixmap pixmap_throbber_back;
extern struct usplash_pixmap pixmap_throbber_fore;
extern struct usplash_pixmap pixmap_frame01;
extern struct usplash_pixmap pixmap_frame02;
extern struct usplash_pixmap pixmap_frame03;
extern struct usplash_pixmap pixmap_frame04;
extern struct usplash_pixmap pixmap_frame05;
extern struct usplash_pixmap pixmap_frame06;
extern struct usplash_pixmap pixmap_frame07;
extern struct usplash_pixmap pixmap_frame08;
extern struct usplash_pixmap pixmap_frame09;
extern struct usplash_pixmap pixmap_frame10;
extern struct usplash_pixmap pixmap_frame11;
extern struct usplash_pixmap pixmap_frame12;
extern struct usplash_pixmap pixmap_frame13;
extern struct usplash_pixmap pixmap_frame14;
extern struct usplash_pixmap pixmap_frame15;
extern struct usplash_pixmap pixmap_frame16;
extern struct usplash_pixmap pixmap_frame17;
extern struct usplash_pixmap pixmap_frame18;
extern struct usplash_pixmap pixmap_frame19;
extern struct usplash_pixmap pixmap_frame20;
extern struct usplash_pixmap pixmap_frame21;
extern struct usplash_pixmap pixmap_frame22;
extern struct usplash_pixmap pixmap_frame23;
extern struct usplash_pixmap pixmap_frame24;
extern struct usplash_pixmap pixmap_frame25;
extern struct usplash_pixmap pixmap_frame26;
extern struct usplash_pixmap pixmap_frame27;
extern struct usplash_pixmap pixmap_frame28;
extern struct usplash_pixmap pixmap_frame29;
extern struct usplash_pixmap pixmap_frame30;
extern struct usplash_pixmap pixmap_frame31;
extern struct usplash_pixmap pixmap_frame32;
extern struct usplash_pixmap pixmap_frame33;
extern struct usplash_pixmap pixmap_frame34;
extern struct usplash_pixmap pixmap_frame35;
extern struct usplash_pixmap pixmap_frame36;
extern struct usplash_pixmap pixmap_frame37;
extern struct usplash_pixmap pixmap_frame38;
extern struct usplash_pixmap pixmap_frame39;
extern struct usplash_pixmap pixmap_frame40;

struct usplash_theme usplash_theme_640_480;
struct usplash_theme usplash_theme_800_600;
struct usplash_theme usplash_theme_1024_768;
struct usplash_theme usplash_theme_1365_768_scaled;

extern int usplash_xres, usplash_yres;

void t_init(struct usplash_theme* theme);
void t_clear_screen(struct usplash_theme* theme);
void t_clear_progressbar(struct usplash_theme* theme);
void t_draw_progressbar(struct usplash_theme* theme, int percentage);
void t_animate_step(struct usplash_theme* theme, int pulsating);

signed int anim_y;

struct usplash_pixmap* frames[] = { &pixmap_frame01, &pixmap_frame02, &pixmap_frame03, &pixmap_frame04, &pixmap_frame05, &pixmap_frame06, &pixmap_frame07, &pixmap_frame08,
   &pixmap_frame09, &pixmap_frame10, &pixmap_frame11, &pixmap_frame12, &pixmap_frame13, &pixmap_frame14, &pixmap_frame15, &pixmap_frame16, &pixmap_frame18, &pixmap_frame19,
   &pixmap_frame20, &pixmap_frame21, &pixmap_frame22, &pixmap_frame23, &pixmap_frame24, &pixmap_frame25, &pixmap_frame26, &pixmap_frame27, &pixmap_frame28, &pixmap_frame29,
   &pixmap_frame30, &pixmap_frame31, &pixmap_frame32, &pixmap_frame32, &pixmap_frame33, &pixmap_frame34, &pixmap_frame34, &pixmap_frame35, &pixmap_frame36, &pixmap_frame37,
   &pixmap_frame38, &pixmap_frame39, &pixmap_frame40};

/* Theme definition */
struct usplash_theme usplash_theme = {
	.version = THEME_VERSION, /* ALWAYS set this to THEME_VERSION, 
                                 it's a compatibility check */
    .next = &usplash_theme_640_480,
    .ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_frame40,

	/* Palette indexes */
	.background             = 0,
  	.progressbar_background = 32,
  	.progressbar_foreground = 131,
	.text_background        = 0,
	.text_foreground        = 117,
	.text_success           = 189,
	.text_failure           = 55,

	/* Progress bar position and size in pixels */
  	.progressbar_x      = 212,
  	.progressbar_y      = 196,
  	.progressbar_width  = 216,
  	.progressbar_height = 8,

	/* Text box position and size in pixels */
  	.text_x      = 96,
  	.text_y      = 246,
  	.text_width  = 360,
  	.text_height = 100,

	/* Text details */
  	.line_height  = 15,
  	.line_length  = 32,
  	.status_width = 35,

    /* Functions */
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
    .clear_screen = t_clear_screen,
};

struct usplash_theme usplash_theme_640_480 = {
	.version = THEME_VERSION, /* ALWAYS set this to THEME_VERSION, 
                                 it's a compatibility check */
    .next = &usplash_theme_800_600,
    .ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_frame40,

	/* Palette indexes */
	.background             = 0,
  	.progressbar_background = 32,
  	.progressbar_foreground = 131,
	.text_background        = 0,
	.text_foreground        = 117,
	.text_success           = 189,
	.text_failure           = 55,

	/* Progress bar position and size in pixels */
  	.progressbar_x      = 100,
  	.progressbar_y      = 251,
  	.progressbar_width  = 320,
  	.progressbar_height = 18,

	/* Text box position and size in pixels */
  	.text_x      = 120,
  	.text_y      = 307,
  	.text_width  = 360,
  	.text_height = 100,

	/* Text details */
  	.line_height  = 15,
  	.line_length  = 32,
  	.status_width = 35,

    /* Functions */
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
    .clear_screen = t_clear_screen,    
};

struct usplash_theme usplash_theme_800_600 = {
	.version = THEME_VERSION, /* ALWAYS set this to THEME_VERSION, 
                                 it's a compatibility check */
    .next = &usplash_theme_1024_768,
    .ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_frame40,

	/* Palette indexes */
	.background             = 0,
  	.progressbar_background = 32,
  	.progressbar_foreground = 131,
	.text_background        = 0,
	.text_foreground        = 117,
	.text_success           = 189,
	.text_failure           = 55,

	/* Progress bar position and size in pixels */
  	.progressbar_x      = 240,
  	.progressbar_y      = 321,
  	.progressbar_width  = 320,
  	.progressbar_height = 18,

	/* Text box position and size in pixels */
  	.text_x      = 220,
  	.text_y      = 407,
  	.text_width  = 360,
  	.text_height = 150,

	/* Text details */
  	.line_height  = 15,
  	.line_length  = 32,
  	.status_width = 35,

    /* Functions */
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
    .clear_screen = t_clear_screen,    
};

struct usplash_theme usplash_theme_1024_768 = {
	.version = THEME_VERSION,
    .next = &usplash_theme_1365_768_scaled,
    .ratio = USPLASH_4_3,

	/* Background and font */
	.pixmap = &pixmap_frame40,

	/* Palette indexes */
	.background             = 0,
  	.progressbar_background = 32,
  	.progressbar_foreground = 131,
	.text_background        = 0,
	.text_foreground        = 117,
	.text_success           = 189,
	.text_failure           = 55,

	/* Progress bar position and size in pixels */
  	.progressbar_x      = 352,
  	.progressbar_y      = 400,
  	.progressbar_width  = 320,
  	.progressbar_height = 18,

	/* Text box position and size in pixels */
  	.text_x      = 322,
  	.text_y      = 475,
  	.text_width  = 380,
  	.text_height = 200,

	/* Text details */
  	.line_height  = 15,
  	.line_length  = 32,
  	.status_width = 35,

    /* Functions */
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
    .clear_screen = t_clear_screen,    
};

struct usplash_theme usplash_theme_1365_768_scaled = {
	.version = THEME_VERSION,
    .next = NULL,
    .ratio = USPLASH_16_9,

	/* Background and font */
	.pixmap = &pixmap_frame40,

	/* Palette indexes */
	.background             = 0,
  	.progressbar_background = 32,
  	.progressbar_foreground = 131,
	.text_background        = 0,
	.text_foreground        = 117,
	.text_success           = 189,
	.text_failure           = 55,

	/* Progress bar position and size in pixels */
  	.progressbar_x      = 352,
  	.progressbar_y      = 475,
  	.progressbar_width  = 320,
  	.progressbar_height = 18,

	/* Text box position and size in pixels */
  	.text_x      = 322,
  	.text_y      = 475,
  	.text_width  = 380,
  	.text_height = 200,

	/* Text details */
  	.line_height  = 15,
  	.line_length  = 32,
  	.status_width = 35,

    /* Functions */
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
    .clear_screen = t_clear_screen,    
};

void t_init(struct usplash_theme *theme) {
    int x, y;
    
    usplash_getdimensions(&x, &y);
    anim_y = (y / 2) - theme->pixmap->height;
    if (anim_y < 0)
       anim_y = 0;
    
    theme->progressbar_x = (x - pixmap_throbber_back.width)/2;
    theme->progressbar_y = anim_y + theme->pixmap->height + 10;    
}

void t_clear_progressbar(struct usplash_theme *theme) {
    t_draw_progressbar(theme, 0);
}

void t_draw_progressbar(struct usplash_theme *theme, int percentage) {
    int w = (pixmap_throbber_back.width * percentage / 100);
    usplash_put(theme->progressbar_x, theme->progressbar_y, &pixmap_throbber_back);
    if(percentage == 0)
        return;
    if(percentage < 0)
        usplash_put_part(theme->progressbar_x - w, theme->progressbar_y, pixmap_throbber_back.width + w,
                         pixmap_throbber_back.height, &pixmap_throbber_fore, -w, 0);
    else
        usplash_put_part(theme->progressbar_x, theme->progressbar_y, w, pixmap_throbber_back.height, 
                         &pixmap_throbber_fore, 0, 0);
}

#define ANIM_FRAMES 40
#define F_BEFORE 25

void t_animate_step(struct usplash_theme* theme, int pulsating) {

    static int pulsate_step = 0;
    static int pulse_width = 56;
    static int step_width = 2;
    static int num_steps = 0;
    static int frame = 0;
    int x, y;
    int x1;
    num_steps = (pixmap_throbber_fore.width - pulse_width)/2;

    usplash_getdimensions(&x, &y);
    frame++;
    if (frame >= F_BEFORE && frame < (F_BEFORE + (ANIM_FRAMES * 2)))
       usplash_put((x - theme->pixmap->width) / 2, anim_y , frames[(frame - F_BEFORE) / 2]);
    else if (frame >= F_BEFORE + (ANIM_FRAMES * 2))
       usplash_put((x - theme->pixmap->width) / 2, anim_y, frames[ANIM_FRAMES]);

    if (pulsating) {
        t_draw_progressbar(theme, 0);
    
        if(pulsate_step < num_steps/2+1)
	        x1 = 2 * step_width * pulsate_step;
        else
	        x1 = pixmap_throbber_fore.width - pulse_width - 2 * step_width * (pulsate_step - num_steps/2+1);

        usplash_put_part(theme->progressbar_x + x1, theme->progressbar_y, pulse_width,
                         pixmap_throbber_fore.height, &pixmap_throbber_fore, x1, 0);

        pulsate_step = (pulsate_step + 1) % num_steps;
    }
}

void t_clear_screen(struct usplash_theme* theme)
{
   usplash_clear(0, 0, usplash_xres, usplash_yres, theme->background);
}
