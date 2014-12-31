
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

  s->provider = strdup(flu_list_get(d, "provider"));
  s->header = strdup(flu_list_get(d, "header"));
  s->aki = strdup(flu_list_get(d, "aki"));
  s->sak = strdup(flu_list_get(d, "sak"));

  flu_list_free_all(d);

  if (s->provider == NULL)
  {
    s->provider = strdup("aws");
    free(s->header); s->header = strdup("amz");
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

  return s;
}

void fcla_sig4_session_free(fcla_sig4_session *c)
{
  if (c == NULL) return;

  free(c->provider);
  free(c->provider_u);
  free(c->header);
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

static char *sha256(void *data, ssize_t len)
{
  if (len < 0) len = strlen(data);

  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)data, len, h);

  return bin_to_hex(h, SHA256_DIGEST_LENGTH);
}

static unsigned char *hmac_sha256(void *key, ssize_t klen, char *data)
{
  unsigned char *r = calloc(SHA256_DIGEST_LENGTH, sizeof(char));

  return HMAC(
    EVP_sha256(),
    key, klen < 0 ? strlen(key) : klen,
    (unsigned char *)data, strlen(data),
    r, NULL);
}

static char *hmac_sha256_hex(void *key, ssize_t klen, char *data)
{
  unsigned char *sha = hmac_sha256(key, klen, data);
  char *hex = bin_to_hex(sha, 32);
  free(sha);

  return hex;
}

static char *canonical_uri(
  fcla_sig4_session *s, fcla_sig4_request *r)
{
  return strdup(r->path);
}

static char *canonical_query_string(
  fcla_sig4_session *s, fcla_sig4_request *r)
{
  return strdup(r->query);
    // TODO: sorted by query parameters
}

static char *hcaseget(flu_dict *hs, const char *k)
{
  for (flu_node *fn = hs->first; fn; fn = fn->next)
  {
    if (strcasecmp(fn->key, k) == 0) return fn->item;
  }

  return NULL;
}

static char *canonical_headers(
  fcla_sig4_session *s, fcla_sig4_request *r)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_list *l = flu_split(r->signed_headers, ";");
  for (flu_node *fn = l->first; fn; fn = fn->next)
  {
    char *key = fn->item; if (*key == '_') continue;
    flu_sbputs(b, key);
    flu_sbputc(b, ':');
    flu_sbputs(b, hcaseget(r->headers, key));
    flu_sbputc(b, '\n');
  }
  flu_list_free_all(l);

  return flu_sbuffer_to_string(b);
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
  fcla_sig4_session *s, fcla_sig4_request *r)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  if (r->meth == 'g') flu_sbputs(b, "GET\n");
  else if (r->meth == 'p') flu_sbputs(b, "POST\n");
  else if (r->meth == 'u') flu_sbputs(b, "PUT\n");
  else if (r->meth == 'd') flu_sbputs(b, "DELETE\n");
  else flu_sbputs(b, "HEAD\n"); // :-p

  flu_sbputs_f(b, canonical_uri(s, r)); flu_sbputc(b, '\n');
  flu_sbputs_f(b, canonical_query_string(s, r)); flu_sbputc(b, '\n');
  flu_sbputs_f(b, canonical_headers(s, r)); flu_sbputc(b, '\n');
  flu_sbputs(b, r->signed_headers); flu_sbputc(b, '\n');
  flu_sbputs(b, flu_list_get(r->headers, "x-%s-content-sha256", s->header));

  return flu_sbuffer_to_string(b);
}

static char *string_to_sign(fcla_sig4_session *s, fcla_sig4_request *r)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbputs(b, s->provider_u);
  flu_sbputs(b, "4-HMAC-256\n");

  flu_sbputs(b, r->date);
  flu_sbputc(b, '\n');

  flu_sbprintf(
    b, "%s/%s/%s/%s4_request\n", r->date, s->region, s->service, s->provider);

  char *cr = canonical_request(s, r);
  puts("... canonical_request"); puts(cr); puts("...");
  flu_sbputs_f(b, sha256(cr, -1));
  free(cr);

  return flu_sbuffer_to_string(b);
}

// specs only
//
char *fcla_sig4_string_to_sign(fcla_sig4_session *s, fcla_sig4_request *r)
{
  return string_to_sign(s, r);
}

unsigned char *signing_key(fcla_sig4_session *s, fcla_sig4_request *r)
{
  char *sak = flu_sprintf("%s4%s", s->provider_u, s->sak);

  unsigned char *date_key =
    hmac_sha256(sak, -1, r->date);
  unsigned char *date_region_key =
    hmac_sha256(date_key, 32, s->region);
  unsigned char *date_region_service_key =
    hmac_sha256(date_region_key, 32, s->service);

  char *as = flu_sprintf("%s4_request", s->provider);
  unsigned char *signing_key = hmac_sha256(date_region_service_key, 32, as);

  flu_zero_and_free(sak, -1);
  free(date_key);
  free(date_region_key);
  free(date_region_service_key);
  free(as);

  return signing_key;
}

// specs only
//
char *fcla_sig4_signing_key(fcla_sig4_session *s, fcla_sig4_request *r)
{
  unsigned char *sk = signing_key(s, r);
  char *hsk = bin_to_hex(sk, 32);
  free(sk);

  return hsk;
}

static char *signature(fcla_sig4_session *s, fcla_sig4_request *r)
{
  char *sk = signing_key(s, r);
  char *sts = string_to_sign(s, r);
  puts("*** string_to_sign"); puts(sts); puts("***");
  char *sig = hmac_sha256_hex(sk, 32, sts);
  free(sk);
  free(sts);

  return sig;
}

void fcla_sig4_sign(
  fcla_sig4_session *s,
  char meth, char *host, char *path, char *query, flu_dict *headers,
  char *body, size_t bodyl)
{
  fcla_sig4_request req;
  req.meth = meth;
  req.host = host;
  req.path = path;
  req.query = query;
  req.headers = headers;
  req.body = body;
  req.bodyl = bodyl;

  struct timespec *now = NULL;
  flu_node *nnow = flu_list_getn(req.headers, "_date");
  if (nnow) { now = nnow->item; nnow->item = NULL; }
  else { now = flu_now(); }

  req.date = flu_tstamp(now, 1, 'd');
  char *gmt_date = flu_tstamp(now, 1, 'g');
  char *bigt_date = flu_tstamp(now, 1, 'T');
  free(now);

  flu_list_set(req.headers, "date", gmt_date);
  flu_list_set(req.headers, "x-%s-date", s->header, bigt_date);

  if (flu_list_get(req.headers, "Host") == NULL)
  {
    flu_list_set(req.headers, "host", strdup(host));
  }

  flu_list_set(
    req.headers, "x-%s-content-sha256", s->header, sha256(body, bodyl));

  req.signed_headers = signed_headers(req.headers);

  char *sig = signature(s, &req);

  flu_sbuffer *a = flu_sbuffer_malloc();

  flu_sbprintf(
    a, "%s4-HMAC-SHA256 ", s->provider_u);
  flu_sbprintf(
    a, "Credential=%s/%s/%s/%s/%s4_request,",
    s->aki, req.date, s->region, s->service, s->provider);
  flu_sbprintf(
    a, "SignedHeaders=%s,", req.signed_headers);
  flu_sbprintf(
    a, "Signature=%s", sig);

  flu_putf(flu_list_to_sm(headers));

  free(req.date);
  free(req.signed_headers);
  free(sig);

  //expect(flu_list_get(headers, "Authorization") === ""
  //  "AWS4-HMAC-SHA256 "
  //  "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
  //  "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
  //  "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");

  flu_list_set(headers, "Authorization", flu_sbuffer_to_string(a));
}

