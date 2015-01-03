
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
  char *bucket,
  char meth, char *host, char *path, char *query,
  char *sbody, FILE *fbody,
  size_t rcount)
{
  host = host ? host : "s3.amazonaws.com";
  if (bucket) host = flu_sprintf("%s.%s", bucket, host);
  else host = strdup(host);

  flu_list *headers = flu_list_malloc();

  //flu_list_set(headers, "_v", strdup("true"));

  fcla_sig4_sign(
    s, meth, host, path, query, headers, NULL, 0);

  char *uri = NULL;

  // TODO: s3.amazonaws.com/s3.your-domain.com/object
  //         if . in bucket name

  uri = flu_sprintf("https://%s/%s%s", host, path, query);
  //printf("uri: >%s<\n", uri);

  fcla_response *res = fcla_do_request(meth, uri, headers, NULL, NULL, NULL);

  free(uri);
  flu_list_free_all(headers);

  fcla_response_free(s->last_response);
  s->last_response = res;
  //flu_putf(fcla_response_to_s(res));

  free(host);

  //if (res->status_code != 307) { c->tmp_bucket = NULL; return res; }
  if (res->status_code != 307) return res;

  fprintf(stderr, "...redirection not yet implemented...\n");
  return NULL;

//  char *loc = flu_list_get(res->headers, "Location");
//  char *a = strstr(loc, ".s3-");
//  char *b = strstr(a, ".amazonaws.com/");
//  //c->tmp_endpoint = strndup(a + 1, b - a - 1);
//  //printf("tmp_endpoint: >%s<\n", c->tmp_endpoint);
//
//  return request(s, bucket, meth, host, path, query, sbody, fbody, rcount + 1);
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
  fcla_response *res = request(s, NULL, 'g', NULL, "", "", NULL, NULL, 0);

  return extract(res->body, "Name");
}

char *fcla_s3_read(fcla_sig4_session *s, const char *bucket, ...)
{
  va_list ap; va_start(ap, bucket);
  char *b = flu_svprintf(bucket, ap);
  char *fname = va_arg(ap, char *);
  char *f = flu_svprintf(fname, ap);
  va_end(ap);

  fcla_response *res = request(s, b, 'g', NULL, f, "", NULL, NULL, 0);

  free(b);
  free(f);

  return res->status_code == 200 ? strdup(res->body) : NULL;
}

