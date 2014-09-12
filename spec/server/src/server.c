
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
#include <unistd.h>

#include "gajeta.h"
#include "shervin.h"


void grey_logger(char level, const char *pref, const char *msg)
{
  char *lstr = fgaj_level_to_string(level);

  printf("[1;30m%21s %s %s[0;0m\n", lstr, pref, msg);

  fgaj_level_string_free(lstr);
}

static void hello_handler(shv_request *req, shv_response *res, void *params)
{
  res->status_code = 200;
  res->content_type = "text/plain; charset=utf-8";
  res->body = "**hello world**\n";
}

int main()
{
  fgaj_conf_get()->logger = grey_logger;

  shv_route **routes = (shv_route *[]){
    &(shv_route){ shv_any_guard, hello_handler, NULL },
    NULL // optional since we have a "shv_any_guard" above
  };

  shv_serve(4567, routes);
}

