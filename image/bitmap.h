/*
 * This file is part of NetSurf, http://netsurf.sourceforge.net/
 * Licensed under the GNU General Public License,
 *                http://www.opensource.org/licenses/gpl-license
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
 */

/** \file
 * Generic bitmap handling (interface).
 *
 * This interface wraps the native platform-specific image format, so that
 * portable image convertors can be written.
 *
 * The bitmap format is either RGBA.
 */

#ifndef _NETSURF_IMAGE_BITMAP_H_
#define _NETSURF_IMAGE_BITMAP_H_

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
	BITMAP_READY,		/** Bitmap buffer is ready */
	BITMAP_ALLOCATE_MEMORY,	/** Allocate memory */
	BITMAP_CLEAR_MEMORY,	/** Clear the memory */
} bitmap_state;

struct content;

/** An opaque image. */
struct bitmap;

struct bitmap *bitmap_create(int width, int height, bitmap_state state);
void bitmap_set_opaque(struct bitmap *bitmap, bool opaque);
bool bitmap_test_opaque(struct bitmap *bitmap);
bool bitmap_get_opaque(struct bitmap *bitmap);
char *bitmap_get_buffer(struct bitmap *bitmap);
size_t bitmap_get_rowstride(struct bitmap *bitmap);
void bitmap_destroy(struct bitmap *bitmap);
bool bitmap_save(struct bitmap *bitmap, const char *path);
void bitmap_modified(struct bitmap *bitmap);

#endif
