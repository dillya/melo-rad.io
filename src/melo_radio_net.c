/*
 * Copyright (C) 2020 Alexandre Dilly <dillya@sparod.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <stddef.h>

#include <melo/melo_module.h>

#define MELO_LOG_TAG "melo_radio_net"
#include <melo/melo_log.h>

#include "melo_radio_net_browser.h"

#define MELO_RADIO_NET_ID "net.radio"

static MeloRadioNetBrowser *browser;

static void
melo_radio_net_enable (void)
{
  /* Create radio.net browser */
  browser = melo_radio_net_browser_new ();
}

static void
melo_radio_net_disable (void)
{
  /* Release radio.net browser */
  g_object_unref (browser);
}

static const char *melo_radio_net_browser_list[] = {
    MELO_RADIO_NET_BROWSER_ID, NULL};

const MeloModule MELO_MODULE_SYM = {
    .id = MELO_RADIO_NET_ID,
    .version = MELO_VERSION (1, 0, 0),
    .api_version = MELO_API_VERSION,

    .name = "Radio.net",
    .description = "Browse and play all radios from Radio.net directory.",

    .browser_list = melo_radio_net_browser_list,

    .enable_cb = melo_radio_net_enable,
    .disable_cb = melo_radio_net_disable,
};
