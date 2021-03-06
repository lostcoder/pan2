/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Pan - A Newsreader for Gtk+
 * Copyright (C) 2002-2006  Charles Kerr <charles@rebelbase.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pan/general/string-view.h>
#include <pan/general/quark.h>
#include "prefs.h"

#ifndef _GroupPrefs_h_
#define _GroupPrefs_h_

namespace pan
{
  class GroupPrefs
  {
    public:

      bool get_flag          (const Quark       & group,
                              const StringView  & key,
                              bool                fallback) const;

      void set_flag          (const Quark       & group,
                              const StringView  & key,
                              bool                fallback);

      int get_int            (const Quark       & group,
                              const StringView  & key,
                              int                 fallback) const;

      void set_int           (const Quark       & group,
                              const StringView  & key,
                              int                 fallback);

      std::string get_string (const Quark       & group,
                              const StringView  & key,
                              const StringView  & fallback) const;

      void set_string        (const Quark       & group,
                              const StringView  & key,
                              const StringView  & fallback);

    protected:
      static void start_element (GMarkupParseContext *context,
                                 const gchar         *element_name,
                                 const gchar        **attribute_names,
                                 const gchar        **attribute_vals,
                                 gpointer             group_prefs_gpointer,
                                 GError             **error);

    public:
      GroupPrefs ();
      GroupPrefs (const StringView& filename);
      virtual ~GroupPrefs ();
      void save () const;

    private:
      typedef std::map<Quark,Prefs> group_prefs_t;
      mutable group_prefs_t _prefs;

    private:
      void from_string (const StringView&);
      void set_from_file (const StringView& filename);
      std::string _filename;
  };
}

#endif
