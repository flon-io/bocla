
//
// specifying bocla
//
// Fri Sep  5 05:37:45 JST 2014
//

#include "bocla.h"

#include "../spec/server/servman.h"


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

    ensure(
      strcmp(res->body, "connect() timed out!") == 0 ||
      strncmp(res->body, "Failed to connect to ", 21) == 0);
  }

  describe "fcla_get()"
  {
    it "gets 200"
    {
      res = fcla_get("http://127.0.0.1:4567/hello");

      ensure(res->status_code == 200);
      ensure(res->body === "**hello world**\n");
      ensure(flu_list_get(res->headers, "content-length") === "16");
    }

    it "gets 404"
    {
      res = fcla_get("http://127.0.0.1:4567/nada");

      ensure(res->status_code == 404);
      ensure(res->body === "");
    }

    it "gets /mirror"
    {
      res = fcla_get("http://127.0.0.1:4567/mirror");

      ensure(res->status_code == 200);

      ensure(res->body != NULL);

      //printf("\n%s\n", res->body);
      flu_list *d = fcla_extract_headers(res->body);

      ensure(flu_list_get(d, "PATH_INFO") === "/mirror");
      ensure(flu_list_get(d, "REQUEST_METHOD") === "GET");

      flu_list_and_items_free(d, free);
    }
  }

  describe "fcla_get_h()"
  {
    it "gets 200"
    {
      flu_list *hs = flu_d("user-agent", "flon bocla 0.x", NULL);
      //
      res = fcla_get_h("http://127.0.0.1:4567/mirror", hs);

      ensure(res != NULL);
      ensure(res->body != NULL);

      //printf("\n%s\n", res->body);
      flu_list *d = fcla_extract_headers(res->body);

      ensure(flu_list_get(d, "HTTP_USER_AGENT") === "flon bocla 0.x");

      flu_list_and_items_free(d, free);

      flu_list_free(hs);
        // the values are not on the heap, no need to use _and_items_free()
    }
  }

  describe "fcla_head()"
  {
    it "heads 200"
    {
      res = fcla_head("http://127.0.0.1:4567/hello");

      ensure(res->status_code == 200);
      ensure(res->body === "");
      ensure(flu_list_get(res->headers, "content-length") === "16");
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

