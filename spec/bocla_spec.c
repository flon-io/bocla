
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

  before each
  {
    fcla_response *res = NULL;
  }
  after each
  {
    if (res != NULL) fcla_response_free(res);
  }

  it "places the error message in res->body when it goes wrong"
  {
    res = fcla_get("http://www.example.com:4567");

    ensure(res->status_code == -1);
    ensure(res->body ^== "Failed to connect to ");
  }

  describe "fcla_get()"
  {
    it "gets 200"
    {
      res = fcla_get("http://127.0.0.1:4567");

      ensure(res->status_code == 200);
      ensure(res->body === "**hello world**\n");
      ensure(flu_list_get(res->headers, "Content-Length") === "16");
    }

    it "gets 404"
    {
      res = fcla_get("http://127.0.0.1:4567/nada");

      ensure(res->status_code == 404);
      ensure(res->body ~== "Sinatra doesn&rsquo;t know this ditty\\.");
    }
  }

  describe "fcla_head()"
  {
    it "heads 200"
    {
      res = fcla_head("http://127.0.0.1:4567");

      ensure(res->status_code == 200);
      ensure(res->body === "");
      ensure(flu_list_get(res->headers, "Content-Length") === "16");
    }
  }

  describe "fcla_delete()"
  {
    it "deletes 200"
    {
      res = fcla_delete("http://127.0.0.1:4567/d");

      ensure(res->status_code == 200);
      ensure(res->body === "deleted.");
      ensure(flu_list_get(res->headers, "Content-Length") === "8");
    }
  }
}

