/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006-2010 William Pitcock, Tony Vroon, George Averill, Giacomo
 *  Lozito, Derek Pomery and Yoshiki Yazawa, and John Lindgren.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <audacious/plugin.h>
#include <libaudcore/audstrings.h>

static void strip_char (gchar * text, gchar c)
{
    gchar * set = text;
    gchar a;

    while ((a = * text ++))
    {
        if (a != c)
            * set ++ = a;
    }

    * set = 0;
}

static gchar * read_win_text (const gchar * path)
{
    void * raw;
    gint64 size;

    vfs_file_get_contents (path, & raw, & size);

    if (raw == NULL)
        return NULL;

    gchar * text = g_convert (raw, size, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
    g_free (raw);
    strip_char (text, '\r');
    return text;
}

static gchar * split_line (gchar * line)
{
    gchar * feed = strchr (line, '\n');

    if (feed == NULL)
        return NULL;

    * feed = 0;
    return feed + 1;
}

static gchar * convert_path (gchar * path, const gchar * base)
{
    if (strstr (path, "://") != NULL)
        return g_strdup (path);

    return aud_construct_uri (path, base);
}

static gboolean playlist_load_m3u (const gchar * path, gint list, gint at)
{
    gchar * text = read_win_text (path);

    if (text == NULL)
        return FALSE;

    gchar * parse = text;
    struct index * add = index_new ();

    while (1)
    {
        gchar * next = split_line (parse);

        while (* parse == ' ' || * parse == '\t')
            parse ++;

        if (! * parse)
            break;

        if (* parse == '#')
            goto NEXT;

        gchar * s = convert_path (parse, path);

        if (s != NULL)
            index_append (add, s);

NEXT:
        parse = next;
    }

    aud_playlist_entry_insert_batch (list, at, add, NULL);
    g_free (text);
    return TRUE;
}

static gboolean playlist_save_m3u (const gchar * filename, gint playlist)
{
    gint entries = aud_playlist_entry_count (playlist);
    gchar *outstr = NULL;
    VFSFile *file;
    gchar *fn = NULL;
    gint count;

    fn = g_filename_to_uri(filename, NULL, NULL);
    file = vfs_fopen(fn ? fn : filename, "wb");
    g_free(fn);

    if (! file)
        return FALSE;

    for (count = 0; count < entries; count ++)
    {
        const gchar * filename = aud_playlist_entry_get_filename (playlist,
         count);
        const gchar * title = aud_playlist_entry_get_title (playlist, count,
         FALSE);
        gint seconds = aud_playlist_entry_get_length (playlist, count, FALSE) /
         1000;

        if (title != NULL)
        {
            outstr = g_locale_from_utf8 (title, -1, NULL, NULL, NULL);

            if(outstr) {
                vfs_fprintf(file, "#EXTINF:%d,%s\n", seconds, outstr);
                g_free(outstr);
                outstr = NULL;
            }
            else
                vfs_fprintf (file, "#EXTINF:%d,%s\n", seconds, title);
        }

        fn = g_filename_from_uri (filename, NULL, NULL);
        vfs_fprintf (file, "%s\n", fn != NULL ? fn : filename);
        g_free(fn);
    }

    vfs_fclose(file);
    return TRUE;
}

static const gchar * const m3u_exts[] = {"m3u", NULL};

static PlaylistPlugin m3u_plugin = {
 .description = "M3U Playlist Format",
 .extensions = m3u_exts,
 .load = playlist_load_m3u,
 .save = playlist_save_m3u
};

static PlaylistPlugin * const m3u_plugins[] = {& m3u_plugin, NULL};

SIMPLE_PLAYLIST_PLUGIN (m3u, m3u_plugins)
