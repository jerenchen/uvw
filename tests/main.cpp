#include <uvw.h>
using namespace uvw;


// Multply proc
class MultProc: public Processor
{
  public:
  Var<double> x_, y_, z_;

  MultProc(): Processor() {}

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
class PreAddProc: public Processor
{
  double d_; // internal
  public:
  Var<double> a_, b_, c_;

  PreAddProc(): Processor() {}

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

std::function<Processor*()> create_preadd = [](){return new PreAddProc();};

// define workspace
struct MyWorkpace: public uvw::Workspace
{
  MyWorkpace(): uvw::Workspace()
  {
    // build nodes and link vars so that:
    //   z = (a + b) * y
    auto* p = new_proc("PreAdd");
    auto* q = new_proc("Mult");

    std::cout << "Linking 'x' to 'c'..." << std::endl;
    q->get("x")->link(p->get("c"));
    // or alternatively...
    //ws::link(uvw::duo(p,"c"), uvw::duo(q,"x"));

    set_output(uvw::duo(q,"z"));
  }

  // helper to get procs
  PreAddProc* preadd_proc()
  {
    return proc_ptrs_.size()?
      static_cast<PreAddProc*>(proc_ptrs_[0]) : nullptr;
  };
  MultProc* mult_proc()
  {
    return proc_ptrs_.size()>1?
      static_cast<MultProc*>(proc_ptrs_[1]) : nullptr;
  };
};

int main(int argc, char * argv[])
{
  // proc registration
  uvw::ws::reg_proc("PreAdd", create_preadd);
  uvw::ws::reg_proc("Mult", ([](){return new MultProc();})); // inplace func

  MyWorkpace mws;
  auto* add = mws.preadd_proc();
  
  // create duo hash keys
  uvw::duo add_a(add,"a");
  uvw::duo add_b(add,"b");
  uvw::duo add_c(add,"c");

  std::cout << "'add->c' is a double? " << add->c_.is_of_type<double>() << std::endl;
  std::cout << "'add->c' is an int? " << add->c_.is_of_type<int>() << std::endl;

  // access variable's reference through proc
  add->ref<double>("a") = 2;
  std::cout << "\'add->a\' is set to " << add->ref<double>("a") << std::endl;
  // access through workspace via duo hash key
  ws::ref<double>(add_b) = 3;
  std::cout << "\'add->b\' is set to " << ws::ref<double>(add_b) << std::endl;

  auto* mult = mws.mult_proc();
  uvw::duo mult_x(mult,"x");
  uvw::duo mult_y(mult,"y");
  uvw::duo mult_z(mult,"z");

  // access variable's reference directly (if accessiable i.e. public)
  mult->y_.set(7);
  std::cout << "\'mult->y\' is set to " << mult->y_.get() << std::endl;

  // generate processing sequence
  mws.process(true);

  std::cout << "\'z\' equals " << ws::ref<double>(mult_z) << " (expected 35)" << std::endl;

  add->a_.set(6);
  std::cout << "\'add->a\' is now set to " << ws::ref<double>(add_a) << 
    ", but 'add' won't be pre-processed this time." << std::endl;
  mult->y_.set(4);
  std::cout << "\'mult->y\' is now set to " << mult->y_.get() << std::endl;

  mws.process();

  std::cout << "\'z\' now equals " << mult->z_() << " (expected 20)" << std::endl;

  std::cout << "Re-running with pre-processes..." << std::endl;
  mws.process(true);

  std::cout << "\'z\' now equals " << mult->ref<double>("z") << " (expected 36)" << std::endl;

  std::cout << ws::stats() << std::endl;

  // serialize
  auto json_stream = mws.to_json();
  std::cout << json_stream.dump(2) << std::endl;

  // clear workspace
  std::cout << "Clean up workspace before deserializing..." << std::endl;
  mws.clear();

  std::cout << ws::stats() << std::endl;

  // deserialize
  mws.from_json(json_stream);

  std::cout << "json identical? " <<
    (json_stream.dump().compare(mws.to_json().dump()) == 0) <<
    std::endl;

  std::cout << ws::stats() << std::endl;

  add = mws.preadd_proc();
  mult = mws.mult_proc();

  add->a_.set(8);
  add->b_.set(3);
  mult->y_.set(2);

  mws.process(true);

  std::cout << "\'z\' now equals " << mult->ref<double>("z") << " (expected 22)" << std::endl;

  return 1;
}