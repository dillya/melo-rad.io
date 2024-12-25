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

#include <stdio.h>

#include <melo/melo_http_client.h>
#include <melo/melo_library.h>
#include <melo/melo_playlist.h>

#define MELO_LOG_TAG "radio_net_browser"
#include <melo/melo_log.h>

#include <melo/proto/browser.pb-c.h>

#include "melo_radio_net_browser.h"

#define RADIO_PLAYER_ID "com.sparod.radio.player"

#define MELO_RADIO_NET_BROWSER_URL "https://prod.radio-api.net/"
#define MELO_RADIO_NET_BROWSER_USER_AGENT "rad.io for Melo (Android API)"
#define MELO_RADIO_NET_BROWSER_ASSET_URL \
  "https://station-images.prod.radio-api.net/"

typedef struct {
  MeloRadioNetBrowser *browser;
  char *tag;
  unsigned int offset;
  unsigned int count;
} MeloRadioNetBrowserAsync;

struct _MeloRadioNetBrowser {
  GObject parent_instance;

  MeloHttpClient *client;
  JsonNode *tags;
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
      "Browse in Radio.net directory", "icon", MELO_RADIO_NET_BROWSER_ICON,
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
category_cb (JsonObject *obj, MeloRequest *req)
{
  MeloRadioNetBrowserAsync *async = melo_request_get_user_data (req);
  Browser__Response resp = BROWSER__RESPONSE__INIT;
  Browser__Response__MediaList media_list = BROWSER__RESPONSE__MEDIA_LIST__INIT;
  Browser__Response__MediaItem **items_ptr;
  Browser__Response__MediaItem *items;
  MeloMessage *msg;
  JsonArray *array;
  unsigned int i, count, len;

  /* Check categories is available */
  if (!json_object_has_member (obj, async->tag)) {
    return NULL;
  }

  /* Get categories */
  array = json_object_get_array_member (obj, async->tag);
  if (!array || json_array_get_length (array) < 1) {
    return NULL;
  }

  /* Set response type */
  resp.resp_case = BROWSER__RESPONSE__RESP_MEDIA_LIST;
  resp.media_list = &media_list;

  /* Get array length */
  len = json_array_get_length (array);

  /* Invalid offset */
  if (async->offset >= len) {
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
    items[i].id = (char *) json_object_get_string_member (obj, "systemName");
    items[i].name = (char *) json_object_get_string_member (obj, "name");
    items[i].type = BROWSER__RESPONSE__MEDIA_ITEM__TYPE__FOLDER;
  }

  /* Pack message */
  msg = melo_message_new (browser__response__get_packed_size (&resp));
  melo_message_set_size (
      msg, browser__response__pack (&resp, melo_message_get_data (msg)));

  /* Free item list */
  free (items_ptr);
  free (items);

  return msg;
}

static MeloMessage *
station_cb (JsonObject *obj, MeloRequest *req)
{
  static Browser__Action actions[] = {
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
      {
          .base = PROTOBUF_C_MESSAGE_INIT (&browser__action__descriptor),
          .type = BROWSER__ACTION__TYPE__SET_FAVORITE,
          .name = "Add radio to favorites",
          .icon = "fa:star",
      },
      {
          .base = PROTOBUF_C_MESSAGE_INIT (&browser__action__descriptor),
          .type = BROWSER__ACTION__TYPE__UNSET_FAVORITE,
          .name = "Remove radio from favorites",
          .icon = "fa:star",
      },
  };
  static Browser__Action *actions_ptr[] = {
      &actions[0],
      &actions[1],
      &actions[2],
      &actions[3],
  };
  static uint32_t set_fav_actions[] = {0, 1, 2};
  static uint32_t unset_fav_actions[] = {0, 1, 3};
  MeloRadioNetBrowserAsync *async = melo_request_get_user_data (req);
  Browser__Response resp = BROWSER__RESPONSE__INIT;
  Browser__Response__MediaList media_list = BROWSER__RESPONSE__MEDIA_LIST__INIT;
  Browser__Response__MediaItem **items_ptr;
  Browser__Response__MediaItem *items;
  Tags__Tags *tags;
  MeloMessage *msg;
  JsonArray *array;
  unsigned int i, len;

  /* Check categories is available */
  if (!json_object_has_member (obj, "playables")) {
    return NULL;
  }

  /* Get categories */
  array = json_object_get_array_member (obj, "playables");
  if (!array || json_array_get_length (array) < 1) {
    return NULL;
  }

  /* Set response type */
  resp.resp_case = BROWSER__RESPONSE__RESP_MEDIA_LIST;
  resp.media_list = &media_list;

  /* Get array length */
  len = json_array_get_length (array);

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

  /* Set actions */
  media_list.n_actions = G_N_ELEMENTS (actions_ptr);
  media_list.actions = actions_ptr;

  /* Add media items */
  for (i = 0; i < len; i++) {
    const char *cover;
    uint64_t id;

    /* Init media item */
    browser__response__media_item__init (&items[i]);
    tags__tags__init (&tags[i]);
    media_list.items[i] = &items[i];

    /* Get next station */
    obj = json_array_get_object_element (array, i);
    if (!obj)
      continue;

    /* Set station ID */
    items[i].id = (char *) json_object_get_string_member (obj, "id");

    /* Set station name */
    items[i].name = (char *) json_object_get_string_member (obj, "name");

    /* Set media type */
    items[i].type = BROWSER__RESPONSE__MEDIA_ITEM__TYPE__MEDIA;

    /* Set favorite and action IDs */
    id = melo_library_get_media_id_from_browser (
        MELO_RADIO_NET_BROWSER_ID, items[i].id);
    items[i].favorite =
        melo_library_media_get_flags (id) & MELO_LIBRARY_FLAG_FAVORITE;
    if (items[i].favorite) {
      items[i].n_action_ids = G_N_ELEMENTS (unset_fav_actions);
      items[i].action_ids = unset_fav_actions;
    } else {
      items[i].n_action_ids = G_N_ELEMENTS (set_fav_actions);
      items[i].action_ids = set_fav_actions;
    }

    /* Set tags */
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
  }
  free (items_ptr);
  free (items);
  free (tags);

  return msg;
}

static void
list_cb (MeloHttpClient *client, JsonNode *node, void *user_data)
{
  MeloRequest *req = user_data;

  /* Make media list response from JSON node */
  if (node) {
    MeloRadioNetBrowserAsync *async = melo_request_get_user_data (req);
    MeloMessage *msg = NULL;

    /* Save tag list */
    if (async->tag && !async->browser->tags) {
      async->browser->tags = json_node_ref (node);
    }

    /* Parse node as array or object */
    if (async->tag)
      msg = category_cb (json_node_get_object (node), req);
    else
      msg = station_cb (json_node_get_object (node), req);

    /* Free async object */
    g_free (async->tag);
    free (async);

    /* Send media list response */
    if (msg)
      melo_request_send_response (req, msg);
  }

  /* Release request */
  melo_request_complete (req);
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
  melo_request_complete (req);

  return true;
}

static bool
melo_radio_net_browser_get_media_list (MeloRadioNetBrowser *browser,
    Browser__Request__GetMediaList *r, MeloRequest *req)
{
  MeloRadioNetBrowserAsync *async;
  const char *query = r->query;
  char *url;
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
  async->browser = browser;
  async->tag = NULL;
  async->offset = r->offset;
  async->count = r->count;
  melo_request_set_user_data (req, async);

  /* Perform search */
  if (g_str_has_prefix (r->query, "search:")) {
    search = true;
    query += 7;
  } else
    query++;

  /* Generate URL */
  if (!search) {
    char *q;

    /* Split request */
    q = strchr (query, '/');

    /* Generate URL */
    if (!q || *q == '\0') {
      /* Save current category */
      async->tag = g_strdup (query);

      /* Use cache */
      if (browser->tags) {
        list_cb (browser->client, browser->tags, req);
        return true;
      }

      /* Create category URL */
      url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL "stations/tags");
    } else {
      /* Create sub-category URL */
      *q++ = '\0';
      url = g_strdup_printf (MELO_RADIO_NET_BROWSER_URL
          "stations/by-tag?systemName=%s&tagType=%s&count=%d&offset=%d",
          q, query, r->count, r->offset);
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
        "stations/search?query=%s&count=%d&offset=%d",
        q, r->count, r->offset);
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
    JsonObject *obj = NULL;
    JsonArray *array;

    /* Get array */
    array = json_node_get_array (node);
    if (array) {
      /* Get first item */
      obj = json_array_get_object_element (array, 0);
    }

    /* Get object */
    if (obj) {
      const char *name, *url = NULL, *cover;
      unsigned int i, count;
      MeloTags *tags = NULL;
      JsonArray *urls;

      /* Get station name */
      name = json_object_get_string_member (obj, "name");

      /* Get station URL */
      urls = json_object_get_array_member (obj, "streams");
      count = json_array_get_length (urls);
      for (i = 0; i < count; i++) {
        JsonObject *o;

        /* Get next object */
        o = json_array_get_object_element (urls, i);
        if (!o || !json_object_has_member (o, "url"))
          continue;

        /* Get URL */
        url = json_object_get_string_member (o, "url");
        break;
      }

      /* Get tags from object */
      cover = melo_radio_net_browser_get_cover (obj);
      if (cover) {
        tags = melo_tags_new ();
        if (tags) {
          melo_tags_set_cover (tags, melo_request_get_object (req), cover);
          melo_tags_set_browser (tags, MELO_RADIO_NET_BROWSER_ID);
          melo_tags_set_media_id (
              tags, (char *) json_object_get_string_member (obj, "id"));
        }
      }

      MELO_LOGD ("play radio %s: %s", name, url);

      /* Get action type */
      type = (uintptr_t) melo_request_get_user_data (req);

      /* Do action */
      if (type == BROWSER__ACTION__TYPE__PLAY)
        melo_playlist_play_media (RADIO_PLAYER_ID, url, name, tags);
      else if (type == BROWSER__ACTION__TYPE__ADD)
        melo_playlist_add_media (RADIO_PLAYER_ID, url, name, tags);
      else {
        char *path, *media;

        /* Separate path */
        path = g_strdup (url);
        media = strrchr (path, '/');
        if (media)
          *media++ = '\0';

        /* Set / unset favorite marker */
        if (type == BROWSER__ACTION__TYPE__UNSET_FAVORITE) {
          uint64_t id;

          /* Get media ID */
          id = melo_library_get_media_id (RADIO_PLAYER_ID, 0, path, 0, media);

          /* Unset favorite */
          melo_library_update_media_flags (
              id, MELO_LIBRARY_FLAG_FAVORITE_ONLY, true);
        } else if (type == BROWSER__ACTION__TYPE__SET_FAVORITE)
          /* Set favorite */
          melo_library_add_media (RADIO_PLAYER_ID, 0, path, 0, media, 0,
              MELO_LIBRARY_SELECT (COVER), name, tags, 0,
              MELO_LIBRARY_FLAG_FAVORITE_ONLY);

        /* Free resources */
        g_free (path);
        melo_tags_unref (tags);
      }
    }
  }

  /* Release request */
  melo_request_complete (req);
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
      r->type != BROWSER__ACTION__TYPE__ADD &&
      r->type != BROWSER__ACTION__TYPE__SET_FAVORITE &&
      r->type != BROWSER__ACTION__TYPE__UNSET_FAVORITE)
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
  url = g_strdup_printf (
      MELO_RADIO_NET_BROWSER_URL "stations/details?stationIds=%s", id);

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
