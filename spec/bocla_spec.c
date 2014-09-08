
//
// specifying bocla
//
// Fri Sep  5 05:37:45 JST 2014
//

#include "bocla.h"

#include "../spec/server.h"


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
      ensure(0 == 1);
    }
  }
}

