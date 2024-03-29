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

#define MELO_RADIO_NET_BROWSER_ICON \
  "svg:<svg viewBox=\"0 0 172 172\"><g transform=\"translate(-40 -33)\"><g " \
  "transform=\"translate(40.623 33.379)\"><path d=\"m48.727 63.736c-3.78 " \
  "1.715-7.97 2.679-12.383 2.679-16.579 0-30.069-13.489-30.069-30.07 0-16.58 " \
  "13.49-30.069 30.069-30.069 16.58 0 30.069 13.489 30.069 30.069 0 " \
  "4.413-0.963 8.603-2.678 12.381l4.694 4.696c2.716-5.092 4.26-10.903 " \
  "4.26-17.077 0-20.073-16.272-36.345-36.345-36.345-20.072 0-36.344 " \
  "16.272-36.344 36.345s16.272 36.346 36.344 36.346c6.174 0 11.986-1.545 " \
  "17.078-4.261l-4.695-4.694\"/><path d=\"m137.23 66.032h-42.038c-7.215 " \
  "0-13.725 2.179-18.774 5.39l-27.885-27.89c1.245-2.106 1.959-4.562 " \
  "1.959-7.186 0-7.815-6.333-14.15-14.147-14.15-7.815 0-14.151 6.335-14.151 " \
  "14.15 0 7.814 6.336 14.147 14.151 14.147 2.622 0 5.081-0.714 " \
  "7.183-1.958l43.034 43.033c1.898-2.729 5.061-4.516 8.637-4.516h42.04c5.808 " \
  "0 10.511 4.702 10.511 10.509v42.041c0 5.808-4.707 10.513-10.511 " \
  "10.513h-42.04c-5.804 " \
  "0-10.511-4.707-10.511-10.513v-39.902l-18.304-18.304c-1.82 2.826-4.747 " \
  "8.474-4.747 16.165v42.041c0 17.958 15.025 31.531 33.555 " \
  "31.531h42.038c18.53 0 33.552-13.573 " \
  "33.552-31.531v-42.041c0-17.955-15.022-31.529-33.552-31.529\"/></g></g></" \
  "svg>"

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
