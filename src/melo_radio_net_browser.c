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

#include <melo/melo_http_client.h>
#include <melo/melo_playlist.h>

#define MELO_LOG_TAG "radio_net_browser"
#include <melo/melo_log.h>

#include <melo/proto/browser.pb-c.h>

#include "melo_radio_net_browser.h"

#define RADIO_PLAYER_ID "com.sparod.radio.player"

#define MELO_RADIO_NET_BROWSER_URL "https://api.radio.net/info/v2/search/"
#define MELO_RADIO_NET_BROWSER_USER_AGENT "rad.io for Melo (Android API)"
#define MELO_RADIO_NET_BROWSER_ASSET_URL \
  "https://static.radio.net/images/broadcasts/"

typedef struct {
  unsigned int offset;
  unsigned int count;
} MeloRadioNetBrowserAsync;

struct _MeloRadioNetBrowser {
  GObject parent_instance;

  MeloHttpClient *client;
};

MELO_DEFINE_BROWSER (MeloRadioNetBrowser, melo_radio_net_browser)

static bool melo_radio_net_browser_handle_request (
    MeloBrowser *browser, const MeloMessage *msg, MeloRequest *req);
static char *melo_radio_net_browser_get_asset (
    MeloBrowser *browser, const char *id);

static void
melo_radio_net_browser_finalize (GObject *object)
{
  MeloRadioNetBrowser *browser = MELO_RADIO_NET_BROWSER (object);

  /* Release HTTP client */
  g_object_unref (browser->client);

  /* Chain finalize */
  G_OBJECT_CLASS (melo_radio_net_browser_parent_class)->finalize (object);
}

static void
melo_radio_net_browser_class_init (MeloRadioNetBrowserClass *klass)
{
  MeloBrowserClass *parent_class = MELO_BROWSER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* Setup callbacks */
  parent_class->handle_request = melo_radio_net_browser_handle_request;
  parent_class->get_asset = melo_radio_net_browser_get_asset;

  /* Set finalize */
  object_class->finalize = melo_radio_net_browser_finalize;
}

static void
melo_radio_net_browser_init (MeloRadioNetBrowser *self)
{
  /* Create new HTTP client */
  self->client = melo_http_client_new (MELO_RADIO_NET_BROWSER_USER_AGENT);
}

MeloRadioNetBrowser *
melo_radio_net_browser_new ()
{
  return g_object_new (MELO_TYPE_RADIO_NET_BROWSER, "id",
      MELO_RADIO_NET_BROWSER_ID, "name", "Radio.net", "description",
      "Browse in Radio.net directory", "icon", "fa:broadcast-tower",
      "support-search", true, NULL);
}

static const char *
melo_radio_net_browser_get_cover (JsonObject *obj)

{
  const char *logo;

  /* Get biggest logo first */
  if (json_object_has_member (obj, "logo300x300"))
    logo = json_object_get_string_member (obj, "logo300x300");
  else if (json_object_has_member (obj, "logo175x175"))
    logo = json_object_get_string_member (obj, "logo175x175");
  else if (json_object_has_member (obj, "logo100x100"))
    logo = json_object_get_string_member (obj, "logo100x100");
  else if (json_object_has_member (obj, "logo44x44"))
    logo = json_object_get_string_member (obj, "logo44x44");
  else
    return NULL;

  /* Remove prefix */
  if (!g_str_has_prefix (logo, MELO_RADIO_NET_BROWSER_ASSET_URL))
    return NULL;

  return logo + sizeof (MELO_RADIO_NET_BROWSER_ASSET_URL) - 1;
}

static MeloMessage *
array_cb (JsonArray *array, MeloRequest *req)
{
  MeloRadioNetBrowserAsync *async = melo_request_get_user_data (req);
  Browser__Response resp = BROWSER__RESPONSE__INIT;
  Browser__Response__MediaList media_list = BROWSER__RESPONSE__MEDIA_LIST__INIT;
  Browser__Response__MediaItem **items_ptr;
  Browser__Response__MediaItem *items;
  MeloMessage *msg;
  unsigned int i, count, len;

  /* Set response type */
  resp.resp_case = BROWSER__RESPONSE__RESP_MEDIA_LIST;
  resp.media_list = &media_list;

  /* Get array length */
  len = json_array_get_length (array);

  /* Invalid offset */
  if (async->offset >= len) {
    free (async);
    return NULL;
  }

  /* Calculate item list length */
  count = len - async->offset;
  if (count > async->count)
    count = async->count;

  /* Set list count and offset */
  media_list.count = count;
  media_list.offset = async->offset;

  /* Allocate item list */
  items_ptr = malloc (sizeof (*items_ptr) * count);
  items = malloc (sizeof (*items) * count);

  /* Set item list */
  media_list.n_items = count;
  media_list.items = items_ptr;

  /* Add media items */
  for (i = 0; i < count; i++) {
    JsonObject *obj;

    /* Init media item */
    browser__response__media_item__init (&items[i]);
    media_list.items[i] = &items[i];

    /* Get next entry */
    obj = json_array_get_object_element (array, i + async->offset);
    if (!obj)
      continue;

    /* Set media */
    items[i].id = (char *) json_object_get_string_member (obj, "systemEnglish");
    items[i].name = (char *) json_object_get_string_member (obj, "localized");
    items[i].type = BROWSER__RESPONSE__MEDIA_ITEM__TYPE__FOLDER;
  }

  /* Pack message */
  msg = melo_message_new (browser__response__get_packed_size (&resp));
  melo_message_set_size (
      msg, browser__response__pack (&resp, melo_message_get_data (msg)));

  /* Free item list */
  free (items_ptr);
  free (items);

  /* Free async object */
  free (async);

  return msg;
}

static MeloMessage *
object_cb (JsonObject *obj, MeloRequest *req)
{
  static Browser__Action actions[2] = {
      {
          .base = PROTOBUF_C_MESSAGE_INIT (&browser__action__descriptor),
          .type = BROWSER__ACTION__TYPE__PLAY,
          .name = "Play radio",
          .icon = "fa:play",
      },
      {
          .base = PROTOBUF_C_MESSAGE_INIT (&browser__action__descriptor),
          .type = BROWSER__ACTION__TYPE__ADD,
          .name = "Add radio to playlist",
          .icon = "fa:plus",
      },
  };
  static Browser__Action *actions_ptr[2] = {
      &actions[0],
      &actions[1],
  };
  MeloRadioNetBrowserAsync *async = melo_request_get_user_data (req);
  Browser__Response resp = BROWSER__RESPONSE__INIT;
  Browser__Response__MediaList media_list = BROWSER__RESPONSE__MEDIA_LIST__INIT;
  Browser__Response__MediaItem **items_ptr;
  Browser__Response__MediaItem *items;
  Tags__Tags *tags;
  MeloMessage *msg;
  JsonArray *array;
  unsigned int i, len, skip;

  /* Check categories is available */
  if (!json_object_has_member (obj, "categories")) {
    free (async);
    return NULL;
  }

  /* Get categories */
  array = json_object_get_array_member (obj, "categories");
  if (!array || json_array_get_length (array) < 1) {
    free (async);
    return NULL;
  }

  /* Get matches */
  obj = json_array_get_object_element (array, 0);
  if (!obj || !json_object_has_member (obj, "matches")) {
    free (async);
    return NULL;
  }

  /* Set response type */
  resp.resp_case = BROWSER__RESPONSE__RESP_MEDIA_LIST;
  resp.media_list = &media_list;

  /* Get matches array */
  array = json_object_get_array_member (obj, "matches");
  len = json_array_get_length (array);

  /* Calculate skip */
  skip = async->offset % async->count;
  if (skip >= len) {
    free (async);
    return NULL;
  }

  /* Calculate final item count */
  len -= skip;
  if (len > async->count)
    len = async->count;

  /* Set list count and offset */
  media_list.count = len;
  media_list.offset = async->offset;

  /* Allocate item list */
  items_ptr = malloc (sizeof (*items_ptr) * len);
  items = malloc (sizeof (*items) * len);
  tags = malloc (sizeof (*tags) * len);

  /* Set item list */
  media_list.n_items = len;
  media_list.items = items_ptr;

  /* Add media items */
  for (i = 0; i < len; i++) {
    const char *cover;
    JsonObject *o;

    /* Init media item */
    browser__response__media_item__init (&items[i]);
    media_list.items[i] = &items[i];

    /* Get next station */
    obj = json_array_get_object_element (array, i + skip);
    if (!obj)
      continue;

    /* Set station ID */
    items[i].id = g_strdup_printf (
        "%lld", (long long) json_object_get_int_member (obj, "id"));

    /* Set station name */
    o = json_object_get_object_member (obj, "name");
    if (o)
      items[i].name = (char *) json_object_get_string_member (o, "value");

    /* Set media type */
    items[i].type = BROWSER__RESPONSE__MEDIA_ITEM__TYPE__MEDIA;

    /* Set media actions */
    items[i].n_actions = G_N_ELEMENTS (actions_ptr);
    items[i].actions = actions_ptr;

    /* Set tags */
    tags__tags__init (&tags[i]);
    items[i].tags = &tags[i];

    /* Set cover */
    cover = melo_radio_net_browser_get_cover (obj);
    if (cover)
      tags[i].cover =
          melo_tags_gen_cover (melo_request_get_object (req), cover);
  }

  /* Pack message */
  msg = melo_message_new (browser__response__get_packed_size (&resp));
  melo_message_set_size (
      msg, browser__response__pack (&resp, melo_message_get_data (msg)));

  /* Free item list */
  for (i = 0; i < len; i++) {
    if (tags[i].cover != protobuf_c_empty_string)
      free (tags[i].cover);
    free (items[i].id);
  }
  free (items_ptr);
  free (items);
  free (tags);

  /* Free asynchronous object */
  free (async);

  return msg;
}

static void
list_cb (MeloHttpClient *client, JsonNode *node, void *user_data)
{
  MeloRequest *req = user_data;

  /* Make media list response from JSON node */
  if (node) {
    MeloMessage *msg;

    /* Parse node as array or object */
    if (json_node_get_node_type (node) == JSON_NODE_ARRAY)
      msg = array_cb (json_node_get_array (node), req);
    else if (json_node_get_node_type (node) == JSON_NODE_OBJECT)
      msg = object_cb (json_node_get_object (node), req);

    /* Send media list response */
    if (msg)
      melo_request_send_response (req, msg);
  }

  /* Release request */
  melo_request_unref (req);
}

static bool
melo_radio_net_browser_get_root (MeloRequest *req)
{
  static const struct {
    const char *id;
    const char *name;
    const char *icon;
  } root[] = {
      {"genres", "Genres", "fa:guitar"},
      {"topics", "Topics", "fa:comment"},
      {"countries", "Countries", "fa:flag"},
      {"languages", "Languages", "fa:globe-europe"},
      {"cities", "Cities", "fa:city"},
  };
  Browser__Response resp = BROWSER__RESPONSE__INIT;
  Browser__Response__MediaList media_list = BROWSER__RESPONSE__MEDIA_LIST__INIT;
  Browser__Response__MediaItem *items_ptr[G_N_ELEMENTS (root)];
  Browser__Response__MediaItem items[G_N_ELEMENTS (root)];
  Tags__Tags tags[G_N_ELEMENTS (root)];
  MeloMessage *msg;
  unsigned int i;

  /* Set response type */
  resp.resp_case = BROWSER__RESPONSE__RESP_MEDIA_LIST;
  resp.media_list = &media_list;

  /* Set item list */
  media_list.n_items = G_N_ELEMENTS (root);
  media_list.items = items_ptr;

  /* Set list count and offset */
  media_list.count = G_N_ELEMENTS (root);
  media_list.offset = 0;

  /* Add media items */
  for (i = 0; i < G_N_ELEMENTS (root); i++) {
    /* Init media item */
    browser__response__media_item__init (&items[i]);
    media_list.items[i] = &items[i];

    /* Set media */
    items[i].id = (char *) root[i].id;
    items[i].name = (char *) root[i].name;
    items[i].type = BROWSER__RESPONSE__MEDIA_ITEM__TYPE__FOLDER;

    /* Set tags */
    tags__tags__init (&tags[i]);
    items[i].tags = &tags[i];
    tags[i].cover = (char *) root[i].icon;
  }

  /* Pack message */
  msg = melo_message_new (browser__response__get_packed_size (&resp));
  melo_message_set_size (
      msg, browser__response__pack (&resp, melo_message_get_data (msg)));

  /* Send media list response */
  melo_request_send_response (req, msg);

  /* Release request */
  melo_request_unref (req);

  return true;
}

static bool
melo_radio_net_browser_get_media_list (MeloRadioNetBrowser *browser,
    Browser__Request__GetMediaList *r, MeloRequest *req)
{
  MeloRadioNetBrowserAsync *async;
  const char *query = r->query;
  char *url;
  unsigned int page, count;
  bool search = false;
  bool ret;

  /* Root media list */
  if (!g_strcmp0 (query, "/"))
    return melo_radio_net_browser_get_root (req);

  /* Allocate async object */
  async = malloc (sizeof (*async));
  if (!async)
    return false;

  /* Set async object */
  async->offset = r->offset;
  async->count = r->count;
  melo_request_set_user_data (req, async);

  /* Perform search */
  if (g_str_has_prefix (r->query, "search:")) {
    search = true;
    query += 7;
  } else
    query++;

  /* Calculate page index */
  page = r->offset / r->count;
  count = r->count;
  if (page * r->count != r->offset)
    count *= 2;
  page++;

  /* Generate URL */
  if (!search) {
    char *q;

    /* Split request */
    q = strchr (query, '/');

    /* Generate URL */
    if (!q || *q++ == '\0') {
      /* Create category URL */
      url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL "get%s", query);
    } else {
      const char *cat;

      /* Get category */
      if (g_str_has_prefix (query, "genres/"))
        cat = "genre";
      else if (g_str_has_prefix (query, "topics/"))
        cat = "topic";
      else if (g_str_has_prefix (query, "countries/"))
        cat = "country";
      else if (g_str_has_prefix (query, "languages/"))
        cat = "language";
      else if (g_str_has_prefix (query, "cities/"))
        cat = "city";
      else
        return false;

      /* Create sub-category URL */
      url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL
          "stationsby%s?%s=%s"
          "&sizeperpage=%d&pageindex=%d&sorttype=STATION_NAME",
          cat, cat, q, count, page);
    }
  } else {
    char *q, *p;

    /* Generate query string */
    p = q = g_strdup (query);
    while (*p != '\0') {
      if (*p == ' ')
        *p = '+';
      p++;
    }

    /* Create search URL */
    url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL
        "stationsonly?query=%s"
        "&sizeperpage=%d&pageindex=%d&sorttype=STATION_NAME",
        q, count, page);
    g_free (q);
  }

  MELO_LOGD ("get_media_list: %s", url);

  /* Get list from URL */
  ret = melo_http_client_get_json (browser->client, url, list_cb, req);
  g_free (url);

  return ret;
}

static void
action_cb (MeloHttpClient *client, JsonNode *node, void *user_data)
{
  MeloRequest *req = user_data;

  /* Extract radio URL from JSON node */
  if (node) {
    Browser__Action__Type type;
    JsonObject *obj;

    /* Get object */
    obj = json_node_get_object (node);
    if (obj) {
      const char *name, *url = NULL, *cover;
      unsigned int i, count;
      MeloTags *tags = NULL;
      JsonArray *urls;

      /* Get station name */
      name = json_object_get_string_member (obj, "name");

      /* Get station URL */
      urls = json_object_get_array_member (obj, "streamUrls");
      count = json_array_get_length (urls);
      for (i = 0; i < count; i++) {
        JsonObject *o;

        /* Get next object */
        o = json_array_get_object_element (urls, i);
        if (!o || !json_object_has_member (o, "streamUrl"))
          continue;

        /* Get URL */
        url = json_object_get_string_member (o, "streamUrl");
        break;
      }

      /* Get tags from object */
      cover = melo_radio_net_browser_get_cover (obj);
      if (cover) {
        tags = melo_tags_new ();
        if (tags)
          melo_tags_set_cover (tags, melo_request_get_object (req), cover);
      }

      MELO_LOGD ("play radio %s: %s", name, url);

      /* Get action type */
      type = (uintptr_t) melo_request_get_user_data (req);

      /* Do action */
      if (type == BROWSER__ACTION__TYPE__PLAY)
        melo_playlist_play_media (RADIO_PLAYER_ID, url, name, tags);
      else if (type == BROWSER__ACTION__TYPE__ADD)
        melo_playlist_add_media (RADIO_PLAYER_ID, url, name, tags);
    }
  }

  /* Release request */
  melo_request_unref (req);
}

static bool
melo_radio_net_browser_do_action (MeloRadioNetBrowser *browser,
    Browser__Request__DoAction *r, MeloRequest *req)
{
  const char *path = r->path;
  const char *id;
  char *url;
  bool ret;

  /* Check action type */
  if (r->type != BROWSER__ACTION__TYPE__PLAY &&
      r->type != BROWSER__ACTION__TYPE__ADD)
    return false;

  /* Action on search item */
  if (g_str_has_prefix (path, "search:"))
    path += 7;

  /* Get station ID */
  id = strrchr (path, '/');
  if (id)
    id++;
  else
    id = path;

  /* Save action type in request */
  melo_request_set_user_data (req, (void *) r->type);

  /* Generate URL from path */
  url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL "station?station=%s", id);

  /* Get radio URL from sparod */
  ret = melo_http_client_get_json (browser->client, url, action_cb, req);
  g_free (url);

  return ret;
}

static bool
melo_radio_net_browser_handle_request (
    MeloBrowser *browser, const MeloMessage *msg, MeloRequest *req)
{
  MeloRadioNetBrowser *rbrowser = MELO_RADIO_NET_BROWSER (browser);
  Browser__Request *r;
  bool ret = false;

  /* Unpack request */
  r = browser__request__unpack (
      NULL, melo_message_get_size (msg), melo_message_get_cdata (msg, NULL));
  if (!r) {
    MELO_LOGE ("failed to unpack request");
    return false;
  }

  /* Handle request */
  switch (r->req_case) {
  case BROWSER__REQUEST__REQ_GET_MEDIA_LIST:
    ret = melo_radio_net_browser_get_media_list (
        rbrowser, r->get_media_list, req);
    break;
  case BROWSER__REQUEST__REQ_DO_ACTION:
    ret = melo_radio_net_browser_do_action (rbrowser, r->do_action, req);
    break;
  default:
    MELO_LOGE ("request %u not supported", r->req_case);
  }

  /* Free request */
  browser__request__free_unpacked (r, NULL);

  return ret;
}

static char *
melo_radio_net_browser_get_asset (MeloBrowser *browser, const char *id)
{
  return g_strconcat (MELO_RADIO_NET_BROWSER_ASSET_URL, id, NULL);
}
