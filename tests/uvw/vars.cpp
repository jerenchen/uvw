#include <catch2/catch.hpp>

#include <uvw.h>


TEST_CASE("Vars...", "[Variables]")
{
    uvw::Var<double> v_;
    REQUIRE( v_.is_of_type<int>() == false );
    REQUIRE( v_.is_of_type<double>() == true );

    auto& v = v_.ref();
    v = 3;
    REQUIRE( v_.get() == 3 );
    v_.set(7);
    REQUIRE( v == 7 );
}