
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
def canonicalized_resource(uri)

  r = []
  r << "/#{@bucket}" if @bucket
  r << uri.path

  #q = query_string.select { |k, v|
  #  S3_PARAMS.include?(k)
  #}.to_a.sort_by { |k, v|
  #  k
  #}.collect { |k, v|
  #  "#{k}=#{v}"
  #}
  #r << '?' + q.join('&') if q.any?

  r << '?delete' if uri.query == 'delete'

  r.join
end

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
static char *string_to_sign(char *meth, char *uri, flu_dict *headers)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbputs(b, meth); flu_sbputs(b, "\n");
  flu_sbprintf(b, "%s\n", flu_list_getd(headers, "content-md5", ""));
  flu_sbprintf(b, "%s\n", flu_list_getd(headers, "content-type", ""));
  flu_sbprintf(b, "%s\n", flu_list_get(headers, "date"));
  // hs
  flu_sbprintf(b, "%s", "/");

  return flu_sbuffer_to_string(b);
}

static void sign(
  fcla3_context *c,
  char *meth, char *uri,
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

  //headers['authorization'] =
  //  [
  //    "AWS #{@aki}",
  //    Base64.encode64(
  //      OpenSSL::HMAC.digest(
  //        OpenSSL::Digest.new('SHA1'),
  //        @sak,
  //        string_to_sign(meth, uri, headers)
  //      )
  //    ).strip
  //  ].join(':')
  //unsigned char* digest;
  //digest = HMAC(EVP_sha1(), key, strlen(key), (unsigned char*)data, strlen(data), NULL, NULL);

  char *string = string_to_sign(meth, uri, headers);

  printf("string to sign: >%s<\n", string);

  unsigned char *d =
    HMAC(
      EVP_sha1(),
      c->sak, strlen(c->sak),
      (unsigned char *)string, strlen(string),
      NULL, NULL);

  char *auth = flu_sprintf("AWS %s:%s", c->aki, flu64_encode((char *)d, -1));

  flu_list_set(headers, "authorization", auth);

  printf("%s // req headers:\n", uri);
  for (flu_node *n = headers->first; n; n = n->next)
  {
    printf("  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  }
}

/*
def sign(meth, uri, headers, body)

  headers['date'] ||= Time.now.rfc822

  if body

    headers['content-type'] =
      if body.match(/^<\?xml/)
        #'multipart/form-data'
        'application/xml'
      else
        'text/plain'
      end
    #headers['content-type'] = 'text/plain' unless body.match(/^<\?xml/)

    headers['content-md5'] =
      Base64.encode64(Digest::MD5.digest(body)).strip
  end

  headers['authorization'] =
    [
      "AWS #{@aki}",
      Base64.encode64(
        OpenSSL::HMAC.digest(
          OpenSSL::Digest.new('SHA1'),
          @sak,
          string_to_sign(meth, uri, headers)
        )
      ).strip
    ].join(':')
end

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
*/

/*
#include <stdio.h>
#include <string.h>
#include <openssl/hmac.h>

int main()
{
    // The key to hash
    char key[] = "012345678";

    // The data that we're going to hash using HMAC
    char data[] = "hello world";

    unsigned char* digest;

    // Using sha1 hash engine here.
    // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
    digest = HMAC(EVP_sha1(), key, strlen(key), (unsigned char*)data, strlen(data), NULL, NULL);

    // Be careful of the length of string with the choosen hash engine. SHA1 produces a 20-byte hash value which rendered as 40 characters.
    // Change the length accordingly with your choosen hash engine
    char mdString[20];
    for(int i = 0; i < 20; i++)
         sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    printf("HMAC digest: %s\n", mdString);

    return 0;
}
*/

flu_list *fcla3_list_buckets(fcla3_context *c)
{
  flu_list *headers = flu_list_malloc();

  char *uri = flu_sprintf("https://%s.amazonaws.com/", c->endpoint);

  sign(c, "GET", uri, headers, NULL, 0);

  fcla_response *res = fcla_get_h(uri, headers);

  flu_putf(fcla_response_to_s(res));

  flu_list_free(headers);

  return flu_list_malloc();
}

