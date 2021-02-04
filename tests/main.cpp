#include <uvw.h>
#include <iostream>

using namespace uvw;

// Multply proc
class MultProc: public Processor
{
  double x_, y_, z_;
  std::string name_;

  public:
  MultProc(): Processor()
  {
    initialize();
  }

  void cleanup() override
  {
    del_var<double>("x");
    del_var<double>("y");
    del_var<double>("z");
    del_var<std::string>("name");
  }

  bool initialize() override
  {
    return (
      new_var<double>("y", y_) &&
      new_var<double>("x", x_) &&
      new_var<double>("z", z_) &&
      new_var<std::string>("name", name_)
    );
  }

  bool process() override
  {
    std::cout << "Processing 'Mult'..." << std::endl;
    z_ = x_ * y_;
    return true;
  }
};

// Addition Proc
class AddProc: public Processor
{
  double a_, b_, c_;

  public:
  AddProc(): Processor()
  {
    initialize();
  }

  void cleanup() override
  {
    for (auto& label : {"a", "b", "c"})
    {
      del_var<double>(label);
    }
  }

  bool initialize() override
  {
    return (
      new_var<double>("a", a_) &&
      new_var<double>("b", b_) &&
      new_var<double>("c", c_)
    );
  }

  bool process() override
  {
    std::cout << "Processing 'Add'..." << std::endl;
    c_ = a_ + b_;
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

  ws::get<double>(add_a) = 2;
  std::cout << "\'add.a\' is set to " << add.var<double>("a") << std::endl;
  ws::get<double>(add_b) = 3;
  std::cout << "\'add.b\' is set to " << add.var<double>("b") << std::endl;

  std::cout << "Adding 'mult' proc with vars \'x\' \'y\' & \'z\'..." << std::endl;
  MultProc mult;

  Duo mult_x(&mult,"x");
  Duo mult_y(&mult,"y");
  Duo mult_z(&mult,"z");

  if (ws::link(add_c, mult_x))
  {
    std::cout << "Linked \'mult.x\' to \'add.c\'..." << std::endl;
  }

  ws::get<double>(mult_y) = 7;
  std::cout << "\'mult.y\' is set to " << mult.var<double>("y") << std::endl;

  std::cout << "Processing \'z = (a + b) * y\'..." << std::endl;
  auto seq = ws::schedule(mult_z);
  ws::execute(seq);

  std::cout << "\'z\' equals " << ws::get<int>(mult_z) << std::endl;

  return 1;
}