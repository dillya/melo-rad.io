/*
 * melo_rad_io_plugin.c: rad.io / radio.de / radio.fr plugin for Melo
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

#include <melo_plugin.h>
#include <melo_module.h>

#include "melo_browser_rad_io.h"

static MeloModule *module = NULL;
static MeloBrowser *browser = NULL;

static gboolean
melo_rad_io_enable (void)
{

  /* Get Radio module */
  module = melo_module_get_module_by_id ("radio");
  if (!module)
    return FALSE;

  /* Create a new Rad.io browser */
  browser = melo_browser_new (MELO_TYPE_BROWSER_RAD_IO, "radio_rad.io");

  return melo_module_register_browser (module, browser);
}

static gboolean
melo_rad_io_disable (void)
{
  /* Unregister browser */
  melo_module_unregister_browser (module, "radio_rad.io");

  /* Free browser and module */
  g_object_unref (browser);
  g_object_unref (module);
  module = NULL;
  browser = NULL;

  return TRUE;
}

DECLARE_MELO_PLUGIN ("rad.io",
                     "rad.io / radio.de / radio.fr browser for Melo",
                     melo_rad_io_enable,
                     melo_rad_io_disable);
