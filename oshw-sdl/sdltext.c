/* sdltext.c: Font-rendering functions for SDL.
 *
 * Copyright (C) 2001,2002 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"SDL.h"
#include	"sdlgen.h"
#include	"../err.h"

/* Accept a bitmap (as an 8-bit SDL surface) and from it extract the
 * glyphs of a font.
 */
static int makefontfromsurface(fontinfo *pf, SDL_Surface *surface)
{
    char		brk[256];
    unsigned char      *p;
    unsigned char      *dest;
    Uint8		foregnd, bkgnd;
    int			pitch, wsum;
    int			count, ch;
    int			x, y, x0, y0, w;

    if (surface->format->BytesPerPixel != 1)
	return FALSE;

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

    pitch = surface->pitch;
    p = surface->pixels;
    foregnd = p[0];
    bkgnd = p[pitch];
    for (y = 1, p += pitch ; y < surface->h && *p == bkgnd ; ++y, p += pitch) ;
    pf->h = y - 1;

    wsum = 0;
    ch = 32;
    memset(pf->w, 0, sizeof pf->w);
    memset(brk, 0, sizeof brk);
    for (y = 0 ; y + pf->h < surface->h && ch < 256 ; y += pf->h + 1) {
	p = surface->pixels;
	p += y * pitch;
	x0 = 1;
	for (x = 1 ; x < surface->w ; ++x) {
	    if (p[x] == bkgnd)
		continue;
	    w = x - x0;
	    x0 = x + 1;
	    pf->w[ch] = w;
	    wsum += w;
	    ++ch;
	    if (ch == 127)
		ch = 160;
	}
	brk[ch] = 1;
    }

    count = ch;
    if (!(pf->memory = calloc(wsum, pf->h)))
	memerrexit();

    x0 = 1;
    y0 = 1;
    dest = pf->memory;
    for (ch = 0 ; ch < 256 ; ++ch) {
	pf->bits[ch] = dest;
	if (pf->w[ch] == 0)
	    continue;
	if (brk[ch]) {
	    x0 = 1;
	    y0 += pf->h + 1;
	}
	p = surface->pixels;
	p += y0 * pitch + x0;
	for (y = 0 ; y < pf->h ; ++y, p += pitch)
	    for (x = 0 ; x < pf->w[ch] ; ++x, ++dest)
		*dest = p[x] == bkgnd ? 0 : p[x] == foregnd ? 2 : 1;
	x0 += pf->w[ch] + 1;
    }

    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);

    return TRUE;
}

/*
 *
 */

/* Given a text and a maximum horizontal space to occupy, return
 * the amount of vertial space needed to render the entire text with
 * word-wrapping.
 */
static int measuremltext(unsigned char const *text, int len, int maxwidth)
{
    int	brk, w, h, n;

    if (len < 0)
	len = strlen(text);
    h = 0;
    brk = 0;
    for (n = 0, w = 0 ; n < len ; ++n) {
	w += sdlg.font.w[text[n]];
	if (isspace(text[n])) {
	    brk = w;
	} else if (w > maxwidth) {
	    h += sdlg.font.h;
	    if (brk) {
		w -= brk;
		brk = 0;
	    } else {
		w = sdlg.font.w[text[n]];
		brk = 0;
	    }
	}
    }
    if (w)
	h += sdlg.font.h;
    return h;
}

/*
 * The internal font-drawing functions, one per surface type.
 */

static void drawtextscanline8(Uint8 *scanline, int w, int y, Uint32 *clr,
			      unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w)
	    scanline[x] = (Uint8)clr[glyph[x]];
	scanline += x;
    }
    if (w)
	memset(scanline, clr[0], w);
}

static void drawtextscanline16(Uint16 *scanline, int w, int y, Uint32 *clr,
			       unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; x < sdlg.font.w[text[n]] ; ++x)
	    scanline[x] = (Uint16)clr[glyph[x]];
	scanline += x;
    }
    while (w--)
	scanline[w] = (Uint16)clr[0];
}

static void drawtextscanline24(Uint8 *scanline, int w, int y, Uint32 *clr,
			       unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    Uint32			c;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w) {
	    c = clr[glyph[x]];
	    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		*scanline++ = (Uint8)(c >> 16);
		*scanline++ = (Uint8)(c >> 8);
		*scanline++ = (Uint8)c;
	    } else {
		*scanline++ = (Uint8)c;
		*scanline++ = (Uint8)(c >> 8);
		*scanline++ = (Uint8)(c >> 16);
	    }
	}
    }
    c = clr[0];
    while (w--) {
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    *scanline++ = (Uint8)(c >> 16);
	    *scanline++ = (Uint8)(c >> 8);
	    *scanline++ = (Uint8)c;
	} else {
	    *scanline++ = (Uint8)c;
	    *scanline++ = (Uint8)(c >> 8);
	    *scanline++ = (Uint8)(c >> 16);
	}
    }
}

static void *drawtextscanline32(Uint32 *scanline, int w, int y, Uint32 *clr,
				unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w)
	    scanline[x] = clr[glyph[x]];
	scanline += x;
    }
    while (w--)
	*scanline++ = clr[0];
    return scanline;
}

/*
 * The font-rendering functions.
 */

/* Draw a single line of text to the screen at the position given by
 * the given rectangle and modified according to the given flags.
 */
static void drawtext(SDL_Rect *rect, unsigned char const *text,
		     int len, int flags)
{
    Uint32     *clr;
    void       *p;
    void       *q;
    int		l, r;
    int		pitch, bpp, n, w, y;

    if (len < 0)
	len = text ? strlen(text) : 0;

    w = 0;
    for (n = 0 ; n < len ; ++n)
	w += sdlg.font.w[text[n]];
    if (flags & PT_CALCSIZE) {
	rect->h = sdlg.font.h;
	rect->w = w;
	return;
    }
    if (w >= rect->w) {
	w = rect->w;
	l = r = 0;
    } else if (flags & PT_RIGHT) {
	l = rect->w - w;
	r = 0;
    } else if (flags & PT_CENTER) {
	l = (rect->w - w) / 2;
	r = (rect->w - w) - l;
    } else {
	l = 0;
	r = rect->w - w;
    }

    if (flags & PT_DIM)
	clr = sdlg.dimtextclr.c;
    else if (flags & PT_HILIGHT)
	clr = sdlg.hilightclr.c;
    else
	clr = sdlg.textclr.c;

    pitch = sdlg.screen->pitch;
    bpp = sdlg.screen->format->BytesPerPixel;
    p = (unsigned char*)sdlg.screen->pixels + rect->y * pitch + rect->x * bpp;
    for (y = 0 ; y < sdlg.font.h && y < rect->h ; ++y) {
	switch (bpp) {
	  case 1: drawtextscanline8(p, w, y, clr, text, len);  break;
	  case 2: drawtextscanline16(p, w, y, clr, text, len); break;
	  case 3: drawtextscanline24(p, w, y, clr, text, len); break;
	  case 4:
	    q = drawtextscanline32(p, l, y, clr, "", 0);
	    q = drawtextscanline32(q, w, y, clr, text, len);
	    q = drawtextscanline32(q, r, y, clr, "", 0);
	    break;
	}
	p = (unsigned char*)p + pitch;
    }
    if (flags & PT_UPDATERECT) {
	rect->y += y;
	rect->h -= y;
    }
}

/* Draw one or more lines of text to the screen at the position given
 * by the given rectangle and modified according to the given flags,
 * breaking the text between words (when possible).
 */
static void drawmultilinetext(SDL_Rect *rect, unsigned char const *text,
			      int len, int flags)
{
    SDL_Rect	area;
    int		index, brkw, brkn;
    int		w, n;

    if (flags & PT_CALCSIZE) {
	rect->h = measuremltext(text, len, rect->w);
	return;
    }

    if (len < 0)
	len = strlen(text);

    area = *rect;
    brkw = brkn = 0;
    index = 0;
    for (n = 0, w = 0 ; n < len ; ++n) {
	w += sdlg.font.w[text[n]];
	if (isspace(text[n])) {
	    brkn = n;
	    brkw = w;
	} else if (w > rect->w) {
	    if (brkw) {
		drawtext(&area, text + index, brkn - index,
				 flags | PT_UPDATERECT);
		index = brkn + 1;
		w -= brkw;
	    } else {
		drawtext(&area, text + index, n - index,
				 flags | PT_UPDATERECT);
		w = sdlg.font.w[text[n]];
	    }
	    brkw = 0;
	}
    }
    if (w)
	drawtext(&area, text + index, len - index, flags | PT_UPDATERECT);
    if (flags & PT_UPDATERECT) {
	*rect = area;
    } else {
	while (area.h)
	    drawtext(&area, "", 0, PT_UPDATERECT);
    }
}

/*
 * The exported functions.
 */

/* Render a string of text.
 */
static void _puttext(SDL_Rect *rect, char const *text, int len, int flags)
{
    if (!sdlg.font.h)
	die("No font!");

    if (len < 0)
	len = text ? strlen(text) : 0;

    if (SDL_MUSTLOCK(sdlg.screen))
	SDL_LockSurface(sdlg.screen);

    if (flags & PT_MULTILINE)
	drawmultilinetext(rect, (unsigned char const*)text, len, flags);
    else
	drawtext(rect, (unsigned char const*)text, len, flags);

    if (SDL_MUSTLOCK(sdlg.screen))
	SDL_UnlockSurface(sdlg.screen);
}

/* Calculate the widths of each column in the given table.
 */
static SDL_Rect *_measuretable(SDL_Rect const *area, tablespec const *table)
{
    SDL_Rect		       *cols;
    unsigned char const	       *p;
    int				sep, ml;
    int				n, i, j, w, x, c;

    if (!(cols = malloc(table->cols * sizeof *cols)))
	memerrexit();
    for (i = 0 ; i < table->cols ; ++i) {
	cols[i].x = 0;
	cols[i].y = area->y;
	cols[i].w = 0;
	cols[i].h = area->h;
    }

    ml = -1;
    n = 0;
    for (j = 0 ; j < table->rows ; ++j) {
	for (i = 0 ; i < table->cols ; ++n) {
	    c = table->items[n][0] - '0';
	    if (c == 1) {
		if (table->items[n][1] == '!') {
		    ml = i;
		} else {
		    w = 0;
		    for (p = table->items[n] + 2 ; *p ; ++p)
			w += sdlg.font.w[*p];
		    if (w > cols[i].w)
			cols[i].w = w;
		}
	    }
	    i += c;
	}
    }
    sep = sdlg.font.w[' '] * table->sep;
    w = -sep;
    for (i = 0 ; i < table->cols ; ++i)
	w += cols[i].w + sep;
    if (w > area->w) {
	w -= area->w;
	if (table->collapse >= 0 && cols[table->collapse].w > w)
	    cols[table->collapse].w -= w;
    } else if (ml >= 0) {
	cols[ml].w += area->w - w;
    }

    x = 0;
    for (i = 0 ; i < table->cols && x < area->w ; ++i) {
	cols[i].x = area->x + x;
	x += cols[i].w + sep;
	if (x >= area->w)
	    cols[i].w = area->x + area->w - cols[i].x;
    }
    for ( ; i < table->cols ; ++i) {
	cols[i].x = area->x + area->w;
	cols[i].w = 0;
    }

    return cols;
}

/* Render a single row of a table.
 */
static int _drawtablerow(tablespec const *table, SDL_Rect *cols,
			 int *row, int flags)
{
    SDL_Rect			rect;
    unsigned char const	       *p;
    int				c, f, n, i, y;

    if (!cols) {
	for (i = 0 ; i < table->cols ; i += table->items[(*row)++][0] - '0') ;
	return TRUE;
    }

    if (SDL_MUSTLOCK(sdlg.screen))
	SDL_LockSurface(sdlg.screen);

    y = cols[0].y;
    n = *row;
    for (i = 0 ; i < table->cols ; ++n) {
	p = table->items[n];
	c = p[0] - '0';
	rect = cols[i];
	i += c;
	if (c > 1)
	    rect.w = cols[i - 1].x + cols[i - 1].w - rect.x;
	f = flags | PT_UPDATERECT;
	if (p[1] == '+')
	    f |= PT_RIGHT;
	else if (p[1] == '.')
	    f |= PT_CENTER;
	if (p[1] == '!')
	    drawmultilinetext(&rect, p + 2, -1, f);
	else
	    drawtext(&rect, p + 2, -1, f);
	if (rect.y > y)
	    y = rect.y;
    }

    if (SDL_MUSTLOCK(sdlg.screen))
	SDL_UnlockSurface(sdlg.screen);

    *row = n;
    for (i = 0 ; i < table->cols ; ++i) {
	cols[i].h -= y - cols[i].y;
	cols[i].y = y;
    }

    return TRUE;
}

/* Free the resources associated with a font.
 */
void freefont(void)
{
    if (sdlg.font.h) {
	free(sdlg.font.memory);
	sdlg.font.memory = NULL;
	sdlg.font.h = 0;
    }
}

/* Load a proportional font from the given bitmap file.
 */
int loadfontfromfile(char const *filename, int complain)
{
    SDL_Surface	       *bmp;
    fontinfo		font;

    bmp = SDL_LoadBMP(filename);
    if (!bmp) {
	if (complain)
	    errmsg(filename, "can't load font bitmap: %s", SDL_GetError());
	return FALSE;
    }
    if (!makefontfromsurface(&font, bmp)) {
	if (complain)
	    errmsg(filename, "invalid font file");
	return FALSE;
    }
    SDL_FreeSurface(bmp);
    freefont();
    sdlg.font = font;
    return TRUE;
}

/* Initialize the module.
 */
int _sdltextinitialize(void)
{
    sdlg.font.h = 0;
    sdlg.puttextfunc = _puttext;
    sdlg.measuretablefunc = _measuretable;
    sdlg.drawtablerowfunc = _drawtablerow;
    return TRUE;
}
