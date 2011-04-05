// :tabSize=2:indentSize=2:noTabs=true:

#include <errno.h>
#include <pan/tasks/socket-gnutls.h>

using namespace pan;

SocketTLS::SocketTLS() : handshake(false), tls(false)
{
}

ssize_t SocketTLS::read_p(SocketTLS *self, char *data, int len)
{
  gsize in;
  GIOChannelError *err;
  GIOStatus stat = g_io_channel_read_chars(self->_channel, data, len, &in, &err);

  if (stat == G_IO_STATUS_AGAIN)
    gnutls_transport_set_errno(self->session, EAGAIN);
  else if (stat != G_IO_STATUS_NORMAL)
    gnutls_transport_set_errno(self->session, (int) *err);
  g_clear_error(&err);

  return (stat==G_IO_NORMAL)? in : -1;
}
ssize_t SocketTLS::write_p(SocketTLS *self, char *data, int len)
{
  gsize out;
  GIOChannelError *err;
  GIOStatus stat = g_io_channel_write_chars(self->_channel, data, len, &out, &err);

  if (stat == G_IO_STATUS_AGAIN)
    gnutls_transport_set_errno(self->session, EAGAIN);
  else if (stat != G_IO_STATUS_NORMAL)
    gnutls_transport_set_errno(self->session, (int) *err);
  else // OK
  {
    g_io_channel_flush(self->_channel, NULL);
    return out;
  }
  g_clear_error(&err);
  return -1;
}

void SocketTLS::starttls()
{
  int ret;
  handshake = true;

  gnutls_anon_allocate_client_credentials (&anoncred);
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_credentials_set (session, GNUTLS_CRD_ANON, anoncred);
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) _channel);
  gnutls_transport_set_pull_function (session, (gnutls_pull_func) read_p);
  gnutls_transport_set_pull_function (session, (gnutls_push_func) write_p);
  gnutls_transport_set_lowat (session, 0);
  ret = gnutls_handshake (session);
  if (ret == GNUTLS_E_AGAIN)
    if ( gnutls_record_get_direction(session) )
      set_watch_mode (WRITE_NOW);
    else
      set_watch_mode (READ_NOW);
  else if (ret == 0)
  {
    // handshake already finished
    // should never happen
    handshake = false;
    tls = true;
    _listener->on_socket_response (this, StringView("200 tls handshake done\r\n"));
  }
  else
  {
    // fatal handshake
    handshake = false;
    _listener->on_socket_response (this, StringView("500 tls handshake failed\r\n"));
  }
}
