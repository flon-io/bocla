
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

    char *credentials = flu_readall("../.aws");

    char *a = strstr(credentials, ":") + 1; if (*a == ' ') ++a;
    char *b = strchr(a, '\n');
    c->aki = rdz_strndup(a, b - a - 1);

    a = strstr(b, ":") + 1; if (*a == ' ') ++a;
    b = strchr(a, '\n');
    c->sak = rdz_strndup(a, b - a - 1);

    free(credentials); credentials = NULL;
  }
  after each
  {
    fcla3_context_free(c);
  }

  describe "fcla3_list_buckets()"
  {
    it "lists S3 buckets"
    {
      flu_list *l = fcla3_list_buckets(c);

      expect(c->last_response->status_code i== 200);

      int success = 0;

      for (flu_node *n = l->first; n; n = n->next)
      {
        //printf("* >%s<\n", (char *)n->item);
        if (strcmp((char *)n->item, "flon.io") == 0) success = 1;
      }

      expect(success i== 1);

      flu_list_free_all(l);
    }
  }

  describe "fcla3_read()"
  {
    it "reads a file from S3 and returns it as a char*"
    {
      c->bucket = rdz_strdup("flon-io");

      char *s = fcla3_read(c, "test0.txt");

      expect(c->last_response != NULL);
      expect(c->last_response->status_code i== 200);
      expect(s === "flon is a kind of interpreter");
    }

    it "accepts a bucket name as prefix>"
    {
      c->bucket = rdz_strdup("flon.io");

      char *s = fcla3_read(c, "flon-io>test0.txt");

      expect(c->last_response != NULL);
      expect(c->last_response->status_code i== 200);
      expect(s === "flon is a kind of interpreter");
    }
  }

  describe "fcla3_download()"
  {
    it "reads a file from S3 and writes it to file locally"
    it "accepts a bucket name as prefix>"
  }
}

