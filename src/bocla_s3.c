
//
// Copyright (c) 2013-2015, John Mettraux, jmettraux+flon@gmail.com
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

// https://github.com/flon-io/bocla

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include "bocla_s3.h"


static fcla_response *request(
  fcla_sig4_session *s,
  char meth, char *path, char *query,
  char *sbody, FILE *fbody,
  size_t rcount)
{
  //char *host = flu_sprintf(
  //  "%s.amazonaws.com",
  //  c->tmp_endpoint ? c->tmp_endpoint : c->endpoint);
  //c->tmp_endpoint = NULL;
  char *host = strdup("s3.amazonaws.com");

  flu_list *headers = flu_list_malloc();

  flu_list_set(headers, "_v", strdup("true"));

  fcla_sig4_sign(
    s, meth, host, path, query, headers, NULL, 0);

  //printf("https://%s%s%s // req headers:\n", host, path, query);
  //for (flu_node *n = headers->first; n; n = n->next)
  //{
  //  printf("  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  //}

  char *uri = NULL;

  // TODO: s3.amazonaws.com/s3.your-domain.com/object
  //         if . in bucket name

  //if (c->bucket && strcmp(path, "/") != 0)
  //  uri = flu_sprintf(
  //    "https://%s.%s/%s%s",
  //    c->tmp_bucket ? c->tmp_bucket : c->bucket, host, path, query);
  //else
  uri = flu_sprintf("https://%s/%s%s", host, path, query);
  //
  //printf("uri: >%s<\n", uri);

  fcla_response *res = fcla_do_request(meth, uri, headers, NULL, NULL, NULL);

  free(uri);
  flu_list_free_all(headers);

  fcla_response_free(s->last_response);
  s->last_response = res;
  flu_putf(fcla_response_to_s(res));

  free(host);

  //if (res->status_code != 307) { c->tmp_bucket = NULL; return res; }
  if (res->status_code != 307) return res;

  char *loc = flu_list_get(res->headers, "Location");
  char *a = strstr(loc, ".s3-");
  char *b = strstr(a, ".amazonaws.com/");
  //c->tmp_endpoint = strndup(a + 1, b - a - 1);
  //printf("tmp_endpoint: >%s<\n", c->tmp_endpoint);

  return request(s, meth, path, query, sbody, fbody, rcount + 1);
}

static flu_list *extract(char *body, char *key)
{
  flu_list *r = flu_list_malloc();

  char *a = NULL;
  char *b = body;
  char *skey = flu_sprintf("<%s>", key); size_t skl = strlen(skey);
  char *ekey = flu_sprintf("</%s>", key);

  while (1)
  {
    a = strstr(b, skey); if (a == NULL) break;
    b = strstr(b + 1, ekey);

    flu_list_add(r, strndup(a + skl, b - a - skl));
  }

  free(skey);
  free(ekey);

  return r;
}

flu_list *fcla_s3_list_buckets(fcla_sig4_session *s)
{
  fcla_response *res = request(s, 'g', "", "", NULL, NULL, 0);

  return extract(res->body, "Name");
}

