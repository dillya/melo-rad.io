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

#ifndef _MELO_RADIO_NET_BROWSER_H_
#define _MELO_RADIO_NET_BROWSER_H_

#include <melo/melo_browser.h>

G_BEGIN_DECLS

#define MELO_RADIO_NET_BROWSER_ID "net.radio.browser"

#define MELO_TYPE_RADIO_NET_BROWSER melo_radio_net_browser_get_type ()
MELO_DECLARE_BROWSER (
    MeloRadioNetBrowser, melo_radio_net_browser, RADIO_NET_BROWSER)

/**
 * Create a new radio.net browser.
 *
 * @return the newly radio.net browser or NULL.
 */
MeloRadioNetBrowser *melo_radio_net_browser_new (void);

G_END_DECLS

#endif /* !_MELO_RADIO_NET_BROWSER_H_ */
