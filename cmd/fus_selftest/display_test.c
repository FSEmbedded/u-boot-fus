/*
 * display_test.c
 *
 *  Created on: May 14, 2020
 *      Author: developer
 */

#include <common.h>
#include <command.h>
#include "dram_test.h"
#include <video.h>

#include <xlcd_draw_ll.h>		  /* draw_ll_*() */
#include <cmd_lcd.h>			  /* wininfo_t, kwinfo_t, ... */

extern void lcd_line(const wininfo_t *pwi, XYPOS x1, XYPOS y1, XYPOS x2, XYPOS y2,
	      const colinfo_t *pci);

extern void lcd_rframe(const wininfo_t *pwi, XYPOS x1, XYPOS y1, XYPOS x2, XYPOS y2,
		XYPOS r, const colinfo_t *pci);

COLOR32 rgba2col(const wininfo_t *pwi, RGBA rgba){
	return (COLOR32) (rgba>>8| rgba<<24);
}

COLOR32 rgb2col(u32 r, u32 g, u32 b){
	return (COLOR32) ( 0xFF << 24 | r << 16 | g << 8 | b );
}

COLOR32 hue(int x)
{
    if (x < 60)
        return rgb2col(255, x*255/60, 0);
    if (x < 120)
        return rgb2col(255-(x-60)*255/60, 255, 0);
    if (x < 180)
        return rgb2col(0, 255, (x-120)*255/60);
    if (x < 240)
        return rgb2col(0, 255-(x-180)*255/60, 255);
    if (x < 300)
        return rgb2col((x-240)*255/60, 0, 255);
    return rgb2col(255, 0, 255-(x-300)*255/60);
}

static void set_sn65dsi84_EDT0350(wininfo_t *pwi, pixinfo_t * ppi, u_long *fb ){

	ppi->depth = 24;
	ppi->bpp_shift = 5;
	ppi->rgba2col= rgba2col;
	pwi->fbhres = 320;
	pwi->fbvres = 240;

	pwi->pfbuf = fb;

	pwi->fbdraw = 0;
	pwi->linelen = ((u_long)pwi->fbhres << ppi->bpp_shift) >> 3;

	pwi->clip_left =  0;
	pwi->clip_top =  0;
	pwi->clip_right =  pwi->fbhres;
	pwi->clip_bottom =  pwi->fbvres;
	pwi->ppi = ppi;

}

static void set_tc358775_j070wvtc0211(wininfo_t *pwi, pixinfo_t * ppi, u_long *fb ){

	ppi->depth = 24;
	ppi->bpp_shift = 5;
	ppi->rgba2col= rgba2col;
	pwi->fbhres = 800;
	pwi->fbvres = 480;

	pwi->pfbuf = fb;

	pwi->fbdraw = 0;
	pwi->linelen = ((u_long)pwi->fbhres << ppi->bpp_shift) >> 3;

	pwi->clip_left =  0;
	pwi->clip_top =  0;
	pwi->clip_right =  pwi->fbhres;
	pwi->clip_bottom =  pwi->fbvres;
	pwi->ppi = ppi;

}

void draw_test_screen(const wininfo_t *pwi,
			  XYPOS x1, XYPOS y1, XYPOS x2, XYPOS y2)
{
	const pixinfo_t *ppi = pwi->ppi;
	XYPOS lcd_grid;
	XYPOS x, y;
	XYPOS r1, r2;
	XYPOS hres, vres;
	COLOR32 col;
	colinfo_t ci;

	static const RGBA coltab[] = {
		0xFF0000FF,		  /* R */
		0x00FF00FF,		  /* G */
		0x0000FFFF,		  /* B */
		0xFFFF00FF,		  /* Y */
		0xFF00FFFF,		  /* M */
		0x00FFFFFF,		  /* C */
		0xFFFFFFFF,		  /* W */
	};


	/* Use hres divided by 12 and vres divided by 8 as grid size */
	hres = x2-x1+1;
	vres = y2-y1+1;

	lcd_grid = hres/16;

	/* Draw lines and circles in white; the circle command needs a colinfo
	   structure for the color; however we know that ATTR_ALPHA is cleared
	   so it is enough to set the col entry of this structure. */
	col = ppi->rgba2col(pwi, 0xFFFFFFFF);  /* White */
	ci.col = col;

	for(y = (vres / 2) + (lcd_grid / 2); y < vres; y+= lcd_grid)
	{
		draw_ll_rect(pwi, x1, y, x2, y, col);
	}
	for(y = (vres / 2) - (lcd_grid / 2); y > 0; y-= lcd_grid)
	{
		draw_ll_rect(pwi, x1, y, x2, y, col);
	}

    for (x = lcd_grid/2; x < hres; x += lcd_grid)
    {
    	draw_ll_rect(pwi, x, y1, x, y2, col);
    }



	/* Draw corners; the window is min. 24x16, so +/-7 will always fit */
	col = ppi->rgba2col(pwi, 0x00FF00FF);  /* Green */
	draw_ll_rect(pwi, x1, y1, x1+10, y1, col); /* top left */
	draw_ll_rect(pwi, x1, y1, x1, y1+10, col);
	draw_ll_rect(pwi, x2-10, y1, x2, y1, col); /* top right */
	draw_ll_rect(pwi, x2, y1, x2, y1+10, col);
	draw_ll_rect(pwi, x1, y2-10, x1, y2, col); /* bottom left */
	draw_ll_rect(pwi, x1, y2, x1+10, y2, col);
	draw_ll_rect(pwi, x2, y2-10, x2, y2, col); /* bottom right */
	draw_ll_rect(pwi, x2-10, y2, x2, y2, col);

	/* Draw basic colors */

	/* Red */
	XYPOS c_x0 =  ((hres / 2) - (lcd_grid / 2) - (3 * lcd_grid)) + 1;
	XYPOS c_y0 =  ((vres / 2) - (lcd_grid / 2) - (2 * lcd_grid)) + 1;
	XYPOS c_x1 =  ((hres / 2) - (lcd_grid / 2) - (2 * lcd_grid));
	XYPOS c_y1 = (((vres / 2) - (lcd_grid / 2) - (2 * lcd_grid)) + (2*lcd_grid)) -1;

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[0]));

	/* Green */
	c_x0 =  ((hres / 2) - (lcd_grid / 2) - (2 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) - (lcd_grid / 2) - (1 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[1]));

	/* Blue */
	c_x0 =  ((hres / 2) - (lcd_grid / 2) - (1 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) - (lcd_grid / 2) - (0 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[2]));

	/* Yellow */
	c_x0 =  ((hres / 2) - (lcd_grid / 2) - (0 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) + (lcd_grid / 2) - (0 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[3]));

	/* Magenta */
	c_x0 =  ((hres / 2) + (lcd_grid / 2) - (0 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) + (lcd_grid / 2) + (1 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[4]));

	/* Cyan */
	c_x0 =  ((hres / 2) + (lcd_grid / 2) + (1 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) + (lcd_grid / 2) + (2 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[5]));

	/* White */
	c_x0 =  ((hres / 2) + (lcd_grid / 2) + (2 * lcd_grid)) + 1;
	c_x1 =  ((hres / 2) + (lcd_grid / 2) + (3 * lcd_grid));

	draw_ll_rect(pwi, c_x0, c_y0, c_x1, c_y1, ppi->rgba2col(pwi, coltab[6]));

    // Gray scale bar
    for (int x=0; x < 256; x++){
    	RGBA rgba;
    	c_x0 =((hres - 256) / 2) + x;

		rgba = (x) << 8;
		rgba |= (rgba << 8) | (rgba << 16) | 0xFF;
    	draw_ll_rect(pwi, c_x0, ((vres / 2) + (lcd_grid/2)) + 1, c_x0, ((vres / 2) + (lcd_grid/2) + lcd_grid), ppi->rgba2col(pwi, rgba));
    }

    // Gray hue bar
    for (int x=0; x < 256; x++){
    	c_x0 =((hres - 256) / 2) + x;
    	draw_ll_rect(pwi, c_x0, ((vres / 2) + (lcd_grid/2)) + lcd_grid, c_x0, ((vres / 2) + (lcd_grid/2) + 2* lcd_grid), hue( x*359/255));
    }


    lcd_line(pwi,hres/2-vres/2, 0, hres/2-vres/2+vres-1, vres-1, &ci);
    lcd_line(pwi,hres/2-vres/2+vres, 0, hres/2-vres/2+1, vres-1, &ci);

	/* Draw big and small circle; make sure that circle fits on screen */
	if (hres > vres) {
		r2 = (vres/2);
	} else {
		r2 = (hres/2);;
	}
	r1 = 40;
	x = hres/2 + x1;
	y = vres/2 + y1;
	/* Draw two circles */
	lcd_rframe(pwi, x-r1, y-r1, x+r1-1, y+r1-1, r1, &ci);
	lcd_rframe(pwi, x-r2, y-r2, x+r2-1, y+r2-1, r2, &ci);

    return;



}


int test_display(char * szStrBuffer){

	wininfo_t pwi;
	pixinfo_t ppi;
	DECLARE_GLOBAL_DATA_PTR;
	static u_long fb;

	/* Get frame buffer poitner */
	fb = gd->fb_base;

	//set_sn65dsi84_EDT0350(&pwi,&ppi, &fb);
	set_tc358775_j070wvtc0211(&pwi,&ppi, &fb);
	video_clear();

	draw_test_screen(&pwi,0,0,pwi.fbhres-1 ,pwi.fbvres-1);

	flush_cache(fb, pwi.fbhres*pwi.fbvres*4);

	return 0;

}







