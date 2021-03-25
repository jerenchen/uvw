#ifndef UVW_TESTS_COMMON_H
#define UVW_TESTS_COMMON_H


// Multply proc
struct Multiply: public Processor
{
  Var<double> x_, y_, z_;

  Multiply(): Processor() {}

  bool initialize() override
  {
    x_.properties["parameter"] = Variable::PAR_INPUT;
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
    return true;
  }

  bool process() override
  {
    auto& x = x_();
    auto& y = y_();
    z_() = x * y;
    return true;
  }
};

// Precomputed Addition Proc
struct PreAdd: public Processor
{
  double d_; // internal

  Var<double> a_, b_, c_;

  PreAdd(): Processor() {}

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
    d_ = a_() + b_();
    return true;
  }

  bool process() override
  {
    c_() = d_;
    return true;
  }
};

struct MyWorkpace: public uvw::Workspace
{
  MyWorkpace(): uvw::Workspace()
  {
    // build nodes and link vars so that:
    //   z = (a + b) * y
    auto* p = new_proc("PreAdd");
    auto* q = new_proc("Multiply");

    // Linking 'x' to 'c'...
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

#endif