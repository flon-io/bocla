
//
// specifying bocla_sig4
//
// Sun Dec 28 13:51:26 JST 2014
//

//#include <string.h>

#include "flutim.h"
#include "bocla_sig4.h"


describe "sig4:"
{
  before each
  {
    fcla_sig4_session *s =
      fcla_sig4_session_init("../spec/.aws_sig4", "s3", "us-east-1");

    flu_dict *headers = NULL;
  }
  after each
  {
    fcla_sig4_session_free(s);
    flu_list_free_all(headers);
  }

  describe "fcla_sig4_sign()"
  {
    it "signs a GET request"
    {
      headers = flu_list_malloc();

      flu_list_set(headers, "_date", flu_parse_tstamp("20130524.000000", 1));
        // force a date on the request, spec-only

      flu_list_sets(headers, "Range", "bytes=0-9");

      fcla_sig4_sign(
        s,
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

