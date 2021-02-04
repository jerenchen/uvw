
#include <uvw.h>
using namespace uvw;

// Multply proc
class MultProc: public Processor
{
  public:
  Var<double> x_, y_, z_;

  MultProc(): Processor()
  {
    initialize();
  }

  bool initialize() override
  {
    x_.parameter = true;
    return (
      reg_var<double>("y", y_) &&
      reg_var<double>("x", x_) &&
      reg_var<double>("z", z_)
    );
  }

  bool preprocess() override
  {
    std::cout << "No action in taken pre-processing ..." << std::endl;
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

  PreAddProc(): Processor()
  {
    initialize();
  }

  bool initialize() override
  {
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

int main(int argc, char * argv[])
{
  std::cout << "Adding 'add' proc with vars \'a\' \'b\' & \'c\'..." << std::endl;
  PreAddProc add;

  Duo add_a(&add,"a");
  Duo add_b(&add,"b");
  Duo add_c(&add,"c");

  add.a_() = 2;
  std::cout << "\'add.a\' is set to " << ws::ref<double>(add_a) << std::endl;
  ws::ref<double>(add_b) = 3;
  std::cout << "\'add.b\' is set to " << add.b_() << std::endl;

  std::cout << "Adding 'mult' proc with vars \'x\' \'y\' & \'z\'..." << std::endl;
  MultProc mult;

  Duo mult_x(&mult,"x");
  Duo mult_y(&mult,"y");
  Duo mult_z(&mult,"z");

  if (ws::link(add_c, mult_x))
  {
    std::cout << "Linked \'mult.x\' to \'add.c\'..." << std::endl;
  }

  mult.y_.set(7);
  std::cout << "\'mult.y\' is set to " << mult.y_.get() << std::endl;

  // generate processing sequence
  auto seq = ws::schedule(mult_z);

  ws::execute(seq, true);

  std::cout << "\'z\' equals " << mult.z_() << " (expected 35)" << std::endl;

  add.a_.set(6);
  std::cout << "\'add.a\' is now set to " << ws::ref<double>(add_a) << 
    ", but 'add' won't be pre-processed this time." << std::endl;
  mult.y_.set(4);
  std::cout << "\'mult.y\' is now set to " << mult.y_.get() << std::endl;

  ws::execute(seq);

  std::cout << "\'z\' now equals " << mult.z_() << " (expected 20)" << std::endl;

  std::cout << "Re-running with pre-processes..." << std::endl;
  ws::execute(seq, true);

  std::cout << "\'z\' now equals " << mult.z_() << " (expected 36)" << std::endl;

  return 1;
}