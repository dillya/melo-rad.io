/*
 * melo_browser_rad_io.h: rad.io / radio.de / radio.fr browser
 *
 * Copyright (C) 2016 Alexandre Dilly <dillya@sparod.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __MELO_BROWSER_RAD_IO_H__
#define __MELO_BROWSER_RAD_IO_H__

#include "melo_browser.h"

G_BEGIN_DECLS

#define MELO_TYPE_BROWSER_RAD_IO             (melo_browser_rad_io_get_type ())
#define MELO_BROWSER_RAD_IO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MELO_TYPE_BROWSER_RAD_IO, MeloBrowserRadIo))
#define MELO_IS_BROWSER_RAD_IO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MELO_TYPE_BROWSER_RAD_IO))
#define MELO_BROWSER_RAD_IO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MELO_TYPE_BROWSER_RAD_IO, MeloBrowserRadIoClass))
#define MELO_IS_BROWSER_RAD_IO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MELO_TYPE_BROWSER_RAD_IO))
#define MELO_BROWSER_RAD_IO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MELO_TYPE_BROWSER_RAD_IO, MeloBrowserRadIoClass))

typedef struct _MeloBrowserRadIo MeloBrowserRadIo;
typedef struct _MeloBrowserRadIoClass MeloBrowserRadIoClass;
typedef struct _MeloBrowserRadIoPrivate MeloBrowserRadIoPrivate;

struct _MeloBrowserRadIo {
  MeloBrowser parent_instance;

  /*< private >*/
  MeloBrowserRadIoPrivate *priv;
};

struct _MeloBrowserRadIoClass {
  MeloBrowserClass parent_class;
};

GType melo_browser_rad_io_get_type (void);

G_END_DECLS

#endif /* __MELO_BROWSER_RAD_IO_H__ */
