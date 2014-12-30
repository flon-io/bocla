
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "flutim.h"
#include "bocla.h"
#include "bocla_sig4.h"


fcla_sig4_session *fcla_sig4_session_init(
  const char *path, const char *service, const char *region)
{
  flu_dict *d = flu_readdict(path);
  if (d == NULL) return NULL; // failure

  fcla_sig4_session *s = calloc(1, sizeof(fcla_sig4_session));

  s->provider = flu_list_get(d, "provider");
  s->header = flu_list_get(d, "header");
  s->aki = flu_list_get(d, "aki");
  s->sak = flu_list_get(d, "sak");

  if (s->provider == NULL)
  {
    s->provider = strdup("aws");
    s->header = strdup("amz");
  }
  if (s->header == NULL)
  {
    s->header = strdup(s->provider);
  }

  size_t l = strlen(s->provider);
  s->provider_u = calloc(l + 1, sizeof(char));
  for (size_t i = 0; i < l; ++i) s->provider_u[i] = toupper(s->provider[i]);

  s->service = strdup(service);
  s->region = strdup(region);

  flu_list_free(d);

  return s;
}

void fcla_sig4_session_free(fcla_sig4_session *c)
{
  if (c == NULL) return;

  free(c->provider);
  free(c->provider_u);
  free(c->aki);
  free(c->sak);
  free(c->service);
  free(c->region);
  free(c);
}

static char *bin_to_hex(unsigned char *data, size_t len)
{
  char *r = calloc(2 * len + 1, sizeof(char));

  for (size_t i = 0; i < len; ++i) sprintf(r + 2 * i, "%02x", data[i]);

  return r;
}

static char *sha256(char *data, ssize_t len)
{
  if (len < 0) len = strlen(data);

  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)data, len, h);

  return bin_to_hex(h, SHA256_DIGEST_LENGTH);
}

static unsigned char *hmac_sha256(const char *key, const char *data)
{
  return HMAC(
    EVP_sha256(),
    key, strlen(key),
    (unsigned char *)data, strlen(data),
    NULL, NULL);
}

static char *canonical_uri()
{
  return strdup("canonical URI");
}
static char *canonical_query_string()
{
  return strdup("canonical query string");
}
static char *canonical_headers()
{
  return strdup("canonical query string");
}

static char *to_low(const char *s)
{
  char *r = strdup(s);
  for (size_t i = 0; ; ++i)
  {
    char c = r[i]; if (c == 0) break;
    r[i] = tolower(c);
  }

  return r;
}

static char *signed_headers(flu_dict *headers)
{
  flu_list *d = flu_list_dtrim(headers);

  flu_list *l = flu_list_malloc();
  for (flu_node *fn = headers->first; fn; fn = fn->next)
  {
    if (*fn->key == '_') continue;
    if (strcmp(fn->key, "date") == 0) continue;
    flu_list_add(l, to_low(fn->key));
  }
  flu_list_free(d);

  flu_list_isort(l, (int (*)(const void *, const void *))strcmp);

  flu_sbuffer *b = flu_sbuffer_malloc();

  for (flu_node *fn = l->first; fn; fn = fn->next)
  {
    flu_sbputs(b, fn->item);
    if (fn != l->last) flu_sbputc(b, ';');
  }
  flu_list_free_all(l);

  return flu_sbuffer_to_string(b);
}

static char *canonical_request(
  fcla_sig4_session *s, char meth, flu_dict *headers)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  if (meth == 'g') flu_sbputs(b, "GET\n");
  else if (meth == 'p') flu_sbputs(b, "POST\n");
  else if (meth == 'u') flu_sbputs(b, "PUT\n");
  else if (meth == 'd') flu_sbputs(b, "DELETE\n");
  else flu_sbputs(b, "HEAD\n"); // :-p

  flu_sbputs(b, canonical_uri());
  flu_sbputs(b, canonical_query_string());
  flu_sbputs(b, canonical_headers());
  flu_sbputs(b, signed_headers(headers));
  flu_sbputs(b, flu_list_get(headers, "x-%s-content-sha256", s->header));

  return flu_sbuffer_to_string(b);
}

static char *string_to_sign(
  fcla_sig4_session *s, char meth, flu_dict *headers)
{
  char *d = flu_list_get(headers, "x-%s-date", s->header);

  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbputs(b, s->provider_u);
  flu_sbputs(b, "4-HMAC-256\n");

  flu_sbputs(b, d);
  flu_sbputc(b, '\n');

  flu_sbwrite(b, d, 8);
  flu_sbprintf(b, "/%s/%s/%s4_request\n", s->region, s->service, s->provider);

  puts("...");
  flu_putf(canonical_request(s, meth, headers));
  puts("...");
  flu_sbputs(b, sha256(canonical_request(s, meth, headers), -1));

  return flu_sbuffer_to_string(b);
}

static char *signing_key()
{
  //unsigned char *date_key = ...
  return strdup("nada nada nada");
}

static char *signature(
  fcla_sig4_session *s, char meth, flu_dict *headers)
{
  puts("***"); flu_putf(string_to_sign(s, meth, headers)); puts("***");
  return bin_to_hex(
    hmac_sha256(
      signing_key(),
      string_to_sign(s, meth, headers)),
    32);
}

void fcla_sig4_sign(
  fcla_sig4_session *s,
  char meth, char *host, char *path, char *query, flu_dict *headers,
  char *body, size_t bodyl)
{
  struct timespec *now = NULL;
  flu_node *nnow = flu_list_getn(headers, "_date");
  if (nnow) { now = nnow->item; nnow->item = NULL; }
  else { now = flu_now(); }

  char *short_date = flu_tstamp(now, 1, 'd');
  char *gmt_date = flu_tstamp(now, 1, 'g');
  char *bigt_date = flu_tstamp(now, 1, 'T');
  free(now);

  flu_list_set(headers, "date", gmt_date);
  flu_list_set(headers, "x-%s-date", s->header, bigt_date);

  if (flu_list_get(headers, "Host") == NULL)
  {
    flu_list_set(headers, "host", strdup(host));
  }

  flu_list_set(headers, "x-%s-content-sha256", s->header, sha256(body, bodyl));

  char *sh = signed_headers(headers);

  char *sig = signature(s, meth, headers);

  flu_sbuffer *a = flu_sbuffer_malloc();

  flu_sbprintf(
    a, "%s4-HMAC-SHA256 ", s->provider_u);
  flu_sbprintf(
    a, "Credential=%s/%s/%s/%s/%s4_request,",
    s->aki, short_date, s->region, s->service, s->provider);
  flu_sbprintf(
    a, "SignedHeaders=%s,", sh);
  flu_sbprintf(
    a, "Signature=%s", sig);

  flu_putf(flu_list_to_sm(headers));

  free(sh);
  free(sig);

  //expect(flu_list_get(headers, "Authorization") === ""
  //  "AWS4-HMAC-SHA256 "
  //  "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
  //  "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
  //  "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");

  flu_list_set(headers, "Authorization", flu_sbuffer_to_string(a));
}

