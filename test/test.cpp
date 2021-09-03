#include <confetti/confetti.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"



TEST_CASE("parse empty string") {
    confetti::result r = confetti::parse_text(nullptr);
    REQUIRE(!!r);
    REQUIRE(r.config.contains("default"));

    r = confetti::parse_text("");
    REQUIRE(!!r);
    REQUIRE(r.config.contains("default"));
}



TEST_CASE("parse default section") {
    confetti::result r = confetti::parse_text(" [ Default ] ");
    REQUIRE(!!r);
    REQUIRE(r.config.contains("default"));

    r = confetti::parse_text("[default");
        
    REQUIRE_EQ(r.error_code.value(),
        int(confetti::error::invalid_section_name));
    REQUIRE_EQ(r.line_no, 1);
}



TEST_CASE("parse outside sections") {
    confetti::result r = confetti::parse_text(" X = foo ");
    REQUIRE(!!r);
    REQUIRE(r.config.contains("default"));

    auto const& defaults = r.config["default"];
    REQUIRE_EQ(defaults.size(), 1);

    REQUIRE(defaults.contains("x"));
    auto const& x = defaults["x"];
    REQUIRE(x.is_single());
    auto const maybe_foo = x | "";
    REQUIRE(maybe_foo);
    REQUIRE_EQ(*maybe_foo, "foo");
 
    REQUIRE_FALSE(defaults.contains("y"));
    auto const& y = defaults["y"];
    REQUIRE(y.is_none());
    auto const maybe_bar = y | "bar";
    REQUIRE(maybe_bar);
    REQUIRE_EQ(*maybe_bar, "bar");
}



TEST_CASE("parse inside section") {
    confetti::result r = confetti::parse_text(
        "[sectioN]\n"
        "keY = value");
    REQUIRE(r);
    REQUIRE(r.config.contains("section"));

    auto const& section = r.config["section"];
    REQUIRE(section.contains("key"));
    auto const maybe_value = section["key"] | "";
    REQUIRE(maybe_value);
    REQUIRE_EQ(*maybe_value, "value");
}



TEST_CASE("parse invalid configs") {
    confetti::result r = confetti::parse_text("key = 'foo' bar");
    REQUIRE(!r);
    REQUIRE_EQ(r.error_code.value(),
               int(confetti::error::expected_equal_after_parameter_name));

    r = confetti::parse_text("k1 = v1\n k1 = v2");
    REQUIRE(!r);
    REQUIRE_EQ(r.error_code.value(),
               int(confetti::error::duplicated_parameter));

    r = confetti::parse_text("k1 = v1\n k2 = ");
    REQUIRE(!r);
    REQUIRE_EQ(r.error_code.value(),
               int(confetti::error::invalid_parameter_value));
}


TEST_CASE("parse boolean") {
    confetti::result r = confetti::parse_text(
        "k1 = true\n"
        "k2 = false\n"
        "k3 = True\n"
        "k4 = False\n");
    REQUIRE(!!r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | false;
    REQUIRE(k1);
    REQUIRE(*k1);
    auto const k2 = section["k2"] | true;
    REQUIRE(k2);
    REQUIRE_FALSE(*k2);
    auto const k3 = section["k3"] | false;
    REQUIRE(k3);
    REQUIRE(*k3);
    auto const k4 = section["k4"] | true;
    REQUIRE(k4);
    REQUIRE_FALSE(*k4);
}


TEST_CASE("parse integer") {
    confetti::result r = confetti::parse_text(
        "k1 = -2147483648\n"
        "k2 = +2147483647\n"
        "k3 = 0xFCED\n");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | -1;
    REQUIRE(k1);
    REQUIRE_EQ(*k1, -2147483647 - 1);
    auto const k2 = section["k2"] | -1;
    REQUIRE(k2);
    REQUIRE_EQ(*k2, 2147483647);
    auto const k3 = section["k3"] | -1;
    REQUIRE_FALSE(k3);
}


TEST_CASE("parse unsigned") {
    confetti::result r = confetti::parse_text(
        "k1 = 4294967295\n"
        "k2 = 0xFCED\n"
        "k3 = 0x");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | 1u;
    REQUIRE(k1);
    REQUIRE_EQ(*k1, 4294967295);
    auto const k2 = section["k2"] | 1u;
    REQUIRE(k2);
    REQUIRE_EQ(*k2, 0xFCED);
    auto const k3 = section["k3"] | 1u;
    REQUIRE_FALSE(k3);
}


TEST_CASE("parse long integer") {
    confetti::result r = confetti::parse_text(
        "k1 = -9223372036854775808\n"
        "k2 = 9223372036854775807\n");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | 0ll;
    REQUIRE(k1);
    REQUIRE_EQ(*k1, -9223372036854775807 - 1);
    auto const k2 = section["k2"] | 0ll;
    REQUIRE(k2);
    REQUIRE_EQ(*k2, 9223372036854775807);
}


TEST_CASE("parse double") {
    confetti::result r = confetti::parse_text("k1 = -3.14E+2\n");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | -1.;
    REQUIRE(k1);
    REQUIRE_EQ(*k1, -314.);
}



TEST_CASE("parse string") {
    confetti::result r = confetti::parse_text(
        "k1 = \"\"\n"
        "k2 = ''\n"
        "k3 = \"'foo'\"\n"
        "k4 = '\"bar\"'\n");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | "foo";
    REQUIRE(k1);
    REQUIRE_EQ(*k1, "");
    auto const k2 = section["k2"] | "bar";
    REQUIRE(k2);
    REQUIRE_EQ(*k2, "");
    auto const k3 = section["k3"] | "";
    REQUIRE(k3);
    REQUIRE_EQ(*k3, "'foo'");
    auto const k4 = section["k4"] | "";
    REQUIRE(k4);
    REQUIRE_EQ(*k4, "\"bar\"");
}



TEST_CASE("parse string view") {
    confetti::result r = confetti::parse_text(
        "k1 = \"\"\n"
        "k2 = ''\n"
        "k3 = \"'foo'\"\n"
        "k4 = '\"bar\"'\n");
    REQUIRE(r);
    auto const& section = r.config["default"];
    auto const k1 = section["k1"] | std::string_view{"foo"};
    REQUIRE(k1);
    REQUIRE_EQ(*k1, "");
    auto const k2 = section["k2"] | std::string_view{"bar"};
    REQUIRE(k2);
    REQUIRE_EQ(*k2, "");
    auto const k3 = section["k3"] | std::string_view{};
    REQUIRE(k3);
    REQUIRE_EQ(*k3, "'foo'");
    auto const k4 = section["k4"] | std::string_view{};
    REQUIRE(k4);
    REQUIRE_EQ(*k4, "\"bar\"");
}


TEST_CASE("parse array") {
    confetti::result r = confetti::parse_text(
        "k1 = []\n"
        "k2 = [\n 1\n, 2\n]\n");

    REQUIRE(r);

    auto const& section = r.config["default"];
    auto const& k1 = section["k1"];
    REQUIRE(k1.is_array());
    REQUIRE(k1.empty());
    auto const& k2 = section["k2"] | std::vector<int>{};
    REQUIRE(k2);
    REQUIRE_EQ(k2->size(), 2);
    REQUIRE_EQ((*k2)[0], 1);
    REQUIRE_EQ((*k2)[1], 2);
}


TEST_CASE("parse inline table") {
    confetti::result r =
        confetti::parse_text("k1 = {\n x = 'foo bar', y = [\n1,\n2]\n}\n");

    REQUIRE(r);
    auto const& table = r.config["default"]["k1"];
    auto const x = table["x"] | "";
    REQUIRE(x);
    REQUIRE_EQ(*x, "foo bar");
    auto const y = table["y"] | std::vector<int>{};
    REQUIRE(y);
    REQUIRE_EQ(y->size(), 2);
    REQUIRE_EQ((*y)[0], 1);
    REQUIRE_EQ((*y)[1], 2);
}


TEST_CASE("parse inline array of tables") {
    confetti::result r = confetti::parse_text(
        "data = [\n{k = foo, v = bar},\n{k = bar, v = foo}]");

    REQUIRE(r);
    auto const& data = r.config["default"]["data"];
    REQUIRE(data.is_array());
    REQUIRE_EQ(data.size(), 2);
    auto const& table1 = data[0];
    auto const k1 = table1["k"] | "";
    REQUIRE(k1);
    REQUIRE_EQ(*k1, "foo");
    auto const v1 = table1["v"] | "";
    REQUIRE(v1);
    REQUIRE_EQ(*v1, "bar");
    auto const& table2 = data[1];
    auto const k2 = table2["k"] | "";
    REQUIRE(k1);
    REQUIRE_EQ(*k2, "bar");
    auto const v2 = table2["v"] | "";
    REQUIRE(v2);
    REQUIRE_EQ(*v2, "foo");
}
