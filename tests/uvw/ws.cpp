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
    // sanity
    REQUIRE( uvw::ws::procs().size() == 0 );
    REQUIRE( uvw::ws::vars().size() == 0 );
    REQUIRE( uvw::ws::links().size() == 0 );

    SECTION("Proc Registration")
    {
        uvw::ws::reg_proc("A", ([](){return new A();}));
        uvw::ws::reg_proc("B", ([](){return new B();}));
    }

    // construct ws
    uvw::ws ws_;
    auto* A_ = ws_.new_proc("A");
    auto* B_ = ws_.new_proc("B");
    auto* a_ = (uvw::Var<double>*)A_->get("a");
    auto* b_ = (uvw::Var<double>*)B_->get("b");
    b_->link(a_);
    ws_.set_output(b_->key());

    SECTION("Process")
    {
        REQUIRE( uvw::ws::procs().size() == 2 );
        REQUIRE( uvw::ws::vars().size() == 2 );
        REQUIRE( uvw::ws::links().size() == 1 );

        uvw::var::data_pull = true;
        a_->set(3.1415926);
        REQUIRE( ws_.process() == true );
        REQUIRE( b_->get() == 3.1415926 );

        // set data pull off, access source directly
        uvw::var::data_pull = false;
        a_->set(1.414213);
        REQUIRE( b_->get() == 1.414213 );

        // 'b_' output remains linked to 'a_', as data is not pulled
        b_->set(1.73205);
        REQUIRE( b_->get() == 1.414213 );
    }

    // serialize as json
    SECTION("JSON Serialize")
    {
        auto data = ws_.to_json();
        ws_.clear();

        REQUIRE( uvw::ws::procs().size() == 0 );
        REQUIRE( uvw::ws::vars().size() == 0 );
        REQUIRE( uvw::ws::links().size() == 0 );

        ws_.from_json(data);
        REQUIRE( uvw::ws::procs().size() == 2 );
        REQUIRE( uvw::ws::vars().size() == 2 );
        REQUIRE( uvw::ws::links().size() == 1 );

        REQUIRE( ws_.proc_ptrs().size() == 2 );
        auto* X_ = ws_.proc_ptrs()[0];
        auto* Y_ = ws_.proc_ptrs()[1];
        auto* u_ = (uvw::Var<double>*)X_->get("a");
        auto* v_ = (uvw::Var<double>*)Y_->get("b");
        u_->set(2.71828);
        REQUIRE( ws_.process() == true );
        REQUIRE( v_->get() == 2.71828 );
    }

    SECTION("String JSON Serialize")
    {
        ws_.clear();
        std::string s = "{\
            \"procs\":[\
                {\"type\":\"A\"},\
                {\"type\":\"B\"}\
            ],\
            \"links\":[\
                {\
                    \"src\":{\"index\":0,\"label\":\"a\"},\
                    \"var\":{\"index\":1,\"label\":\"b\"}\
                }\
            ],\
            \"out\": {\"index\":1,\"label\":\"b\"}\
        }";
        REQUIRE( ws_.from_str(s) == true );
        REQUIRE( uvw::ws::procs().size() == 2 );
        REQUIRE( uvw::ws::vars().size() == 2 );
        REQUIRE( uvw::ws::links().size() == 1 );

        REQUIRE( ws_.proc_ptrs().size() == 2 );
        auto* X_ = ws_.proc_ptrs()[0];
        auto* Y_ = ws_.proc_ptrs()[1];
        auto* u_ = (uvw::Var<double>*)X_->get("a");
        auto* v_ = (uvw::Var<double>*)Y_->get("b");
        u_->set(1.73205);
        REQUIRE( ws_.process() == true );
        REQUIRE( v_->get() == 1.73205 );
    }

    SECTION("Proc Library")
    {
        // cannot clear proc lib if procs exist
        REQUIRE( uvw::ws::clear_proc_lib() == false );
        ws_.clear();
        REQUIRE( uvw::ws::clear_proc_lib() == true );
    }
}