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
    uvw::Var<std::string> s_;
    bool initialize() override {
        return reg_var<double>("b", b_) && reg_var<std::string>("s", s_);
    }
};

struct Compound
{
    uvw::Var<double> comp_;
};

template<> bool uvw::Var<Compound>::from_json(const json& data)
    {return uvw::Variable::from_json(data);}
template<> json uvw::Var<Compound>::to_json()
    {return uvw::Variable::to_json();}

struct C : uvw::Processor
{
    uvw::Var<Compound> c_;
    bool initialize() override
    {
        return (
            reg_var<Compound>("c", c_) &&
            reg_var<double>("cc", c_().comp_)
        );
    }
};

TEST_CASE("Workspace ...", "[ws]")
{
    // sanity
    REQUIRE( uvw::ws::procs().size() == 0 );
    REQUIRE( uvw::ws::links().size() == 0 );
    REQUIRE( uvw::ws::vars().size() == 0 );
    REQUIRE( uvw::ws::workspaces().size() == 0 );

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
        REQUIRE( uvw::ws::vars().size() == 3 );
        REQUIRE( uvw::ws::links().size() == 1 );
        REQUIRE( uvw::ws::workspaces().size() == 1 );

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
        REQUIRE( uvw::ws::workspaces().size() == 1 );

        ws_.from_json(data);
        REQUIRE( uvw::ws::procs().size() == 2 );
        REQUIRE( uvw::ws::vars().size() == 3 );
        REQUIRE( uvw::ws::links().size() == 1 );
        REQUIRE( uvw::ws::workspaces().size() == 1 );

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
                {\"type\":\"B\", \"vars\":[\
                    {\"label\":\"s\",\"value\":\"bnode\",\"enabled\":false}\
                ]}\
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
        REQUIRE( uvw::ws::vars().size() == 3 );
        REQUIRE( uvw::ws::links().size() == 1 );
        REQUIRE( uvw::ws::workspaces().size() == 1 );

        REQUIRE( ws_.proc_ptrs().size() == 2 );
        auto* X_ = ws_.proc_ptrs()[0];
        auto* Y_ = ws_.proc_ptrs()[1];
        auto* u_ = (uvw::Var<double>*)X_->get("a");
        auto* v_ = (uvw::Var<double>*)Y_->get("b");
        auto* w_ = (uvw::Var<std::string>*)Y_->get("s");
        u_->set(1.73205);
        REQUIRE( ws_.process() == true );
        REQUIRE( v_->get() == 1.73205 );
        REQUIRE( w_->get() == "bnode" );
        REQUIRE( w_->enabled == false );
    }

    // compound vars
    uvw::ws::reg_proc("C", ([](){return new C();}));
    auto* C_ = ws_.new_proc("C");
    auto& c_ = uvw::ws::ref<Compound>(C_->get("c")->key());

    SECTION("Nexted Vars")
    {
        REQUIRE( a_->link(C_->get("cc")) == true );
        REQUIRE( ws_.set_output(b_->key()) == true ); // re-schedule

        REQUIRE( uvw::ws::procs().size() == 3 );
        REQUIRE( uvw::ws::vars().size() == 5 );
        REQUIRE( uvw::ws::links().size() == 2 );
        REQUIRE( uvw::ws::workspaces().size() == 1 );

        uvw::var::data_pull = true;
        c_.comp_.set(3.1415926);
        REQUIRE( ws_.process() == true );
        REQUIRE( b_->get() == 3.1415926 );

        // set data pull off, reference source directly
        uvw::var::data_pull = false;
        c_.comp_.set(1.414213);
        REQUIRE( a_->get() == 1.414213 );
        REQUIRE( b_->get() == 1.414213 );
    }

    SECTION("Proc Library")
    {
        // cannot clear proc lib if procs exist
        REQUIRE( uvw::ws::clear_proc_lib() == false );
        ws_.clear();
        REQUIRE( uvw::ws::clear_proc_lib() == true );
    }
}