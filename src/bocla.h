
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

// bocla.h

#ifndef FLON_BOCLA_H
#define FLON_BOCLA_H

#include "flutil.h"


typedef struct fcla_response {
  short status_code;
  flu_list *headers;
  char *body;
} fcla_response;

char *fcla_response_to_s(fcla_response *r);

void fcla_response_free(fcla_response *r);

/* Used behind the scenes by the _get/_head/_delete[_x|_d] methods (macros).
 */
fcla_response *fcla_ghd(char meth, char hstyle, char *uri, ...);

//
// fcla_get(uri, ...);

#define fcla_get(...) fcla_ghd('g', 0, __VA_ARGS__)
#define fcla_head(...) fcla_ghd('h', 0, __VA_ARGS__)
#define fcla_delete(...) fcla_ghd('d', 0, __VA_ARGS__)

//
// fcla_get_h(uri, ..., flu_ldict *headers);

#define fcla_get_h(...) fcla_ghd('g', 'h', __VA_ARGS__)
#define fcla_head_h(...) fcla_ghd('h', 'h', __VA_ARGS__)
#define fcla_delete_h(...) fcla_ghd('d', 'h', __VA_ARGS__)

//
// fcla_get_d(uri, ..., k0, v0, ..., NULL);

#define fcla_get_d(...) fcla_ghd('g', 'd', __VA_ARGS__)
#define fcla_head_d(...) fcla_ghd('h', 'd', __VA_ARGS__)
#define fcla_delete_d(...) fcla_ghd('d', 'd', __VA_ARGS__)

//
// fcla_get_hf(uri, ..., flu_ldict *headers, path, ...);

// get and write into a file
//
fcla_response *fcla_get_hf(char *uri, ...);

/* Like fcla_ghd() but for post and put.
 */
fcla_response *fcla_popu(char meth, char hstyle, char bstyle, char *uri, ...);

//
// fcla_post(uri, ..., flu_dict *headers, body, ...);
// fcla_post_f(uri, ..., flu_dict *headers, path, ...);

#define fcla_post(uri, ...) fcla_popu('p', 'h', 's', uri, __VA_ARGS__)
#define fcla_put(uri, ...) fcla_popu('u', 'h', 's', uri, __VA_ARGS__)
#define fcla_post_f(uri, ...) fcla_popu('p', 'h', 'f', uri, __VA_ARGS__)
#define fcla_put_f(uri, ...) fcla_popu('u', 'h', 'f', uri, __VA_ARGS__)

//
// fcla_post_h(uri, ..., body, ..., flu_dict *headers);
// fcla_post_fh(uri, ..., path, ..., flu_dict *headers);

#define fcla_post_h(uri, ...) fcla_popu('p', 'H', 's', uri, __VA_ARGS__)
#define fcla_put_h(uri, ...) fcla_popu('u', 'H', 's', uri, __VA_ARGS__)
#define fcla_post_fh(uri, ...) fcla_popu('p', 'H', 'f', uri, __VA_ARGS__)
#define fcla_put_fh(uri, ...) fcla_popu('u', 'H', 'f', uri, __VA_ARGS__)

//
// fcla_post_d(uri, ..., body, ..., k0, v0, ..., NULL);
// fcla_post_fd(uri, ..., path, ..., k0, v0, ..., NULL);

#define fcla_post_d(uri, ...) fcla_popu('p', 'd', 's', uri, __VA_ARGS__)
#define fcla_put_d(uri, ...) fcla_popu('u', 'd', 's', uri, __VA_ARGS__)
#define fcla_post_fd(uri, ...) fcla_popu('p', 'd', 'f', uri, __VA_ARGS__)
#define fcla_put_fd(uri, ...) fcla_popu('u', 'd', 'f', uri, __VA_ARGS__)

/* The method behind all the requests.
 * Exposed here since it's used by bocla3.
 */
fcla_response *fcla_do_request(
  char meth, char *uri,
  flu_list *headers,
  char *dpath,
  char *sbody, FILE *fbody);


//
// helpers

flu_list *fcla_extract_headers(char *head);

#endif // FLON_BOCLA_H

