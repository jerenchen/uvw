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

    SECTION("JSON Serialize")
    {
        v_.enabled = false;
        auto data = v_.to_json();
        uvw::Var<double> u_;
        u_.from_json(data);
        REQUIRE( u_.enabled == false );
    }
}