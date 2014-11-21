
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

// https://github.com/flon-io/bocla

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>

#include "flu64.h"
#include "flutim.h"
#include "bocla.h"
#include "bocla3.h"


void fcla3_context_free(fcla3_context *c)
{
  if (c == NULL) return; // behave like free() itself
  free(c->endpoint);
  free(c->aki);
  free(c->sak);
  free(c->bucket);
  fcla_response_free(c->last_response);
  free(c);
}

  // TODO: make that flu_strcmp() maybe...
  //
static int _strcmp(const void *a, const void *b)
{
  return strcmp((char *)a, (char *)b);
}

static char *canonicalize_headers(flu_dict *headers)
{
  flu_list *l = flu_list_malloc();

  for (flu_node *n = headers->first; n; n = n->next)
  {
    if (strncmp(n->key, "x-amz-", 6) == 0) flu_list_add(l, n->key);
  }

  if (l->size < 1) { flu_list_free(l); return NULL; }

  flu_list_isort(l, _strcmp);

  flu_sbuffer *b = flu_sbuffer_malloc();

  for (flu_node *n = l->first; n; n = n->next)
  {
    char *k = n->item;
    flu_sbprintf(b, "%s:%s\n", k, (char *)flu_list_get(headers, k));
      // the k, when printed, should be lowercase
  }

  char *r = flu_sbuffer_to_string(b);
  r[strlen(r) - 1] = 0;

  return r;
}

/*
#S3_PARAMS =
#  %w[
#    acl location logging notification partNumber policy
#    requestPayment torrent uploadId uploads versionId
#    versioning versions delete lifecycle
#  ] +
#  %w[
#    response-content-type response-content-language
#    response-expires response-cache-control
#    response-content-disposition response-content-encoding
#  ]
*/

static char *canonicalize_resource(
  fcla3_context *c, char *host, char *path, char *query)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  if (c->bucket) flu_sbprintf(b, "/%s", c->bucket);
  flu_sbputs(b, path);
  if (strstr(query, "delete=")) flu_sbputs(b, "?delete");

  return flu_sbuffer_to_string(b);
}

static char *string_to_sign(
  fcla3_context *c,
  char meth,
  char *host, char *path, char *query,
  flu_dict *headers)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  if (meth == 'g') flu_sbputs(b, "GET\n");
  else if (meth == 'p') flu_sbputs(b, "POST\n");
  else if (meth == 'u') flu_sbputs(b, "PUT\n");
  else if (meth == 'd') flu_sbputs(b, "DELETE\n");
  else flu_sbputs(b, "HEAD\n"); // :-p

  flu_sbprintf(b, "%s\n", flu_list_getd(headers, "content-md5", ""));
  flu_sbprintf(b, "%s\n", flu_list_getd(headers, "content-type", ""));
  flu_sbprintf(b, "%s\n", flu_list_get(headers, "date"));

  char *hs = canonicalize_headers(headers);
  if (hs) { flu_sbputs(b, hs); free(hs); }

  char *pa = canonicalize_resource(c, host, path, query);
  flu_sbputs(b, pa); free(pa);

  return flu_sbuffer_to_string(b);
}

static void sign(
  fcla3_context *c,
  char meth, char *host, char *path, char *query,
  flu_dict *headers,
  char *body, size_t bodyl)
{
  flu_list_set(headers, "date", flu_tstamp(NULL, 0, '2'));

  if (body)
  {
    char *type =
      strncmp(body, "<?xml", 5) == 0 ? "application/xml" : "text/plain";

    flu_list_set(headers, "content-type", strdup(type));

    unsigned char d[16];
    MD5((unsigned char *)body, bodyl, d);

    flu_list_set(headers, "content-md5", flu64_encode((char *)d, 16));
  }

  char *string = string_to_sign(c, meth, host, path, query, headers);

  //printf("string to sign: >%s<\n", string);

  unsigned char *d =
    HMAC(
      EVP_sha1(),
      c->sak, strlen(c->sak),
      (unsigned char *)string, strlen(string),
      NULL, NULL);
  free(string);

  char *sig = flu64_encode((char *)d, 20);
  char *auth = flu_sprintf("AWS %s:%s", c->aki, sig);
  free(sig);

  flu_list_set(headers, "authorization", auth);
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

static fcla_response *request(
  fcla3_context *c,
  char meth, char *path, char *query,
  char *sbody, FILE *fbody,
  size_t rcount)
{
  char *host = flu_sprintf(
    "%s.amazonaws.com",
    c->tmp_endpoint ? c->tmp_endpoint : c->endpoint);
  c->tmp_endpoint = NULL;

  flu_list *headers = flu_list_malloc();

  sign(c, meth, host, path, query, headers, NULL, 0);

  //printf("https://%s%s%s // req headers:\n", host, path, query);
  //for (flu_node *n = headers->first; n; n = n->next)
  //{
  //  printf("  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  //}

  char *uri = NULL;

  // TODO: s3.amazonaws.com/s3.your-domain.com/object
  //         if . in bucket name

  if (c->bucket && strcmp(path, "/") != 0)
    uri = flu_sprintf("https://%s.%s%s%s", c->bucket, host, path, query);
  else
    uri = flu_sprintf("https://%s%s%s", host, path, query);
  //printf("uri: >%s<\n", uri);

  fcla_response *res = fcla_do_request(meth, uri, headers, NULL, NULL);

  flu_list_free_all(headers);

  fcla_response_free(c->last_response);
  c->last_response = res;
  //flu_putf(fcla_response_to_s(res));

  free(host);

  if (res->status_code != 307) return res;

  char *loc = flu_list_get(res->headers, "Location");
  char *a = strstr(loc, ".s3-");
  char *b = strstr(a, ".amazonaws.com/");
  c->tmp_endpoint = strndup(a + 1, b - a - 1);
  //printf("tmp_endpoint: >%s<\n", c->tmp_endpoint);

  return request(c, meth, path, query, sbody, fbody, rcount + 1);
}

flu_list *fcla3_list_buckets(fcla3_context *c)
{
  fcla_response *res = request(c, 'g', "/", "", NULL, NULL, 0);

  return extract(res->body, "Name");
}

char *fcla3_read(fcla3_context *c, const char *fname, ...)
{
  va_list ap; va_start(ap, fname);
  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbputs(b, "/");
  flu_sbvprintf(b, fname, ap);
  char *f = flu_sbuffer_to_string(b);
  va_end(ap);

  fcla_response *res = request(c, 'g', f, "", NULL, NULL, 0);

  free(f);

  return res->status_code == 200 ? strdup(res->body) : NULL;
}

