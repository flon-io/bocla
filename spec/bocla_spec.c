
//
// specifying bocla
//
// Fri Sep  5 05:37:45 JST 2014
//

#include "bocla.h"

#include "../spec/server/server.h"


context "bocla"
{
  before all
  {
    server_start();
  }
  after all
  {
    server_stop();
  }

  describe "fcla_get()"
  {
    it "gets"
    {
      fcla_response *res = fcla_get("http://127.0.0.1:4567");

      ensure(res->status_code == 200);

      fcla_response_free(res);
    }
  }
}

