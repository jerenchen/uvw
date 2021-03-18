#include <catch2/catch.hpp>

#include <uvw.h>


TEST_CASE("Duohash...", "[duo]")
{
    uvw::Processor p_, q_;

    SECTION("Assignment")
    {
        uvw::Duohash key(&p_, "v");
        REQUIRE( key.raw_ptr == &p_ );
        REQUIRE( key.var_str == "v" );

        auto clone = key;
        REQUIRE( clone.raw_ptr == &p_ );
        REQUIRE( clone.var_str == "v" );
        REQUIRE( clone.is_null() == false );

        key = uvw::Duohash();
        REQUIRE( key.raw_ptr == nullptr );
        REQUIRE( key.var_str.empty() == true );
        REQUIRE( key.is_null() == true );

        REQUIRE( clone.raw_ptr == &p_ );
        REQUIRE( clone.var_str == "v" );
        REQUIRE( clone.is_null() == false );
    }

    SECTION("Operator==")
    {
        REQUIRE( (uvw::Duohash(&p_, "u") == uvw::Duohash(&p_, "u")) == true );
        REQUIRE( (uvw::Duohash(&p_, "u") == uvw::Duohash(&p_, "v")) == false );
        REQUIRE( (uvw::Duohash(&p_, "u") == uvw::Duohash(&q_, "u")) == false );
        REQUIRE( (uvw::Duohash(&p_, "u") == uvw::Duohash(&q_, "v")) == false );
    }

    SECTION("Nullification")
    {
        uvw::Duohash key(&p_, "v");
        REQUIRE( key.raw_ptr == &p_ );
        REQUIRE( key.var_str == "v" );
        REQUIRE( key.is_null() == false );

        key.nullify();
        REQUIRE( key.raw_ptr == nullptr );
        REQUIRE( key.var_str.empty() == true );
        REQUIRE( key.is_null() == true );
    }

    SECTION("Unordered Maps")
    {
        std::unordered_map<uvw::Duohash, uvw::Duohash> key_maps;
        uvw::Duohash src(&p_, "u");
        uvw::Duohash dst(&q_, "v");

        key_maps[dst] = src;
        auto clone = key_maps[dst];
        REQUIRE( clone.raw_ptr == &p_ );
        REQUIRE( clone.var_str == "u" );

        src = dst = uvw::Duohash();
        REQUIRE( src.is_null() == true );
        REQUIRE( dst.is_null() == true );

        auto clone2 = key_maps[uvw::Duohash(&q_,"v")];
        REQUIRE( clone2.raw_ptr == &p_ );
        REQUIRE( clone2.var_str == "u" );

        REQUIRE( key_maps.erase(uvw::Duohash(&q_,"v")) == 1) ;
        REQUIRE( clone2.raw_ptr == &p_ );
        REQUIRE( clone2.var_str == "u" );
        REQUIRE( clone2 == clone );
    }
}