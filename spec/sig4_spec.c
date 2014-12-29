
//
// specifying bocla_sig4
//
// Sun Dec 28 13:51:26 JST 2014
//

//#include <string.h>

//#include "flutil.h"
#include "bocla_sig4.h"


describe "sig4:"
{
  before each
  {
    fcla_sig4_context *c = fcla_sig4_context_init("../.aws");
    c->service = rdz_strdup("s3");
    c->region = rdz_strdup("douchebagistan");

    flu_dict *headers = NULL;
  }
  after each
  {
    fcla_sig4_context_free(c);
    flu_list_free_all(headers);
  }

  //void fcla_sig4_sign(
  //  fcla_sig4_context *c,
  //  char meth, char *host, char *path, char *query,
  //  flu_dict *headers,
  //  char *body, ssize_t bodyl);

  describe "fcla_sig4_sign()"
  {
    it "signs a GET request"
    {
      headers = flu_list_malloc();

      fcla_sig4_sign(
        c,
        'g', "examplebucket.s3.amazonaws.com", "/test.txt", "",
        headers,
        NULL, -1);

      expect(flu_list_get(headers, "Authorization") === ""
        "AWS4-HMAC-SHA256 "
        "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
    }
  }
}

