#include <catch2/catch.hpp>

#include <uvw.h>
using namespace uvw;

#include "common.h"


TEST_CASE("Processing...", "[Processing]")
{
  // ensure ws is empty
  REQUIRE( uvw::ws::procs().size() == 0 );
  REQUIRE( uvw::ws::vars().size() == 0 );
  REQUIRE( uvw::ws::links().size() == 0 );
  REQUIRE( uvw::ws::workspaces().size() == 0 );

  // proc registration
  uvw::ws::reg_proc("PreAdd", ([](){return new PreAdd();}));
  uvw::ws::reg_proc("Multiply", ([](){return new Multiply();}));

  MyWorkpace mws;
  auto* add = mws.preadd_proc();
  auto* mult = mws.mult_proc();
  uvw::duo add_a(add,"a");
  uvw::duo add_b(add,"b");
  uvw::duo add_c(add,"c");
  uvw::duo mult_x(mult,"x");
  uvw::duo mult_y(mult,"y");
  uvw::duo mult_z(mult,"z");

  // (2 + 3) * 7 = 35
  add->ref<double>("a") = 2;
  ws::ref<double>(add_b) = 3;
  mult->y_.set(7);

  mws.process(true);
  REQUIRE( ws::ref<double>(mult_z) == 35 );

  // although 'a' is set to 6, 'add' won't be pre-processed this time
  add->a_.set(6);
  mult->y_.set(4);

  // run without pre-process set to true (defaults to false)
  // (2 + 3) * 4 = 20
  uvw::var::data_pull = true;
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
  BENCHMARK("Preprocess OFF & Data-pull ON")
  {
    mws.process();
  };
#else
  mws.process();
#endif
  REQUIRE( mult->z_() == 20 );

  uvw::var::data_pull = false;
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
  BENCHMARK("Preprocess OFF & Data-pull OFF")
  {
    mws.process();
  };
#else
  mws.process();
#endif
  REQUIRE( mult->z_() == 20 );

  // run with pre-process set to true
  // (6 + 3) * 4 = 36
  uvw::var::data_pull = true;
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
  BENCHMARK("Preprocess ON & Data-pull ON")
  {
    mws.process(true);
  };
#else
  mws.process(true);
#endif
  REQUIRE( mult->z_() == 36 );

  uvw::var::data_pull = false;
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
  BENCHMARK("Preprocess ON & Data-pull OFF")
  {
    mws.process(true);
  };
#else
  mws.process(true);
#endif
  REQUIRE( mult->z_() == 36 );

  // json serialization
  auto json_stream = mws.to_json();

  // clear workspace
  mws.clear();
  REQUIRE( uvw::ws::procs().size() == 0 );
  REQUIRE( uvw::ws::links().size() == 0 );
  REQUIRE( uvw::ws::vars().size() == 0 );
  REQUIRE( uvw::ws::workspaces().size() == 1 );

  // deserialization
  mws.from_json(json_stream);

  // verify if json input & output matches
  REQUIRE( json_stream.dump().compare(mws.to_json().dump()) == 0 );

  add = mws.preadd_proc();
  mult = mws.mult_proc();
  add->a_.set(8);
  add->b_.set(3);
  mult->y_.set(2);

  // (8 + 3) * 2 = 22
  mws.process(true);
  REQUIRE( mult->ref<double>("z") == 22 );
}