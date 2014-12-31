
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

//key = 'wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY'
//dateStamp = '20120215'
//regionName = 'us-east-1'
//serviceName = 'iam'

//kSecret  = '41575334774a616c725855746e46454d492f4b374d44454e472b62507852666943594558414d504c454b4559'
//kDate    = '969fbb94feb542b71ede6f87fe4d5fa29c789342b0f407474670f0c2489e0a0d'
//kRegion  = '69daa0209cd9c5ff5c8ced464a696fd4252e981430b10e3d3fd8e2f197d7a70c'
//kService = 'f72cfd46f26bc4643f06a11eabb6c0ba18780c19a8da0c31ace671265e3c87fa'
//kSigning = 'f4780e2d9f65fa895f9c67b32ce1baf0b0d8a43505a000a1a9e090d414db404d'

  describe "fcla_sig4_signing_key()"
  {
    it "generates the signing key"
    {
      fcla_sig4_session *ses = calloc(1, sizeof(fcla_sig4_session));
      ses->provider = "aws";
      ses->provider_u = "AWS";
      ses->sak = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
      ses->region = "us-east-1";
      ses->service = "iam";

      fcla_sig4_request *req = calloc(1, sizeof(fcla_sig4_request));
      req->date = "20120215";

      char *key = fcla_sig4_signing_key(ses, req);

      expect(key ===f ""
        "f4780e2d9f65fa895f9c67b32ce1baf0b0d8a43505a000a1a9e090d414db404d");

      free(ses);
      free(req);
    }
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

      expect(flu_list_get(headers, "x-amz-content-sha256") === ""
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

      expect(flu_list_get(headers, "Authorization") === ""
        "AWS4-HMAC-SHA256 "
        "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
    }
  }
}

