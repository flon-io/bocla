
//
// specifying bocla
//
// Fri Sep  5 05:37:45 JST 2014
//

#include "bocla.h"

#include "../spec/server/server.h"


describe "bocla"
{
  before all
  {
    server_start();
  }
  after all
  {
    server_stop();
  }

  describe "fcla_ in general"
  {
    it "places the error message in res->body when it goes wrong"
    {
      fcla_response *res = fcla_get("http://www.example.com:4567");

      ensure(res->status_code == -1);
      ensure(res->body ^== "Failed to connect to ");

      fcla_response_free(res);
    }
  }

  describe "fcla_get()"
  {
    it "gets 200"
    {
      fcla_response *res = fcla_get("http://127.0.0.1:4567");

      ensure(res->status_code == 200);
      ensure(res->body === "**hello world**\n");
      ensure(flu_list_get(res->headers, "Content-Length") === "16");

      fcla_response_free(res);
    }

    it "gets 404"
    {
      fcla_response *res = fcla_get("http://127.0.0.1:4567/nada");

      ensure(res->status_code == 404);
      ensure(res->body ~== "Sinatra doesn&rsquo;t know this ditty\\.");

      fcla_response_free(res);
    }
  }
}

