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

  std::cout << "\'x\' exists? " << ws::has_var("x") << std::endl;
  std::cout << "\'y\' exists? " << ws::has_var("y") << std::endl;
  std::cout << "\'z\' exists? " << ws::has_var("z") << std::endl;
  std::cout << "\'w\' exists? " << ws::has_var("w") << std::endl;

  auto& x = ws::get<int>("x");
  x = 3;
  std::cout << "\'x\' is set to " << ws::get<int>("x") << std::endl;
  auto& y = ws::get<double>("y");
  y = 7;
  std::cout << "\'y\' is set to " << ws::get<double>("y") << std::endl;

  std::cout << "Processing \'z = x * y\'..." << std::endl;
  proc.process();

  auto& z = ws::get<int>("z");
  std::cout << "\'z\' equals " << ws::get<int>("z") << std::endl;

  return 1;
}