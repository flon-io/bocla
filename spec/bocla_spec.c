
//
// specifying bocla
//
// Fri Sep  5 05:37:45 JST 2014
//

#include "bocla.h"


describe "bocla"
{
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

    expect(res->status_code == -1);

    expect(
      strcmp(res->body, "connect() timed out!") == 0 ||
      strncmp(res->body, "Failed to connect to ", 21) == 0);
    //expect(
    //  res->body === "connect() timed out!" ||
    //  res->body ^== "Failed to connect to ");
      // rodzo can't deal with that...
  }

  it "set Authorization: Basic u:p when passed '_u' (and '_p')"
  {
    res = fcla_get_d(
      "http://httpbin.org/basic-auth/john/wyvern",
      "_u", "john", "_p", "wyvern", NULL);

    expect(res->status_code i== 200);

    expect(res->body != NULL);

    //printf("\n%s\n", res->body);
    expect(res->body >=== "\"authenticated\": true,");
    expect(res->body >=== "\"user\": \"john\"");
  }

  describe "fcla_get()"
  {
    it "gets 200"
    {
      res = fcla_get("http://httpbin.org/");

      expect(res->status_code == 200);
      //puts(res->body);
      expect(res->body >=== "Testing an HTTP Library can become difficult");
      expect(flu_list_get(res->headers, "Content-Length") === "10806");
    }

    it "gets 404"
    {
      res = fcla_get("http://httpbin.org/status/404");

      expect(res->status_code == 404);
      expect(res->body === "");
    }

    it "composes its uri"
    {
      res = fcla_get("http://httpbin.org/%s", "headers");

      expect(res->status_code == 200);
      //puts(res->body);
      expect(res->body >=== "\"headers\":");
    }
  }

  describe "fcla_get_h()"
  {
    it "gets 200"
    {
      flu_list *hs = flu_d("user-agent", "flon bocla 0.x", NULL);
      //
      res = fcla_get_h("http://httpbin.org/get", hs);

      expect(res != NULL);
      expect(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(res->body >== "\"User-Agent\": \"flon bocla 0.x\"");

      flu_list_free(hs);
        // the values are not on the heap, no need to use _and_items_free()
    }

    it "composes its uri"
    {
      flu_list *hs = flu_d("user-agent", "flon bocla 0.x", NULL);
      //
      res = fcla_get_h("http://httpbin.org/%s", "get?show_env=1", hs);

      expect(res != NULL);
      expect(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(res->body >== "\"User-Agent\": \"flon bocla 0.x\"");

      flu_list_free(hs);
        // the values are not on the heap, no need to use _and_items_free()
    }
  }

  describe "fcla_get_d()"
  {
    it "composes uri and then collect header entries"
    {
      res = fcla_get_d(
        "http://httpbin.org/get%s", "?show_env=1",
        "user-agent", "flon bocla 0.x", NULL);

      expect(res != NULL);
      expect(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(res->body >== "\"User-Agent\": \"flon bocla 0.x\"");
    }

    it "composes its header entries"
    {
      res = fcla_get_d(
        "http://httpbin.org/get%s", "?show_env=1",
        "user-agent", "flon bocla 7.%i", 9,
        "x-bocla-auth", "na%s", "da",
        NULL);

      expect(res != NULL);
      expect(res->body != NULL);

      //printf("\n%s\n", res->body);
      expect(res->body >== "\"User-Agent\": \"flon bocla 7.9\"");
      expect(res->body >== "\"X-Bocla-Auth\": \"nada\"");
    }
  }

  describe "fcla_head()"
  {
    it "heads 200"
    {
      res = fcla_head("http://httpbin.org/?show_env=1");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 200);
      expect(res->body === "");
      expect(flu_list_get(res->headers, "Content-Length") === "10806");
    }
  }

  describe "fcla_delete()"
  {
    it "deletes 200"
    {
      res = fcla_delete("http://httpbin.org/delete?show_env=1");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 200);
      expect(res->body >== "\"url\": \"http://httpbin.org/delete?show_env=1\"");
    }
  }

  describe "fcla_post()"
  {
    it "posts"
    {
      res = fcla_post(
        "http://httpbin.org/post?show_env=1", NULL, "hello\nworld.");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 200);
      expect(res->body >== "\"form\": {\n    \"hello\\nworld.\"");
    }
  }

  describe "fcla_post_f()"
  {
    it "posts"
    {
      res = fcla_post_f(
        "http://httpbin.org/post?show_env=1", NULL, "../Makefile");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 100);
      expect(res->body >== "NAME=bocla");
    }
  }

  describe "fcla_post_h()"
  {
    it "posts"
    {
      flu_dict *h = flu_d("user-agent", "post-h", NULL);

      res = fcla_post_h(
        "http://httpbin.org/%s?show_env=1", "post",
        "hello%sworld.", "\n",
        h);

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res != NULL);
      expect(res->status_code i== 200);
      expect(res->body >== "\"hello\\nworld.\"");
      expect(res->body >== "\"User-Agent\": \"post-h\"");

      flu_list_free(h);
    }
  }

  describe "fcla_post_fh()"
  {
    it "posts"
    {
      flu_dict *h = flu_d("user-agent", "post-fh", NULL);

      res = fcla_post_fh(
        "http://httpbin.org/%s?show_env=1", "post",
        "../Makefile",
        h);

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res != NULL);
      expect(res->status_code i== 100);
      expect(res->body >== "NAME=bocla");
      expect(res->body >== "\"User-Agent\": \"post-fh\"");

      flu_list_free(h);
    }
  }

  describe "fcla_post_d()"
  {
    it "posts"
    {
      res = fcla_post_d(
        "http://httpbin.org/%s?show_env=1", "post",
        "hello%sworld.", "\n",
        "user-%s", "agent", "post-%s", "d", NULL);

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res != NULL);
      expect(res->status_code i== 200);
      expect(res->body >== "hello\\nworld");
      expect(res->body >== "\"User-Agent\": \"post-d\"");
    }
  }

  describe "fcla_post_fd()"
  {
    it "posts"
    {
      res = fcla_post_fd(
        "http://httpbin.org/post%s", "?show_env=1",
        "../Makefile",
        "user-%s", "agent", "post-%s", "d", NULL);

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res != NULL);
      expect(res->status_code i== 100);
      expect(res->body >== "NAME=bocla");
      expect(res->body >== "spec clean upgrade");
    }
  }

  describe "fcla_put()"
  {
    it "puts"
    {
      res = fcla_put("http://httpbin.org/put?show_env=1", NULL, "put put put");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 200);
      expect(res->body >== "http://httpbin.org/put");
      expect(res->body >== "\"put put put\"");
    }
  }

  describe "fcla_put_f()"
  {
    it "puts"
    {
      res = fcla_put_f(
        "http://httpbin.org/put?show_env=1", NULL, "../Makefile");

      //printf("\n>>>\n%s\n<<<\n", res->body);
      expect(res->status_code i== 100);
      expect(res->body >== "NAME=bocla");
    }
  }
}

