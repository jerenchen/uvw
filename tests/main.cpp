#include <uvw.h>
#include <iostream>

using namespace uvw;

class MyProc: public Processor
{
  int x_, z_;
  double y_;

  public:
  MyProc(): Processor("MyProc")
  {
    initialize();
  }

  bool initialize() override
  {
    return (
      new_var<double>("y", y_) &&
      new_var<int>("x", x_) &&
      new_var<int>("z", z_)
    );
  }

  bool process() override
  {
    z_ = x_ * y_;
    return true;
  }
};

int main(int argc, char * argv[])
{
  std::cout << "Adding new proc with vars \'x\' \'y\' & \'z\'..." << std::endl;
  MyProc proc;

  Duo kx(&proc,"x");
  Duo ky(&proc,"y");
  Duo kz(&proc,"z");

  auto& x = ws::get<int>(kx);
  x = 3;
  std::cout << "\'x\' is set to " << ws::get<int>(kx) << std::endl;
  auto& y = ws::get<double>(ky);
  y = 7;
  std::cout << "\'y\' is set to " << ws::get<double>(ky) << std::endl;

  std::cout << "Processing \'z = x * y\'..." << std::endl;
  proc.process();

  auto& z = ws::get<int>(kz);
  std::cout << "\'z\' equals " << z << std::endl;

  return 1;
}