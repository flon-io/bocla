
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

#include <ctype.h>
#include <stdlib.h>

#include "flutil.h"
#include "aabro.h"
#include "shv_protected.h"

//#include "gajeta.h"


abr_parser *request_parser = NULL;


static void shv_init_parser()
{
  abr_parser *sp = abr_string(" ");
  abr_parser *crlf = abr_string("\r\n");

  abr_parser *lws = abr_rex("(\r\n)?[ \t]+");

  //abr_parser *text =
  //  abr_alt(abr_rex("[^\x01-\x1F\x7F]"), lws, abr_r("+"));
  abr_parser *text =
    abr_rex("[^\x01-\x1F\x7F]+");

  abr_parser *token =
    abr_rex("[^\x01-\x1F\x7F()<>@,;:\\\\\"/[\\]?={} \t]+");

  abr_parser *method =
    abr_n_alt(
      "method",
      abr_rex("GET|POST|PUT|DELETE|HEAD|OPTIONS|TRACE|CONNECT|LINK|UNLINK"),
      abr_name("extension_method", token),
      NULL);
  abr_parser *request_uri =
    abr_n_rex("request_uri", "[^ \t\r\n]{1,2048}"); // arbitrary limit
  abr_parser *http_version =
    abr_n_rex("http_version", "HTTP/[0-9]+\\.[0-9]+");

  abr_parser *request_line =
    abr_seq(method, sp, request_uri, sp, http_version, crlf, NULL);

  abr_parser *field_content =
    text;

  abr_parser *field_name =
    abr_name("field_name", token);
  abr_parser *field_value =
    abr_n_rep("field_value", abr_alt(field_content, lws, NULL), 0, -1);

  abr_parser *message_header =
    abr_n_seq("message_header", field_name, abr_string(":"), field_value, NULL);

  //abr_parser *message_body =
  //  abr_n_regex("message_body", "^.+"); // well, the rest

  request_parser =
    abr_seq(
      request_line,
      abr_seq(message_header, crlf, NULL), abr_q("*"),
      crlf,
      //abr_rep(message_body, 0, 1),
      NULL);
  // do not include the message_body

  //puts(abr_parser_to_string(request_parser));
}

shv_request *shv_parse_request(char *s)
{
  //
  // parse

  if (request_parser == NULL) shv_init_parser();

  abr_tree *r = abr_parse(s, 0, request_parser);
  //abr_tree *r = abr_parse_f(s, 0, request_parser, ABR_F_ALL);

  //puts(abr_tree_to_string_with_leaves(s, r));

  shv_request *req = calloc(1, sizeof(shv_request));
  req->startMs = flu_getMs();
  req->status_code = 400; // Bad Request

  if (r->result != 1) { abr_tree_free(r); return req; }

  req->status_code = 200; // ok, for now

  abr_tree *t = NULL;

  // method

  t = abr_tree_lookup(r, "method");
  req->method = shv_method_to_char(abr_tree_str(s, t));

  // uri

  t = abr_tree_lookup(r, "request_uri");
  req->uri = abr_tree_string(s, t);

  // version

    // reject when not 1.0 or 1.1?

  // headers

  flu_list *hs = abr_tree_list_named(r, "message_header");

  req->headers = flu_list_malloc();
  for (flu_node *h = hs->first; h != NULL; h = h->next)
  {
    abr_tree *th = (abr_tree *)h->item;
    abr_tree *tk = abr_tree_lookup(th, "field_name");
    abr_tree *tv = abr_tree_lookup(th, "field_value");

    char *sk = abr_tree_string(s, tk);
    for (char *kk = sk; *kk; ++kk) *kk = tolower(*kk);

    char *sv = abr_tree_string(s, tv);

    flu_list_set(req->headers, sk, flu_strtrim(sv));

    free(sk);
    free(sv);
  }

  flu_list_free(hs);

  req->uri_d =
    shv_parse_host_and_path(
      flu_list_get(req->headers, "host"),
      req->uri);

  //
  // over

  abr_tree_free(r);

  return req;
}

ssize_t shv_request_content_length(shv_request *r)
{
  char *cl = flu_list_get(r->headers, "content-length");

  return (cl == NULL) ? -1 : atol(cl);
}

void shv_request_free(shv_request *r)
{
  if (r->uri != NULL) free(r->uri);
  if (r->uri_d != NULL) flu_list_free_all(r->uri_d);
  if (r->headers != NULL) flu_list_free_all(r->headers);
  free(r);
}

