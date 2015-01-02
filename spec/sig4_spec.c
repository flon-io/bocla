
//
// specifying bocla_sig4
//
// Sun Dec 28 13:51:26 JST 2014
//

//#include <string.h>

#include "flutil.h"
#include "flutim.h"
#include "bocla_sig4.h"


describe "sig4:"
{
  char *to_hex(unsigned char *s)
  {
    char *r = calloc(65, sizeof(char));
    for (size_t i = 0; i < 32; ++i) sprintf(r + 2 * i, "%02x", s[i]);

    return r;
  }

  char *to_int_list(unsigned char *s)
  {
    flu_sbuffer *b = flu_sbuffer_malloc();
    for (size_t i = 0; i < 32; ++i) flu_sbprintf(b, "%i ", s[i]);

    return flu_sbuffer_to_string(b);
  }

  describe "the signing key"
  {
//key = 'wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY'
//dateStamp = '20120215'
//regionName = 'us-east-1'
//serviceName = 'iam'

//kSecret  = '41575334774a616c725855746e46454d492f4b374d44454e472b62507852666943594558414d504c454b4559'
//kDate    = '969fbb94feb542b71ede6f87fe4d5fa29c789342b0f407474670f0c2489e0a0d'
//kRegion  = '69daa0209cd9c5ff5c8ced464a696fd4252e981430b10e3d3fd8e2f197d7a70c'
//kService = 'f72cfd46f26bc4643f06a11eabb6c0ba18780c19a8da0c31ace671265e3c87fa'
//kSigning = 'f4780e2d9f65fa895f9c67b32ce1baf0b0d8a43505a000a1a9e090d414db404d'

    it "is generated correctly (1)"
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

    it "is generated correctly (2)"
    {
      // http://docs.aws.amazon.com/general/latest/gr/sigv4-calculate-signature.html
      fcla_sig4_session *ses = calloc(1, sizeof(fcla_sig4_session));
      ses->provider = "aws";
      ses->provider_u = "AWS";
      ses->sak = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
      ses->region = "us-east-1";
      ses->service = "iam";

      fcla_sig4_request *req = calloc(1, sizeof(fcla_sig4_request));
      req->date = "20110909";

      unsigned char *k = (unsigned char []){
        152, 241, 216, 137, 254, 196, 244, 66, 26, 220, 82, 43, 171, 12, 225,
        248, 46, 105, 41, 194, 98, 237, 21, 229, 169, 76, 144, 239, 209, 227,
        176, 231 };
      char *hk = to_hex(k);
        //98f1d889fec4f4421adc522bab0ce1f82e6929c262ed15e5a94c90efd1e3b0e7

      char *key = fcla_sig4_signing_key(ses, req);

      expect(key ===F hk);

      free(ses);
      free(req);
    }
  }

  describe "the string to sign"
  {
    it "is generated correctly"
    {
      fcla_sig4_session *ses = calloc(1, sizeof(fcla_sig4_session));
      ses->provider = "aws";
      ses->provider_u = "AWS";
      ses->header = "amz";
      ses->sak = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
      ses->region = "us-east-1";
      ses->service = "s3";

      fcla_sig4_request *req = calloc(1, sizeof(fcla_sig4_request));
      req->date = "20130524";
      req->meth = 'g';
      req->path = "/test.txt";
      req->query = "";
      //
      req->headers = flu_list_malloc();
      flu_list_sets(
        req->headers, "Host", "examplebucket.s3.amazonaws.com");
      flu_list_sets(
        req->headers, "Range", "bytes=0-9");
      flu_list_sets(
        req->headers, "x-amz-date", "20130524T000000Z");
      flu_list_sets(
        req->headers, "x-amz-content-sha256",
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
      //
      req->signed_headers = "host;range;x-amz-content-sha256;x-amz-date";

      expect(fcla_sig4_string_to_sign(ses, req) ===f ""
        "AWS4-HMAC-256\n"
        "20130524T000000Z\n"
        "20130524/us-east-1/s3/aws4_request\n"
        "7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972");

      free(ses);
      flu_list_free_all(req->headers);
      free(req);
    }
  }

  describe "fcla_sig4_sign()"
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

      expect(flu_list_get(headers, "x-amz-date") === ""
        "20130524T000000Z");

      expect(flu_list_get(headers, "x-amz-content-sha256") === ""
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

      expect(flu_list_get(headers, "Authorization") === ""
        "AWS4-HMAC-SHA256 "
        "Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,"
        "SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,"
        "Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
    }
  }

//  it "computes the expected hash"
//  {
//    char *key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
//    char *akey = flu_sprintf("AWS4%s", key);
//    char *date = NULL;
//    unsigned char *dk = NULL;
//
//    puts("");
//
//    date = "20120215";
//    dk = hmac_sha256(akey, -1, date);
//
//    printf("key, date: %s %s\n", key, date);
//    printf("date_key:  %s\n", to_hex(dk));
//
//    puts("");
//
//    date = "20110909";
//    dk = hmac_sha256(akey, -1, date);
//
//    printf("key, date: %s %s\n", key, date);
//    printf("date_key:  %s\n", to_hex(dk));
//
//    puts("");
//
//    date = "20130524";
//    dk = hmac_sha256(akey, -1, date);
//
//    printf("key, date: %s %s\n", key, date);
//    printf("date_key:  %s\n", to_hex(dk));
//  }
}

