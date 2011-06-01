/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// :tabSize=2:indentSize=2:noTabs=true:
/*
 * Pan - A Newsreader for Gtk+
 * Copyright (C) 2011  K. Haley <haleykd@users.sf.net>
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

#ifndef __SocketGNUTLS_h__
#define __SocketGNUTLS_h__

#include <pan/tasks/socket-impl-gio.h>

namespace pan
{
  /**
   * GnuTLS implementation of Socket
   *
   * @ingroup tasks
   */
  class SocketTLS: public GIOChannelSocket
  {
    public:
      SocketTLS();
      void starttls();
    private:
      bool handshake;
      bool tls;
      gnutls_session_t session;
      gnutls_anon_client_credentials_t anoncred;

      virtual DoResult do_read ();
      virtual DoResult do_write ();
      static ssize_t read_p(SocketTLS *, char *, int);
      static ssize_t write_p(SocketTLS *, char *, int);
  };
}
#endif
