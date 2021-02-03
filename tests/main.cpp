#include <uvw.h>
#include <iostream>

using namespace uvw;

class MyProc: public Processor
{
  public:
  MyProc(): Processor("MyProc")
  {
    initialize();
  }

  bool initialize() override
  {
    return (
      new_var<double>("y") &&
      new_var<int>("x") &&
      new_var<int>("z")
    );
  }

  bool process() override
  {
    auto& x = var<int>("x");
    auto& y = var<double>("y");
    auto& z = var<int>("z");
    
    z = x * y;

    return true;
  }
};

int main(int argc, char * argv[])
{
  std::cout << "Adding new proc with vars \'x\' \'y\' & \'z\'..." << std::endl;
  MyProc proc;

  Processor* ptr = *(ws::procs().begin());
  Duo kx(ptr,"x");
  Duo ky(ptr,"y");
  Duo kz(ptr,"z");

  auto& x = ws::get<int>(kx);
  x = 3;
  std::cout << "\'x\' is set to " << ws::get<int>(kx) << std::endl;
  auto& y = ws::get<double>(ky);
  y = 7;
  std::cout << "\'y\' is set to " << ws::get<double>(ky) << std::endl;

  std::cout << "Processing \'z = x * y\'..." << std::endl;
  proc.process();

  auto& z = ws::get<int>(kz);
  std::cout << "\'z\' equals " << ws::get<int>(kz) << std::endl;

  return 1;
}