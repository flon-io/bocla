
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
  s->aki = flu_list_get(d, "aki");
  s->sak = flu_list_get(d, "sak");

  if (s->provider == NULL) s->provider = strdup("aws");

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

static char *string_to_sign()
{
  return strdup("nada nada nada");
}

static char *signing_key()
{
  //unsigned char *date_key = ...
  return strdup("nada nada nada");
}

static unsigned char *hmac_sha256(const char *key, const char *data)
{
  return HMAC(
    EVP_sha256(),
    key, strlen(key),
    (unsigned char *)data, strlen(data),
    NULL, NULL);
}

static char *signed_headers(flu_dict *headers)
{
  return strdup("host;range");
}

static char *signature(
  fcla_sig4_session *s)
{
  return bin_to_hex(
    hmac_sha256(
      signing_key(),
      string_to_sign()),
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
  flu_list_set(headers, "x-%s-date", s->provider, bigt_date);

  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)body, bodyl, h);

  flu_list_set(
    headers,
    "x-%s-content-sha256", s->provider,
    bin_to_hex(h, SHA256_DIGEST_LENGTH));

  char *sh = signed_headers(headers);

  char *sig = signature(s);

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

