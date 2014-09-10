
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

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <curl/curl.h>

#include "bocla.h"


void fcla_response_free(fcla_response *r)
{
  if (r->headers != NULL) flu_list_free(r->headers);
  if (r->body != NULL) free(r->body);
  free(r);
}

fcla_response *fcla_request(
  char meth, char *uri, flu_list *headers, char *body)
{
  fcla_response *res = calloc(1, sizeof(fcla_response));
  res->status_code = -1;

  char *buffer = NULL;

  CURL *curl;

  curl = curl_easy_init();

  if (curl == NULL) { res->body = "curl initialization failed"; goto _done; }

  buffer = calloc(512, sizeof(char));

  curl_easy_setopt(curl, CURLOPT_URL, uri);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

  CURLcode r = curl_easy_perform(curl);

  if (r != CURLE_OK) { res->body = buffer; goto _done; }

  res->status_code = 200;
  // TODO: grab real status_code and body...

_done:

  if (curl != NULL) curl_easy_cleanup(curl);
  if (res->status_code > -1 && buffer != NULL) free(buffer);

  return res;
}

fcla_response *fcla_get(char *uri)
{
  return fcla_request('g', uri, NULL, NULL);
}

