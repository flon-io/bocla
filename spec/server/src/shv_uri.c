
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
#include <string.h>

#include "flutil.h"
#include "aabro.h"
#include "shv_protected.h"

//#include "gajeta.h"


abr_parser *uri_parser = NULL;


static char *shv_unescape(char *s)
{
  size_t l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));

  for (size_t i = 0, j = 0; i < l; ++j)
  {
    if (s[i] != '%') { r[j] = s[i++]; continue; }

    char *code = strndup(s + i + 1, 2);
    char c = strtol(code, NULL, 16);
    free(code);
    i = i + 3;
    r[j] = c;
  }

  return r;
}

static void shv_init_uri_parser()
{
  abr_parser *scheme =
    abr_n_rex("scheme", "https?");
  abr_parser *host =
    abr_n_rex("host", "[^:/]+");
  abr_parser *port =
    abr_seq(abr_string(":"), abr_n_rex("port", "[1-9][0-9]+"));

  abr_parser *path =
    abr_n_rex("path", "[^\\?#]+");
  abr_parser *quentry =
    abr_n_seq("quentry",
      abr_n_rex("key", "[^=&#]+"),
      abr_seq(abr_string("="), abr_n_rex("val", "[^&#]+")), abr_q("?"),
      NULL);
  abr_parser *query =
    abr_n_seq("query",
      quentry,
      abr_seq(abr_string("&"), quentry), abr_q("*"),
      NULL);
  abr_parser *fragment =
    abr_n_rex("fragment", ".+");

  abr_parser *shp =
    abr_seq(
      scheme,
      abr_string("://"),
      host,
      port, abr_q("?"),
      NULL);

  uri_parser =
    abr_seq(
      shp, abr_q("?"),
      path,
      abr_seq(abr_string("?"), query), abr_q("?"),
      abr_seq(abr_string("#"), fragment), abr_q("?"),
      NULL);
}

flu_dict *shv_parse_uri(char *uri)
{
  if (uri_parser == NULL) shv_init_uri_parser();

  abr_tree *r = abr_parse(uri, 0, uri_parser);
  //abr_tree *r = abr_parse_f(uri, 0, uri_parser, ABR_F_ALL);
  abr_tree *t = NULL;

  //printf("uri >%s<\n", uri);
  //puts(abr_tree_to_string_with_leaves(uri, r));

  flu_dict *d = flu_list_malloc();

  t = abr_tree_lookup(r, "scheme");
  if (t != NULL) flu_list_set(d, "_scheme", abr_tree_string(uri, t));
  t = abr_tree_lookup(r, "host");
  if (t != NULL) flu_list_set(d, "_host", abr_tree_string(uri, t));
  t = abr_tree_lookup(r, "port");
  if (t != NULL) flu_list_set(d, "_port", abr_tree_string(uri, t));

  t = abr_tree_lookup(r, "path");
  flu_list_set(d, "_path", abr_tree_string(uri, t));

  flu_list *l = abr_tree_list_named(r, "quentry");
  for (flu_node *n = l->first; n != NULL; n = n->next)
  {
    t = abr_tree_lookup((abr_tree *)n->item, "key");
    char *k = abr_tree_string(uri, t);

    t = abr_tree_lookup((abr_tree *)n->item, "val");
    char *v = abr_tree_string(uri, t);
    char *vv = shv_unescape(v);

    flu_list_set(d, k, vv);

    free(k); // since flu_list_set() copies it
    free(v);
  }
  flu_list_free(l);

  t = abr_tree_lookup(r, "fragment");
  if (t != NULL) flu_list_set(d, "_fragment", abr_tree_string(uri, t));

  abr_tree_free(r);

  return d;
}

flu_dict *shv_parse_host_and_path(char *host, char *path)
{
  if (host == NULL) return shv_parse_uri(path);

  char *s = flu_sprintf("%s%s", host, path);
  flu_dict *d = shv_parse_uri(s);
  free(s);

  return d;
}

