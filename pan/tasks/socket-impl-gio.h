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

#ifndef __SocketGIO_h__
#define __SocketGIO_h__

#include <string>
#include <glib/giochannel.h>
#include <glib/gstring.h>
#include <pan/tasks/socket.h>

namespace pan
{
  /**
   * glib implementation of Socket
   *
   * @ingroup tasks
   */
  class GIOChannelSocket: public Socket
  {
    public:
      GIOChannelSocket ();
      virtual ~GIOChannelSocket ();
      virtual bool open (const StringView& address, int port, std::string& setme_err);
      virtual void write_command (const StringView& chars, Listener *);
      virtual void get_host (std::string& setme) const;

    protected:
      GIOChannel * _channel;
      Listener * _listener;
      GString * _out_buf;
      GString * _in_buf;
      std::string _partial_read;
      bool _io_performed;
      enum DoResult { IO_ERR, IO_READ, IO_WRITE, IO_DONE };
      virtual DoResult do_read ();
      virtual DoResult do_write ();
      enum WatchMode { READ_NOW, WRITE_NOW, IGNORE_NOW };
      void set_watch_mode (WatchMode mode);

    private:
      unsigned int _tag_watch;
      unsigned int _tag_timeout;
      std::string _host;

    private:
      static gboolean gio_func (GIOChannel*, GIOCondition, gpointer);
      gboolean gio_func (GIOChannel*, GIOCondition);
      static gboolean timeout_func (gpointer);

    public:

      /**
       * Socket::Creator that instantiates GIOSocket objects.
       */
      class Creator: public Socket::Creator {
        public:
          virtual ~Creator () { }
          virtual void create_socket (const StringView& host, int port, WorkerPool&, Listener *l);
      };
  };
}

#endif
