
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
  free(c->endpoint);
  free(c->aki);
  free(c->sak);
  free(c->bucket);
}

/*
def canonicalized_amz_headers(headers)

  s = headers.select { |k, v|
    k.match(/^x-amz-/i)
  }.collect { |k, v|
    [ k.downcase, v ]
  }.sort_by { |k, v|
    k
  }.collect { |k, v|
    "#{k}:#{v}"
  }.join("\n")

  s == '' ? nil : s
end

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

def string_to_sign(meth, uri, headers)

  hs = canonicalized_amz_headers(headers)

  a = []
  a << meth.to_s.upcase
  a << headers['content-md5']
  a << headers['content-type']
  a << headers['date']
  a << hs if hs
  a << canonicalized_resource(uri)

  a.join("\n")
end
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
  // hs
  char *pa = canonicalize_resource(c, host, path, query);
  flu_sbputs(b, pa);

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

  printf("string to sign: >%s<\n", string);

  unsigned char *d =
    HMAC(
      EVP_sha1(),
      c->sak, strlen(c->sak),
      (unsigned char *)string, strlen(string),
      NULL, NULL);

  char *auth = flu_sprintf("AWS %s:%s", c->aki, flu64_encode((char *)d, 20));

  flu_list_set(headers, "authorization", auth);

  printf("https://%s%s%s // req headers:\n", host, path, query);
  for (flu_node *n = headers->first; n; n = n->next)
  {
    printf("  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  }
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

  flu_putf(fcla_response_to_s(res));

  flu_list_free(headers);

  return flu_list_malloc();
}

