#include <catch2/catch.hpp>

#include <uvw.h>


struct TestProc : uvw::Processor
{
    uvw::Var<double> x_;
};

TEST_CASE("Duohash...", "[duo]")
{
    uvw::Processor p_;
    uvw::Var<double> v_;

    uvw::Duohash key(&p_, "v");
    REQUIRE( key.raw_ptr == &p_ );

    SECTION("assignment")
    {
        key = uvw::Duohash();
        REQUIRE( key.raw_ptr == nullptr );
        REQUIRE( key.var_str.empty() == true );
        REQUIRE( key.is_null() == true );
    }
}