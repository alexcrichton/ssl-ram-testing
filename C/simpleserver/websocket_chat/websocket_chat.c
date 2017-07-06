/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "mongoose.h"
#include <openssl/ssl.h>

static sig_atomic_t s_signal_received = 0;
static const char *s_http_port = "9000";
static struct mg_serve_http_opts s_http_server_opts;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct websocket_message *wm = (struct websocket_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      /* Usual HTTP request - serve static files */
      mg_serve_http(nc, hm, s_http_server_opts);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    case MG_EV_WEBSOCKET_FRAME:
      /* New websocket message. Send it back. */
      mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, wm->data, wm->size);
      break;
    default:
      break;
  }
}

int main(void) {
  struct mg_mgr mgr;
  struct mg_connection *nc;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, s_http_port, ev_handler);
  s_http_server_opts.document_root = ".";
  mg_set_protocol_http_websocket(nc);
  mg_set_ssl(nc, "ssl.pem", NULL);
  SSL_CTX_set_mode(nc->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);


  printf("Started on port %s\n", s_http_port);
  while (s_signal_received == 0) {
    mg_mgr_poll(&mgr, 200);
  }
  mg_mgr_free(&mgr);

  return 0;
}
