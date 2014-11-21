
//
// specifying bocla3
//
// Fri Nov 21 13:54:19 JST 2014
//

#include <string.h>

#include "flutil.h"
#include "bocla3.h"


describe "bocla3"
{
  before each
  {
    fcla3_context *c = calloc(1, sizeof(fcla3_context));
    c->endpoint = rdz_strdup("s3");

    char *s = flu_readall("../.aws");

    char *a = strstr(s, ":") + 1; if (*a == ' ') ++a;
    char *b = strchr(a, '\n');
    c->aki = rdz_strndup(a, b - a - 1);

    a = strstr(b, ":") + 1; if (*a == ' ') ++a;
    b = strchr(a, '\n');
    c->sak = rdz_strndup(a, b - a - 1);
  }
  after each
  {
    fcla3_context_free(c);
  }

  it "lists buckets"
  {
    flu_list *l = fcla3_list_buckets(c);

    for (flu_node *n = l->first; n; n = n->next)
    {
      printf("* %s\n", (char *)n->item);
    }
  }
}

