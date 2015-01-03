
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

// bocla_sig4.h

#ifndef FLON_BOCLA_SIG4_H
#define FLON_BOCLA_SIG4_H

#include "bocla.h"


typedef struct {
  char *provider; // usually "aws"
  char *provider_u; // uppercase
  char *header; // "amz" for "x-amz-..."
  char *aki;
  char *sak;
  char *service;
  char *region;
  fcla_response *last_response;
} fcla_sig4_session;

typedef struct {
  char meth;
  char *host;
  char *path;
  char *query;
  char *date;
  flu_dict *headers;
  char *signed_headers;
  char *body;
  size_t bodyl;
} fcla_sig4_request;

fcla_sig4_session *fcla_sig4_session_init(
  const char *path, const char *region, const char *service);

void fcla_sig4_session_free(fcla_sig4_session *c);

void fcla_sig4_sign(
  fcla_sig4_session *s,
  char meth, char *host, char *path, char *query, flu_dict *headers,
  char *body, size_t bodyl);


// bin_to_x() helpers

char *fcla_bin_to_hex(unsigned char *data, size_t len);


// specs only

char *fcla_sig4_signing_key(fcla_sig4_session *s, fcla_sig4_request *r);
char *fcla_sig4_string_to_sign(fcla_sig4_session *s, fcla_sig4_request *r);

#endif // FLON_BOCLA_SIG4_H

