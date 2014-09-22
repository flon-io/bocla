
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
#include <unistd.h>
#include <string.h>

#include "gajeta.h"
#include "shervin.h"
#include "shv_protected.h"


void grey_logger(char level, const char *pref, const char *msg)
{
  char *lstr = fgaj_level_to_string(level);

  printf("[1;30m%19s %-38s %s[0;0m\n", lstr, pref, msg);

  fgaj_level_string_free(lstr);
}

static int hello_handler(
  shv_request *req, flu_dict *rod, shv_response *res, flu_dict *params)
{
  res->status_code = 200;
  //flu_list_set(res->headers, "content-type", "text/plain; charset=utf-8");

  flu_list_add(res->body, strdup("**hello world**\n"));

  return 1;
}

static int mirror_handler(
  shv_request *req, flu_dict *rod, shv_response *res, flu_dict *params)
{
  res->status_code = 200;
  //flu_list_set(res->headers, "content-type", "text/plain; charset=utf-8");

  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbprintf(
    b, "%s %s HTTP/1.1\r\n", shv_char_to_method(req->method), req->uri);

  for (flu_node *n = req->headers->first; n; n = n->next)
  {
    flu_sbprintf(b, "%s: %s\r\n", n->key, n->item);
  }
  flu_sbprintf(b, "method: %s\r\n", shv_char_to_method(req->method));
  flu_sbprintf(b, "path: %s\r\n", req->uri);
  flu_sbputs(b, "\r\n");
  if (req->body) flu_sbputs(b, req->body);

  flu_list_add(res->body, flu_sbuffer_to_string(b));

  return 1;
}

static int delete_handler(
  shv_request *req, flu_dict *rod, shv_response *res, flu_dict *params)
{
  res->status_code = 200;
  //flu_list_set(res->headers, "content-type", "text/plain; charset=utf-8");

  flu_list_add(res->body, strdup("deleted."));

  return 1;
}

static int null_handler(
  shv_request *req, flu_dict *rod, shv_response *res, flu_dict *params)
{
  res->status_code = 200;

  flu_list_add(res->body, strdup("ok."));

  return 1;
}

int main()
{
  fgaj_conf_get()->logger = grey_logger;
  //fgaj_conf_get()->level = 10;

  shv_route *routes[] = {
    shv_rp("GET /hello", hello_handler, NULL),
    shv_rp("/mirror", mirror_handler, NULL),
    shv_rp("DELETE /d", delete_handler, NULL),
    shv_rp("/null", null_handler, NULL),
    NULL // end route
  };

  shv_serve(4567, routes);
}

