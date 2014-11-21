
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>

#include "flutil.h"
#include "bocla.h"


char *fcla_response_to_s(fcla_response *r)
{
  flu_sbuffer *sb = flu_sbuffer_malloc();

  flu_sbprintf(sb, "< --res-->\n");
  flu_sbprintf(sb, "  status_code: %i\n", r->status_code);
  for (flu_node *n = r->headers->first; n; n = n->next)
    flu_sbprintf(sb, "  * \"%s\": \"%s\"\n", n->key, (char *)n->item);
  flu_sbprintf(sb, "  body >>>\n%s\n<<<\n", r->body);
  flu_sbprintf(sb, "</--res-->\n");

  return flu_sbuffer_to_string(sb);
}

void fcla_response_free(fcla_response *r)
{
  if (r == NULL) return; // like free() does
  if (r->headers != NULL) flu_list_and_items_free(r->headers, free);
  free(r->body);
  free(r);
}

static short fcla_extract_status(char *head)
{
  return strtol(strchr(head, ' '), NULL, 10);
}

static char *fcla_crlf(char *s)
{
  char *s0 = strchr(s, '\r');
  if (s0 != NULL && s0[1] == '\n' && s0[2] != '\0') return s0 + 2;
  return NULL;
}

flu_list *fcla_extract_headers(char *head)
{
  flu_list *l = flu_list_malloc();
  char *s = head;
  while (1)
  {
    s = fcla_crlf(s);
    if (s == NULL || s[0] == '\r') break;
    char *c = strchr(s, ':');
    char *r = strchr(c + 1, '\r');
    char *k = strndup(s, c - s);
    char *v = strndup(c + 1, r - c - 1);
    flu_list_set(l, k, flu_strtrim(v));
    free(k);
    free(v);
  }
  return l;
}

fcla_response *fcla_do_request(
  char meth,
  char *uri,
  flu_list *headers,
  char *sbody,
  FILE *fbody)
{
  fcla_response *res = calloc(1, sizeof(fcla_response));
  res->status_code = -1;

  char *buffer = NULL;
  flu_sbuffer *bhead = NULL;
  flu_sbuffer *bbody = NULL;

  CURL *curl;
  struct curl_slist *cheaders = NULL;

  curl = curl_easy_init();

  if (curl == NULL) { res->body = "curl initialization failed"; goto _done; }

  buffer = calloc(512, sizeof(char));

  if (meth == 'h') curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
  //else if (meth == 'p') curl_easy_setopt(curl, CURLOPT_POST, 1);
  //else if (meth == 'u') curl_easy_setopt(curl, CURLOPT_PUT, 1);
  else if (meth == 'p') curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
  else if (meth == 'u') curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  else if (meth == 'd') curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

  curl_easy_setopt(curl, CURLOPT_URL, uri);

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

  bhead = flu_sbuffer_malloc();
  bbody = flu_sbuffer_malloc();
  //
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, bhead->stream);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, bbody->stream);

  if (meth == 'p' || meth == 'u')
  {
    if (sbody)
    {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sbody);
    }
    else // fbody
    {
      struct stat fi; fstat(fileno(fbody), &fi);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
      curl_easy_setopt(curl, CURLOPT_READDATA, fbody);
      curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fi.st_size);
    }
  }

  if (headers && flu_list_get(headers, "_u"))
  {
    char *us = flu_list_get(headers, "_u");
    char *pa = flu_list_get(headers, "_p"); if (pa == NULL) pa = "";

    curl_easy_setopt(curl, CURLOPT_USERNAME, us);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pa);
  }

  if (headers != NULL)
  {
    flu_list *tl = flu_list_dtrim(headers);
    for (flu_node *n = tl->first; n != NULL; n = n->next)
    {
      if (*n->key == '_' && (n->key[1] == 'u' || n->key[1] == 'p')) continue;
      char *s = flu_sprintf("%s: %s", n->key, (char *)n->item);
      cheaders = curl_slist_append(cheaders, s);
      free(s);
    }
    flu_list_free(tl);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, cheaders);
  }

  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  CURLcode r = curl_easy_perform(curl);

  if (r != CURLE_OK) { res->body = buffer; goto _done; }

  char *shead = flu_sbuffer_to_string(bhead); bhead = NULL;
  //printf("\n-->\n%s<--\n", shead);

  res->status_code = fcla_extract_status(shead);
  res->headers = fcla_extract_headers(shead);

  free(shead);

  res->body = flu_sbuffer_to_string(bbody); bbody = NULL;

_done:

  if (curl) curl_easy_cleanup(curl);
  if (res->status_code > -1 && buffer != NULL) free(buffer);
  if (bhead) flu_sbuffer_free(bhead);
  if (bbody) flu_sbuffer_free(bbody);
  if (cheaders) curl_slist_free_all(cheaders);

  return res;
}

fcla_response *fcla_ghd(char meth, char hstyle, char *uri, ...)
{
  va_list ap; va_start(ap, uri);

  char *u = flu_svprintf(uri, ap);

  flu_dict *h = NULL;
  if (hstyle == 'h') h = va_arg(ap, flu_dict *);
  else if (hstyle == 'd') h = flu_vsd(ap);

  va_end(ap);

  fcla_response *r = fcla_do_request(meth, u, h, NULL, NULL);

  free(u);
  if (hstyle == 'd') flu_list_free_all(h);

  return r;
}

fcla_response *fcla_popu(char meth, char hstyle, char bstyle, char *uri, ...)
{
  char *u = NULL;
  flu_dict *h = NULL;
  char *s = NULL;
  FILE *f = NULL;

  va_list ap; va_start(ap, uri);

  u = flu_svprintf(uri, ap);

  if (hstyle == 'h') h = va_arg(ap, flu_dict *);

  char *sf = va_arg(ap, char *);
  if (sf == NULL) return NULL;
  s = flu_svprintf(sf, ap);

  if (hstyle == 'H') h = va_arg(ap, flu_dict *);
  else if (hstyle == 'd') h = flu_vsd(ap);

  va_end(ap);

  if (bstyle == 'f')
  {
    f = fopen(s, "r");
    // TODO: error handling
  }

  fcla_response *r = fcla_do_request(meth, u, h, f ? NULL : s, f);

  free(u);
  free(s);
  if (hstyle == 'd') flu_list_free_all(h);
  if (f) fclose(f);

  return r;
}

