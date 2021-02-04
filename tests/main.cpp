
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
    return (
      reg_var<double>("y", y_) &&
      reg_var<double>("x", x_) &&
      reg_var<double>("z", z_)
    );
  }

  bool process() override
  {
    auto& x = x_();
    auto& y = y_();
    std::cout << "Processing " << x << " '*' " << y << "..." << std::endl;
    z_() = x * y;
    return true;
  }
};

// Addition Proc
class AddProc: public Processor
{
  public:
  Var<double> a_, b_, c_;

  AddProc(): Processor()
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

  bool process() override
  {
    auto& c = c_();
    auto& b = b_();
    auto& a = a_();
    std::cout << "Processing " << a << " '+' " << b << "..." << std::endl;
    c = a + b;
    return true;
  }
};

int main(int argc, char * argv[])
{
  std::cout << "Adding 'add' proc with vars \'a\' \'b\' & \'c\'..." << std::endl;
  AddProc add;

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

  std::cout << "Processing \'z = (a + b) * y\'..." << std::endl;
  auto seq = ws::schedule(mult_z);
  ws::execute(seq);

  std::cout << "\'z\' equals " << mult.z_() << std::endl;

  return 1;
}