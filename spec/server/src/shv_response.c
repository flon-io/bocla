
//
// Copyright (c) 2013-2014, John Mettraux, jmettraux+flon@gmail.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Made in Japan.
//

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ev.h>

#include "gajeta.h"
#include "shervin.h"
#include "shv_protected.h"


//
// shv_response

shv_response *shv_response_malloc(short status_code)
{
  shv_response *r = calloc(1, sizeof(shv_response));
  r->status_code = status_code;
  //r->content_type = NULL;
  //r->body = NULL;

  return r;
}

void shv_response_free(shv_response *r)
{
  if (r->body != NULL) free(r->body);
  free(r);
}

//
// shv_respond

static char *shv_reason(short status_code)
{
  if (status_code == 200) return "OK";

  if (status_code == 400) return "Bad Request";
  if (status_code == 404) return "Not Found";

  if (status_code == 500) return "Internal Server Error";

  if (status_code == 100) return "Continue";
  if (status_code == 101) return "Switching Protocols";
  if (status_code == 201) return "Created";
  if (status_code == 202) return "Accepted";
  if (status_code == 203) return "Non-Authoritative Information";
  if (status_code == 204) return "No Content";
  if (status_code == 205) return "Reset Content";
  if (status_code == 206) return "Partial Content";
  if (status_code == 300) return "Multiple Choices";
  if (status_code == 301) return "Moved Permanently";
  if (status_code == 302) return "Found";
  if (status_code == 303) return "See Other";
  if (status_code == 304) return "Not Modified";
  if (status_code == 305) return "Use Proxy";
  if (status_code == 307) return "Temporary Redirect";
  if (status_code == 401) return "Unauthorized";
  if (status_code == 402) return "Payment Required";
  if (status_code == 403) return "Forbidden";
  if (status_code == 405) return "Method Not Allowed";
  if (status_code == 406) return "Not Acceptable";
  if (status_code == 407) return "Proxy Authentication Required";
  if (status_code == 408) return "Request Time-out";
  if (status_code == 409) return "Conflict";
  if (status_code == 410) return "Gone";
  if (status_code == 411) return "Length Required";
  if (status_code == 412) return "Precondition Failed";
  if (status_code == 413) return "Request Entity Too Large";
  if (status_code == 414) return "Request-URI Too Large";
  if (status_code == 415) return "Unsupported Media Type";
  if (status_code == 416) return "Requested range not satisfiable";
  if (status_code == 417) return "Expectation Failed";
  if (status_code == 501) return "Not Implemented";
  if (status_code == 502) return "Bad Gateway";
  if (status_code == 503) return "Service Unavailable";
  if (status_code == 504) return "Gateway Time-out";
  if (status_code == 505) return "HTTP Version not supported";
  return "(no reason-phrase)";
}

void shv_respond(short status_code, struct ev_loop *l, struct ev_io *eio)
{
  shv_con *con = (shv_con *)eio->data;

  if (status_code == -1)
  {
    if (con->res) status_code = con->res->status_code;
    else status_code = con->req->status_code;
  }

  char *content_type = "text/plain; charset=utf-8";
  char *body = "";
  //
  if (con->res)
  {
    if (con->res->content_type) content_type = con->res->content_type;
    if (con->res->body) body = con->res->body;
  }

  time_t tt; time(&tt);
  struct tm *tm; tm = gmtime(&tt);
  char *dt = asctime(tm); // TODO: upgrade to rfc1123
  dt[strlen(dt) - 1] = '\0';

  char *lo = "northpole"; // FIXME

  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbprintf(b, "HTTP/1.1 %i %s\r\n", status_code, shv_reason(status_code));
  flu_sbprintf(b, "Server: shervin %s\r\n", SHV_VERSION);
  flu_sbprintf(b, "Location: %s\r\n", lo);
  flu_sbprintf(b, "Date: %s\r\n", dt);
  flu_sbprintf(b, "Content-Type: %s\r\n", content_type);
  flu_sbprintf(b, "Content-Length: %zu\r\n", strlen(body));

  long long now = flu_getMs();
  flu_sbprintf(b,
    "x-flon-shervin: c%.3fms;r%.3fms;rq%i\r\n",
    (now - con->startMs) / 1000.0,
    (now - con->req->startMs) / 1000.0,
    con->rqount);

  flu_sbprintf(b, "\r\n");

  //free(dt); // not necessary

  flu_sbprintf(b, body);

  flu_sbuffer_close(b);

  send(eio->fd, b->string, b->len, 0);

  flu_sbuffer_free(b);

  now = flu_getMs();
  fgaj_i(
    "c%p r%i %s %s %s %i c%.3fms r%.3fms",
    eio, con->rqount,
    inet_ntoa(con->client->sin_addr),
    shv_char_to_method(con->req->method),
    con->req->uri,
    status_code,
    (flu_getMs() - con->startMs) / 1000.0,
    (flu_getMs() - con->req->startMs) / 1000.0);

  shv_con_reset(con);
}

