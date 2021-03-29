#include <catch2/catch.hpp>

#include <uvw.h>


struct A : uvw::Processor
{
    uvw::Var<double> a_;
    bool initialize() override {return reg_var<double>("a", a_);}
};

struct B : uvw::Processor
{
    uvw::Var<double> b_;
    bool initialize() override {return reg_var<double>("b", b_);}
};

TEST_CASE("Workspace ...", "[ws]")
{
    REQUIRE( uvw::ws::procs().size() == 0 );
    REQUIRE( uvw::ws::vars().size() == 0 );
    REQUIRE( uvw::ws::links().size() == 0 );

    uvw::ws::reg_proc("A", ([](){return new A();}));
    uvw::ws::reg_proc("B", ([](){return new B();}));

    uvw::ws ws_;
    auto* A_ = ws_.new_proc("A");
    auto* B_ = ws_.new_proc("B");
    auto* a_ = (uvw::Var<double>*)A_->get("a");
    auto* b_ = (uvw::Var<double>*)B_->get("b");
    REQUIRE( b_->link(a_) == true );
    REQUIRE( ws_.set_output(b_->key()) == true );
    REQUIRE( uvw::ws::procs().size() == 2 );
    REQUIRE( uvw::ws::vars().size() == 2 );
    REQUIRE( uvw::ws::links().size() == 1 );

    uvw::var::data_pull = true;
    a_->set(3.1415926);
    REQUIRE( ws_.process() == true );
    REQUIRE( b_->get() == 3.1415926 );

    // set data pull off, access source directly
    uvw::var::data_pull = false;
    a_->set(1.4142857);
    REQUIRE( b_->get() == 1.4142857 );

    // 'b_' output remains linked to 'a_', as data is not pulled
    b_->set(1.73205);
    REQUIRE( b_->get() == 1.4142857 );

    // serialize as json
    auto data = ws_.to_json();

    ws_.clear();
    REQUIRE( uvw::ws::procs().size() == 0 );
    REQUIRE( uvw::ws::vars().size() == 0 );
    REQUIRE( uvw::ws::links().size() == 0 );

    // deserialize from json
    ws_.from_json(data);
    REQUIRE( uvw::ws::procs().size() == 2 );
    REQUIRE( uvw::ws::vars().size() == 2 );
    REQUIRE( uvw::ws::links().size() == 1 );

    REQUIRE( ws_.proc_ptrs().size() == 2 );
    auto* C_ = ws_.proc_ptrs()[0];
    auto* D_ = ws_.proc_ptrs()[1];
    auto* c_ = (uvw::Var<double>*)C_->get("a");
    auto* d_ = (uvw::Var<double>*)D_->get("b");
    c_->set(2.71828);
    REQUIRE( ws_.process() == true );
    REQUIRE( d_->get() == 2.71828 );

    // cannot clear proc lib if procs exist
    REQUIRE( uvw::ws::clear_proc_lib() == false );
    ws_.clear();
    REQUIRE( uvw::ws::clear_proc_lib() == true );
}