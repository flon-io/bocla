
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
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "flutim.h"
#include "bocla.h"
#include "bocla_sig4.h"


fcla_sig4_context *fcla_sig4_context_init(const char *path)
{
  flu_dict *d = flu_readdict(path);
  if (d == NULL) return NULL; // failure

  fcla_sig4_context *c = calloc(1, sizeof(fcla_sig4_context));

  c->provider = flu_list_get(d, "provider");
  c->aki = flu_list_get(d, "aki");
  c->sak = flu_list_get(d, "sak");

  if (c->provider == NULL) c->provider = strdup("aws");

  flu_list_free(d);

  return c;
}

void fcla_sig4_context_free(fcla_sig4_context *c)
{
  if (c == NULL) return;

  free(c->provider);
  free(c->aki);
  free(c->sak);
  free(c);
}

static char *to_hex_string(unsigned char *data)
{
  char *r = calloc(65, sizeof(char));

  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    sprintf(r + 2 * i, "%02x", data[i]);

  return r;
}

void fcla_sig4_sign(
  fcla_sig4_context *c, char *service, char *region,
  char meth, char *host, char *path, char *query, flu_dict *headers,
  char *body, size_t bodyl)
{
  struct timespec *now = flu_now();
  char *gmt_date = flu_tstamp(now, 1, 'g');
  char *bigt_date = flu_tstamp(now, 1, 'T');
  free(now);

  flu_list_set(headers, "date", gmt_date);
  flu_list_set(headers, "x-%s-date", c->provider, bigt_date);

  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)body, bodyl, h);

  flu_list_set(headers, "x-%s-content-sha256", c->provider, to_hex_string(h));

flu_putf(flu_list_to_s(headers));

  //expect(flu_list_get(headers, "Authorization") === ""
  //  "AWS4-HMAC-SHA256 "
  //  "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
  //  "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
  //  "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
}

