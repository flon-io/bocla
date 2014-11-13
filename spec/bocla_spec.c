
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
    //ensure(
    //  res->body === "connect() timed out!" ||
    //  res->body ^== "Failed to connect to ");
      // rodzo can't deal with that...
  }

  it "set Authorization: Basic u:p when passed '_u' (and '_p')"
  {
    res = fcla_get_d(
      "http://127.0.0.1:4567/mirror",
      "_u", "john", "_p", "wyvern", NULL);

    ensure(res->status_code == 200);

    ensure(res->body != NULL);

    //printf("\n%s\n", res->body);
    flu_list *d = fcla_extract_headers(res->body);

    ensure(flu_list_get(d, "path") === "/mirror");
    ensure(flu_list_get(d, "method") === "GET");
    ensure(flu_list_get(d, "authorization") === "Basic am9objp3eXZlcm4=");
    ensure(flu_list_get(d, "_u") == NULL);
    ensure(flu_list_get(d, "_p") == NULL);

    flu_list_and_items_free(d, free);
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

      ensure(flu_list_get(d, "path") === "/mirror");
      ensure(flu_list_get(d, "method") === "GET");

      flu_list_and_items_free(d, free);
    }

    it "composes its uri"
    {
      res = fcla_get("http://127.0.0.1:4567/%so", "hell");

      ensure(res->status_code == 200);
      ensure(res->body === "**hello world**\n");
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

      ensure(flu_list_get(d, "user-agent") === "flon bocla 0.x");

      flu_list_and_items_free(d, free);

      flu_list_free(hs);
        // the values are not on the heap, no need to use _and_items_free()
    }

    it "composes its uri"
    {
      flu_list *hs = flu_d("user-agent", "flon bocla 0.x", NULL);
      //
      res = fcla_get_h("http://127.0.0.1:4567/%s", "mirror", hs);

      ensure(res != NULL);
      ensure(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(strstr(res->body, "user-agent: flon bocla 0.x") != NULL);

      flu_list_free(hs);
        // the values are not on the heap, no need to use _and_items_free()
    }
  }

  describe "fcla_get_d()"
  {
    it "composes uri and then collect header entries"
    {
      res = fcla_get_d(
        "http://127.0.0.1:4567/%s", "mirror",
        "user-agent", "flon bocla 0.x", NULL);

      ensure(res != NULL);
      ensure(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(strstr(res->body, "user-agent: flon bocla 0.x") != NULL);
    }

    it "composes its header entries"
    {
      res = fcla_get_d(
        "http://127.0.0.1:4567/%s", "mirror",
        "user-agent", "flon bocla 7.%i", 9,
        "x-bocla-auth", "na%s", "da",
        NULL);

      ensure(res != NULL);
      ensure(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(strstr(res->body, "\r\nuser-agent: flon bocla 7.9\r\n") != NULL);
      expect(strstr(res->body, "\r\nx-bocla-auth: nada\r\n") != NULL);
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
      ensure(flu_list_get(res->headers, "content-length") === "8");
    }
  }

  describe "fcla_post()"
  {
    it "posts"
    {
      res = fcla_post("http://127.0.0.1:4567/mirror", NULL, "hello\nworld.");

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res->status_code == 200);
      ensure(res->body $== "\r\n\r\nhello\nworld.");
    }

    it "posts to /null"
    {
      res = fcla_post("http://127.0.0.1:4567/null", NULL, "nada.");

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res->status_code == 200);
      ensure(res->body === "ok.");
    }
  }

  describe "fcla_post_f()"
  {
    it "posts"
    {
      res = fcla_post_f("http://127.0.0.1:4567/mirror", NULL, __FILE__);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res->status_code == 200);
      ensure(strstr(res->body, "spec/bocla_spec.c") != NULL);
    }
  }

  describe "fcla_post_h()"
  {
    it "posts"
    {
      flu_dict *h = flu_d("user-agent", "post-h", NULL);

      res = fcla_post_h(
        "http://127.0.0.1:4567/mir%sr", "ro",
        "hello%sworld.", "\n",
        h);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      expect(res != NULL);
      expect(res->status_code == 200);
      expect(strstr(res->body, "\r\nuser-agent: post-h\r\n") != NULL);

      flu_list_free(h);
    }
  }

  describe "fcla_post_fh()"
  {
    it "posts"
    {
      flu_dict *h = flu_d("user-agent", "post-fh", NULL);

      res = fcla_post_fh(
        "http://127.0.0.1:4567/mir%sr", "ro",
        __FILE__,
        h);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      expect(res != NULL);
      expect(res->status_code == 200);
      expect(strstr(res->body, "\r\nuser-agent: post-fh\r\n") != NULL);
      expect(strstr(res->body, "\n// Fri Sep  5 05:37:45 JST 2014\n") != NULL);

      flu_list_free(h);
    }
  }

  describe "fcla_post_d()"
  {
    it "posts"
    {
      res = fcla_post_d(
        "http://127.0.0.1:4567/mir%sr", "ro",
        "hello%sworld.", "\n",
        "user-%s", "agent", "post-%s", "d", NULL);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res != NULL);
      ensure(res->status_code == 200);
      ensure(strstr(res->body, "\r\nuser-agent: post-d\r\n") != NULL);
    }
  }

  describe "fcla_post_fd()"
  {
    it "posts"
    {
      res = fcla_post_fd(
        "http://127.0.0.1:4567/mir%sr", "ro",
        __FILE__,
        "user-%s", "agent", "post-%s", "d", NULL);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res != NULL);
      ensure(res->status_code == 200);
      ensure(strstr(res->body, "\r\nuser-agent: post-d\r\n") != NULL);
      expect(strstr(res->body, "\n// Fri Sep  5 05:37:45 JST 2014\n") != NULL);
    }
  }

  describe "fcla_put()"
  {
    it "puts"
    {
      res = fcla_put("http://127.0.0.1:4567/mirror", NULL, "put put put");

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res->status_code == 200);
      ensure(res->body ^== "PUT /mirror HTTP/1.1\r\n");
      ensure(res->body $== "\r\n\r\nput put put");
    }
  }

  describe "fcla_put_f()"
  {
    it "puts"
    {
      res = fcla_put_f("http://127.0.0.1:4567/mirror", NULL, __FILE__);

      //printf("\n>>>\n%s\n<<<\n", res->body);

      ensure(res->status_code == 200);
      ensure(strstr(res->body, "spec/bocla_spec.c") != NULL);
    }
  }
}

