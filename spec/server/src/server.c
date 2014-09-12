
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

#include <time.h>
#include <string.h>

#include "shervin.h"


/*
 * Respond with 200 and the time.
 */
void htime_handler(shv_request *req, shv_response *res, void *params)
{
  time_t tt; time(&tt);
  struct tm *tm; tm = gmtime(&tt);
  char *dt = asctime(tm);

  res->status_code = 200;
  res->content_type = "text/plain; charset=utf-8";
  res->body = strdup(dt);
}

int main()
{
  shv_route **routes = (shv_route *[]){
    &(shv_route){ shv_any_guard, htime_handler, NULL },
    NULL // optional since we have a "shv_any_guard" above
  };

  shv_serve(4001, routes);
}

