#include <catch2/catch.hpp>

#include <uvw.h>


TEST_CASE("Variables...", "[var]")
{
    uvw::Var<double> v_;
    REQUIRE( v_.is_of_type<int>() == false );
    REQUIRE( v_.is_of_type<double>() == true );

    SECTION("Reference")
    {
        auto& v = v_.ref();
        v = 3;
        REQUIRE( v_.get() == 3 );
        v_.set(7);
        REQUIRE( v == 7 );
    }

    SECTION("Linkage")
    {
        struct Proc : uvw::Processor
        {
            uvw::Var<double> r_;
            uvw::Var<int> s_;
            uvw::Var<int> t_;

            bool initialize() override
            {
                return (
                    reg_var<double>("r", r_) &&
                    reg_var<int>("s", s_) &&
                    reg_var<int>("t", t_)
                );
            }
        };
    }
}