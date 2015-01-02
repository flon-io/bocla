
//
// specifying bocla s3
//
// Fri Jan  2 05:48:31 JST 2015
//

//#include <string.h>

//#include "flutil.h"
#include "bocla_s3.h"


describe "bocla s3:"
{
  before each
  {
    fcla_sig4_session *s = fcla_sig4_session_init("../.aws", "s3", "us-east-1");
  }
  after each
  {
    fcla_sig4_session_free(s);
  }

  describe "fcla_s3_list_buckets()"
  {
    it "lists S3 buckets"
    {
      flu_list *l = fcla_s3_list_buckets(s);

      expect(s->last_response->status_code i== 200);

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
    it "accepts a bucket name as prefix>"
  }
//  describe "fcla3_read()"
//  {
//    it "reads a file from S3 and returns it as a char*"
//    {
//      c->bucket = rdz_strdup("flon-io");
//
//      char *s = fcla3_read(c, "test0.txt");
//
//      expect(c->last_response != NULL);
//      expect(c->last_response->status_code i== 200);
//      expect(s ===f "flon is a kind of interpreter");
//    }
//
//    it "accepts a bucket name as prefix>"
//    {
//      c->bucket = rdz_strdup("flon.io");
//
//      char *s = fcla3_read(c, "flon-io>test0.txt");
//
//      expect(c->last_response != NULL);
//      expect(c->last_response->status_code i== 200);
//      expect(s ===f "flon is a kind of interpreter");
//    }
//  }

  describe "fcla_s3_fetch()"
  {
    it "reads a file from S3 and writes it to file locally"
    it "accepts a bucket name as prefix>"
  }

  describe "fcla_s3_list()"
  {
    it "lists the files in the bucket"
  }
}

