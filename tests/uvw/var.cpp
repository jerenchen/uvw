#include <catch2/catch.hpp>

#include <uvw.h>


TEST_CASE("Variables...", "[var]")
{
    REQUIRE( uvw::ws::procs().size() == 0 );
    REQUIRE( uvw::ws::links().size() == 0 );
    REQUIRE( uvw::ws::vars().size() == 0 );
    REQUIRE( uvw::ws::workspaces().size() == 0 );

    enum Parameter
    {
        PAR_NONPARAM,
        PAR_INPUT,
        PAR_OUTPUT
    };

    uvw::Var<double> v_;
    SECTION("Data Type")
    {
        REQUIRE( v_.is_of_type<int64_t>() == false );
        REQUIRE( v_.is_of_type<double>() == true );
    }

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
        v_.enums = {
            {"Item1", 0.333},
            {"Item2", 0.667}
        };
        v_.properties["parameter"] = PAR_INPUT;
        v_.values["default"] = -0.25;
        auto data = v_.to_json();

        uvw::Var<double> u_;
        u_.from_json(data);
        REQUIRE( u_.enabled == false );
        REQUIRE( u_.properties["parameter"] == PAR_INPUT );
        REQUIRE( u_.default_value() == -0.25 );
        REQUIRE( u_["Item0"] == -0.25 );
        REQUIRE( u_["Item2"] == 0.667 );
        REQUIRE( u_.set_enum("Item1") == true );
        REQUIRE( u_.get() == 0.333 );
        REQUIRE( u_.default_enum() == "Item1" );
        u_.values["default"] = 0.667;
        REQUIRE( u_.default_enum() == "Item2" );
        auto keys = std::vector<std::string>{"Item1", "Item2"};
        REQUIRE( u_.enum_keys() == keys );
    }
}