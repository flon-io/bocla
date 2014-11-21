
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
  free(c);
}

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
  char *meth,
  char *host, char *path, char *query,
  flu_dict *headers)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbprintf(b, "%s\n", meth);

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
  char *meth, char *host, char *path, char *query,
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

  //printf("https://%s%s%s // req headers:\n", host, path, query);
  //for (flu_node *n = headers->first; n; n = n->next)
  //{
  //  printf("  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  //}
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

flu_list *fcla3_list_buckets(fcla3_context *c)
{
  flu_list *headers = flu_list_malloc();

  char *host = flu_sprintf("%s.amazonaws.com", c->endpoint);
  char *path = "/";
  char *query = "";

  sign(c, "GET", host, path, query, headers, NULL, 0);

  fcla_response *res =
    fcla_get_h("https://%s%s%s", host, path, query, headers);

  // TODO: keep last response in context...

  //flu_putf(fcla_response_to_s(res));

  free(host);
  flu_list_free_all(headers);

  flu_list *r = extract(res->body, "Name");

  fcla_response_free(res);

  return r;
}

