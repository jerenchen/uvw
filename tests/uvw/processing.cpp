#include <catch2/catch.hpp>

#include <uvw.h>
using namespace uvw;


// Multply proc
struct Multiply: public Processor
{
  Var<double> x_, y_, z_;

  Multiply(): Processor() {}

  bool initialize() override
  {
    std::cout << "Adding 'add' proc with vars \'a\' \'b\' & \'c\'..." << std::endl;
    x_.properties["parameter"] = Variable::INPUT;
    y_.values["min"] = -20.25;
    y_.values["max"] = 25.5;
    return (
      reg_var<double>("y", y_) &&
      reg_var<double>("x", x_) &&
      reg_var<double>("z", z_)
    );
  }

  bool preprocess() override
  {
    std::cout << "No action taken in pre-processing ..." << std::endl;
    return true;
  }

  bool process() override
  {
    auto& x = x_();
    auto& y = y_();
    std::cout << "Processing x * y (" << x << " * " << y << ")..." << std::endl;
    z_() = x * y;
    return true;
  }
};

// Precomputed Addition Proc
struct PreAdd: public Processor
{
  double d_; // internal
  public:
  Var<double> a_, b_, c_;

  PreAdd(): Processor() {}

  bool initialize() override
  {
    std::cout << "Adding 'mult' proc with vars \'x\' \'y\' & \'z\'..." << std::endl;
    return (
      reg_var<double>("a", a_) &&
      reg_var<double>("b", b_) &&
      reg_var<double>("c", c_)
    );
  }

  bool preprocess() override
  {
    std::cout << "Pre-processing d = a + b (" <<
      a_() << " + " << b_() << ")..." << std::endl;
    d_ = a_() + b_();
    return true;
  }

  bool process() override
  {
    std::cout << "Processing c = d (" << d_ << ")..." << std::endl;
    c_() = d_;
    return true;
  }
};

// define workspace
struct MyWorkpace: public uvw::Workspace
{
  MyWorkpace(): uvw::Workspace()
  {
    // build nodes and link vars so that:
    //   z = (a + b) * y
    auto* p = new_proc("PreAdd");
    auto* q = new_proc("Multiply");

    std::cout << "Linking 'x' to 'c'..." << std::endl;
    q->get("x")->link(p->get("c"));
    // or alternatively...
    //ws::link(uvw::duo(p,"c"), uvw::duo(q,"x"));

    set_output(uvw::duo(q,"z"));
  }

  // helper to get procs
  PreAdd* preadd_proc()
  {
    return proc_ptrs_.size()?
      static_cast<PreAdd*>(proc_ptrs_[0]) : nullptr;
  };
  Multiply* mult_proc()
  {
    return proc_ptrs_.size()>1?
      static_cast<Multiply*>(proc_ptrs_[1]) : nullptr;
  };
};

TEST_CASE("Processing...", "[Processing]")
{
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
  mws.process();
  REQUIRE( mult->z_() == 20 );

  // run with pre-process set to true
  // (6 + 3) * 4 = 36
  mws.process(true);
  REQUIRE( mult->z_() == 36 );

  // json serialization
  auto json_stream = mws.to_json();

  // clear workspace
  mws.clear();
  REQUIRE( uvw::ws::procs().size() == 0 );
  REQUIRE( uvw::ws::links().size() == 0 );
  REQUIRE( uvw::ws::vars().size() == 0 );

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