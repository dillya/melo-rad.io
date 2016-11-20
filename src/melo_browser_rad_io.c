/*
 * melo_browser_rad_io.c: rad.io / radio.de / radio.fr browser
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

#include <string.h>

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "melo_browser_rad_io.h"

#define MELO_BROWSER_RAD_IO_URL "https://api.radio.net/info/v2"
#define MELO_BROWSER_RAD_IO_USER_AGENT "rad.io for Melo (Android API)"

/* rad.io browser info */
static MeloBrowserInfo melo_browser_rad_io_info = {
  .name = "Browse rad.io",
  .description = "Navigate though all radios from rad.io / radio.de / radio.fr",
};

static const MeloBrowserInfo *melo_browser_rad_io_get_info (
                                                          MeloBrowser *browser);
static MeloBrowserList *melo_browser_rad_io_get_list (MeloBrowser *browser,
                                                  const gchar *path,
                                                  gint offset, gint count,
                                                  MeloBrowserTagsMode tags_mode,
                                                  MeloTagsFields tags_fields);
static gboolean melo_browser_rad_io_play (MeloBrowser *browser,
                                           const gchar *path);
static gboolean melo_browser_rad_io_add (MeloBrowser *browser,
                                          const gchar *path);

struct _MeloBrowserRadIoPrivate {
  GMutex mutex;
  SoupSession *session;
};

G_DEFINE_TYPE_WITH_PRIVATE (MeloBrowserRadIo, melo_browser_rad_io, MELO_TYPE_BROWSER)

static void
melo_browser_rad_io_finalize (GObject *gobject)
{
  MeloBrowserRadIo *brad_io = MELO_BROWSER_RAD_IO (gobject);
  MeloBrowserRadIoPrivate *priv =
                             melo_browser_rad_io_get_instance_private (brad_io);

  /* Free Soup session */
  g_object_unref (priv->session);

  /* Clear mutex */
  g_mutex_clear (&priv->mutex);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (melo_browser_rad_io_parent_class)->finalize (gobject);
}

static void
melo_browser_rad_io_class_init (MeloBrowserRadIoClass *klass)
{
  MeloBrowserClass *bclass = MELO_BROWSER_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  bclass->get_info = melo_browser_rad_io_get_info;
  bclass->get_list = melo_browser_rad_io_get_list;
  bclass->play = melo_browser_rad_io_play;
  bclass->add = melo_browser_rad_io_add;

  /* Add custom finalize() function */
  oclass->finalize = melo_browser_rad_io_finalize;
}

static void
melo_browser_rad_io_init (MeloBrowserRadIo *self)
{
  MeloBrowserRadIoPrivate *priv =
                                melo_browser_rad_io_get_instance_private (self);

  self->priv = priv;

  /* Init mutex */
  g_mutex_init (&priv->mutex);

  /* Create a new Soup session */
  priv->session = soup_session_new_with_options (NULL);
}

static const MeloBrowserInfo *
melo_browser_rad_io_get_info (MeloBrowser *browser)
{
  return &melo_browser_rad_io_info;
}

static MeloBrowserList *
melo_browser_rad_io_get_list (MeloBrowser *browser, const gchar *path,
                              gint offset, gint count,
                              MeloBrowserTagsMode tags_mode,
                              MeloTagsFields tags_fields)
{
  MeloBrowserList *list;

  /* Create browser list */
  list = melo_browser_list_new (path);
  if (!list)
    return NULL;

  return list;
}

static gboolean
melo_browser_rad_io_play (MeloBrowser *browser, const gchar *path)
{
  return FALSE;
}

static gboolean
melo_browser_rad_io_add (MeloBrowser *browser, const gchar *path)
{
  return FALSE;
}
