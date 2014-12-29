
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
      flu_list_sets(headers, "Range", "bytes=0-9");

      fcla_sig4_sign(
        c,
        "s3", "us-east-1",
        'g', "examplebucket.s3.amazonaws.com", "/test.txt", "",
        headers,
        "", 0); // empty body

      expect(flu_list_get(headers, "x-aws-content-sha256") === ""
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

      expect(flu_list_get(headers, "Authorization") === ""
        "AWS4-HMAC-SHA256 "
        "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
    }
  }
}

