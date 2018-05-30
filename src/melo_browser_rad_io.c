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

#define MELO_BROWSER_RAD_IO_URL "https://api.radio.net/info/v2/search/"
#define MELO_BROWSER_RAD_IO_USER_AGENT "rad.io for Melo (Android API)"

/* rad.io browser info */
static MeloBrowserInfo melo_browser_rad_io_info = {
  .name = "Browse rad.io",
  .description = "Navigate though all radios from rad.io / radio.de / radio.fr",
  .tags_support = TRUE,
  /* Search feature */
  .search_support = TRUE,
  .search_input_text = "Type a radio name...",
  .search_button_text = "Go",
};

static const MeloBrowserInfo *melo_browser_rad_io_get_info (
                                                          MeloBrowser *browser);
static MeloBrowserList *melo_browser_rad_io_get_list (MeloBrowser *browser,
                                        const gchar *path,
                                        const MeloBrowserGetListParams *params);
static MeloBrowserList *melo_browser_rad_io_search (MeloBrowser *browser,
                                         const gchar *input,
                                         const MeloBrowserSearchParams *params);
static MeloTags *melo_browser_rad_io_get_tags (MeloBrowser *browser,
                                               const gchar *path,
                                               MeloTagsFields fields);
static gboolean melo_browser_rad_io_action (MeloBrowser *browser,
                                         const gchar *path,
                                         MeloBrowserItemAction action,
                                         const MeloBrowserActionParams *params);

typedef struct {
  const gchar *id;
  const gchar *name;
} MeloBrowserRadIoCategory;

const static MeloBrowserRadIoCategory melo_browser_rad_io_cats[] = {
  { "cities", "Cities" },
  { "languages", "Languages" },
  { "countries", "Countries" },
  { "topics", "Topics" },
  { "genres", "Genres" },
};

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
  bclass->search = melo_browser_rad_io_search;
  bclass->get_tags = melo_browser_rad_io_get_tags;
  bclass->action = melo_browser_rad_io_action;

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
  priv->session = soup_session_new_with_options (SOUP_SESSION_USER_AGENT,
                                                 MELO_BROWSER_RAD_IO_USER_AGENT,
                                                 NULL);
}

static const MeloBrowserInfo *
melo_browser_rad_io_get_info (MeloBrowser *browser)
{
  return &melo_browser_rad_io_info;
}

static gpointer
melo_browser_rad_io_get_json (MeloBrowserRadIo *brad, const gchar *url,
                              JsonNodeType type)
{
  MeloBrowserRadIoPrivate *priv = brad->priv;
  SoupMessage *msg;
  GInputStream *stream;
  JsonParser *parser;
  JsonNode *node;
  gpointer ret = NULL;

  /* Create request */
  msg = soup_message_new ("GET", url);

  /* Send message and wait answer */
  stream = soup_session_send (priv->session, msg, NULL, NULL);
  if (!stream)
    goto bad_request;

  /* Bad status */
  if (msg->status_code != 200)
    goto bad_status;

  /* Parse JSON */
  parser = json_parser_new ();
  if (!json_parser_load_from_stream (parser, stream, NULL, NULL))
    goto bad_json;

  /* Get root node and check its type */
  node = json_parser_get_root (parser);
  if (!node || json_node_get_node_type (node) != type)
    goto bad_json;

  /* Return node */
  if (type == JSON_NODE_OBJECT) {
    ret = json_node_get_object (node);
    json_object_ref ((JsonObject *) ret);
  } else if (type == JSON_NODE_ARRAY) {
    ret = json_node_get_array (node);
    json_array_ref ((JsonArray *) ret);
  }

bad_json:
  g_object_unref (parser);
bad_status:
  g_object_unref (stream);
bad_request:
  g_object_unref (msg);
  return ret;
}

static inline JsonObject *
melo_browser_rad_io_get_json_object (MeloBrowserRadIo *brad, const gchar *url)
{
  return melo_browser_rad_io_get_json (brad, url, JSON_NODE_OBJECT);
}

static inline JsonArray *
melo_browser_rad_io_get_json_array (MeloBrowserRadIo *brad, const gchar *url)
{
  return melo_browser_rad_io_get_json (brad, url, JSON_NODE_ARRAY);
}

static MeloTags *
melo_browser_rad_io_gen_tags (MeloBrowserRadIo *brad, JsonObject *obj,
                              MeloTagsFields fields)
{
  const gchar *logo = NULL;
  MeloTags *tags;

  /* Generate tags */
  tags = melo_tags_new ();
  if (!tags)
    return NULL;

  /* Get biggest logo first */
  if (json_object_has_member (obj, "logo300x300"))
    logo = json_object_get_string_member (obj, "logo300x300");
  else if (json_object_has_member (obj, "logo175x175"))
    logo = json_object_get_string_member (obj, "logo175x175");
  else if (json_object_has_member (obj, "logo100x100"))
    logo = json_object_get_string_member (obj, "logo100x100");
  else if (json_object_has_member (obj, "logo44x44"))
    logo = json_object_get_string_member (obj, "logo44x44");

  /* Add cover to tags */
  if (logo && fields & MELO_TAGS_FIELDS_COVER)
    melo_tags_set_cover_by_url (tags, logo, MELO_TAGS_COVER_PERSIST_EXIT);

  return tags;
}

static gboolean
melo_browser_rad_io_gen_station_list (MeloBrowserRadIo *brad,
                                      MeloBrowserList *list, JsonObject *obj,
                                      MeloBrowserTagsMode tags_mode,
                                      MeloTagsFields tags_fields)
{
  MeloBrowserItem *item;
  JsonArray *array;
  gint len, i;

  /* Check categories is available */
  if (!json_object_has_member (obj, "categories"))
    return FALSE;

  /* Get categories */
  array = json_object_get_array_member (obj, "categories");
  if (array && json_array_get_length (array) >= 1) {
    JsonObject *o;

    /* Get matches */
    o = json_array_get_object_element (array, 0);
    if (o && json_object_has_member (o, "matches")) {
      array = json_object_get_array_member (o, "matches");
      len = json_array_get_length (array);
      for (i = 0; i < len; i++) {
        const gchar *name;
        gchar id[10];
        JsonObject *n;
        gint iteid;

        /* Get next station */
        o = json_array_get_object_element (array, i);
        if (!o)
          continue;

        /* Get ID and name */
        g_snprintf (id, 10, "%d", json_object_get_int_member (o, "id"));
        n = json_object_get_object_member (o, "name");
        if (n)
          name = json_object_get_string_member (n, "value");

        /* Create browser item */
        item = melo_browser_item_new (id, MELO_BROWSER_ITEM_TYPE_MEDIA);
        if (item) {
          /* Set name with station name */
          item->name = g_strdup (name);
          item->actions = MELO_BROWSER_ITEM_ACTION_FIELDS_PLAY;

          /* Generate tags for item */
          if (tags_mode != MELO_BROWSER_TAGS_MODE_NONE)
            item->tags = melo_browser_rad_io_gen_tags (brad, o, tags_fields);

          /* Add item to list */
          list->items = g_list_prepend (list->items, item);
        }
      }
    }
  }

  return TRUE;
}

static MeloBrowserList *
melo_browser_rad_io_get_list (MeloBrowser *browser, const gchar *path,
                              const MeloBrowserGetListParams *params)
{
  MeloBrowserRadIo *brad = MELO_BROWSER_RAD_IO (browser);
  MeloBrowserList *list;
  MeloBrowserItem *item;
  gchar **parts = NULL;
  gchar *url;
  gint page;
  gint len;
  gint i;

  /* Create browser list */
  list = melo_browser_list_new (path);
  if (!list)
    return NULL;

  /* Parse path */
  if (!g_strcmp0 (path, "/")) {
    /* Add main categories */
    for (i = 0; i < G_N_ELEMENTS (melo_browser_rad_io_cats); i++) {
      item = melo_browser_item_new (melo_browser_rad_io_cats[i].id,
                                    MELO_BROWSER_ITEM_TYPE_CATEGORY);
      if (item) {
        item->name = g_strdup (melo_browser_rad_io_cats[i].name);
        list->items = g_list_prepend (list->items, item);
      }
    }
    return list;
  }

  /* Generate page number from offset / count */
  page = (params->offset / params->count) + 1;

  /* Parse path */
  parts = g_strsplit (path + 1, "/", 3);
  if (!parts)
    return list;

  /* Generate list */
  if (parts[1] && *parts[1] != '\0') {
    JsonObject *obj;

    /* Transform first part */
    len = parts[0] ? strlen (parts[0]) : 0;
    if (len > 0 && parts[0][len - 1] == 's') {
      parts[0][len - 1] = '\0';
      if (len > 2 && parts[0][len - 3] == 'i' && parts[0][len - 2] == 'e') {
        parts[0][len - 3] = 'y';
        parts[0][len - 2] = '\0';
      }
    }

    /* Get stations list */
    url = g_strdup_printf (MELO_BROWSER_RAD_IO_URL "stationsby%s?%s=%s"
                           "&sizeperpage=%d&pageindex=%d&sorttype=STATION_NAME",
                           parts[0], parts[0], parts[1], params->count, page);

    /* Get object from URL */
    obj = melo_browser_rad_io_get_json_object (brad, url);
    g_free (url);

    /* Parse response */
    if (obj) {
      /* Generate station list */
      melo_browser_rad_io_gen_station_list (brad, list, obj, params->tags_mode,
                                            params->tags_fields);

      /* Free object */
      json_object_unref (obj);
    }
  } else {
    JsonArray *array;

    /* Get list by categories */
    url = g_strdup_printf (MELO_BROWSER_RAD_IO_URL "get%s", parts[0]);

    /* Get array from URL */
    array = melo_browser_rad_io_get_json_array (brad, url);
    g_free (url);

    /* Parse response */
    if (array) {
      len = json_array_get_length (array);
      if (params->offset + params->count < len)
        len = params->offset + params->count;
      for (i = params->offset; i < len; i++) {
        const gchar *id, *name;
        JsonObject *o;

        /* Get object */
        o = json_array_get_object_element (array, i);
        if (!o)
          continue;

        /* Get ID and name */
        id = json_object_get_string_member (o, "systemEnglish");
        name = json_object_get_string_member (o, "localized");

        /* Create browser item */
        item = melo_browser_item_new (id, MELO_BROWSER_ITEM_TYPE_CATEGORY);
        if (item) {
          item->name = g_strdup (name);
          list->items = g_list_prepend (list->items, item);
        }
      }

      /* Free array */
      json_array_unref (array);
    }
  }

  /* Reverse list */
  list->items = g_list_reverse (list->items);

  /* Free path parts */
  g_strfreev (parts);

  return list;
}

static MeloBrowserList *
melo_browser_rad_io_search (MeloBrowser *browser, const gchar *input,
                            const MeloBrowserSearchParams *params)
{
  MeloBrowserRadIo *brad = MELO_BROWSER_RAD_IO (browser);
  MeloBrowserList *list;
  JsonObject *obj;
  gchar *url, *query;
  gint page;
  gint len;
  gint i;

  /* Create browser list */
  list = melo_browser_list_new ("");
  if (!list)
    return NULL;

  /* Generate query string */
  query = g_strdup (input);
  len = strlen (query);
  for (i = 0; i < len; i++)
    if (query[i] == ' ')
      query[i] = '+';

  /* Generate page number from offset / count */
  page = (params->offset / params->count) + 1;

  /* Get stations */
  url = g_strdup_printf (MELO_BROWSER_RAD_IO_URL "stationsonly?query=%s"
                         "&sizeperpage=%d&pageindex=%d&sorttype=STATION_NAME",
                         query, params->count, page);
  obj = melo_browser_rad_io_get_json_object (brad, url);
  g_free (query);
  g_free (url);

  /* Parse response */
  if (obj) {
    /* Generate station list */
    melo_browser_rad_io_gen_station_list (brad, list, obj, params->tags_mode,
                                          params->tags_fields);

    /* Free object */
    json_object_unref (obj);
  }

  /* Reverse list */
  list->items = g_list_reverse (list->items);

  return list;
}

static MeloTags *
melo_browser_rad_io_get_tags (MeloBrowser *browser, const gchar *path,
                              MeloTagsFields fields)
{
  MeloBrowserRadIo *brad = MELO_BROWSER_RAD_IO (browser);
  MeloTags *tags = NULL;
  JsonObject *obj;
  gchar *id, *url;

  /* Get radio id from path */
  id = g_path_get_basename (path);
  if (!id)
    return FALSE;

  /* Get station details */
  url = g_strdup_printf (MELO_BROWSER_RAD_IO_URL "station?station=%s", id);
  obj = melo_browser_rad_io_get_json_object (brad, url);
  g_free (url);
  g_free (id);

  /* No response */
  /* Generate tags from response */
  if (obj) {
    tags = melo_browser_rad_io_gen_tags (brad, obj, MELO_TAGS_FIELDS_FULL);
    json_object_unref (obj);
  }

  return tags;
}

static gboolean
melo_browser_rad_io_action (MeloBrowser *browser, const gchar *path,
                            MeloBrowserItemAction action,
                            const MeloBrowserActionParams *params)
{
  MeloBrowserRadIo *brad = MELO_BROWSER_RAD_IO (browser);
  const gchar *stream_url = NULL;
  const gchar *name = NULL;
  MeloTags *tags;
  JsonObject *obj;
  JsonArray *urls;
  gchar *id, *url;
  gint count, i;

  /* Only support play */
  if (action != MELO_BROWSER_ITEM_ACTION_PLAY)
    return FALSE;

  /* Get radio id from path */
  id = g_path_get_basename (path);
  if (!id)
    return FALSE;

  /* Get station details */
  url = g_strdup_printf (MELO_BROWSER_RAD_IO_URL "station?station=%s", id);
  obj = melo_browser_rad_io_get_json_object (brad, url);
  g_free (url);
  g_free (id);

  /* No response */
  if (!obj)
    return FALSE;

  /* Parse object */
  urls = json_object_get_array_member (obj, "streamUrls");
  count = json_array_get_length (urls);
  for (i = 0; i < count; i++) {
    JsonObject *o;

    /* Get next object */
    o = json_array_get_object_element (urls, i);
    if (!o || !json_object_has_member (o, "streamUrl"))
      continue;

    /* Get URL */
    stream_url = json_object_get_string_member (o, "streamUrl");
    break;
  }

  /* Get radio name */
  name = json_object_get_string_member (obj, "name");

  /* Generate tags */
  tags = melo_browser_rad_io_gen_tags (brad, obj, MELO_TAGS_FIELDS_FULL);

  /* Play radio station */
  melo_player_play (browser->player, stream_url, name, tags, FALSE);
  if (tags)
    melo_tags_unref (tags);

  /* Free object */
  json_object_unref (obj);

  return TRUE;
}
