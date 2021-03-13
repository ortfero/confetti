#include <confetti/confetti.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"



TEST_CASE("parse empty string") {
  
  confetti::result r = confetti::parse_string(nullptr);
  REQUIRE(!!r);
  REQUIRE(r.config.contains("default"));

  r = confetti::parse_string("");
  REQUIRE(!!r);
  REQUIRE(r.config.contains("default"));
}



TEST_CASE("parse default section") {

  confetti::result r = confetti::parse_string(" [ Default ] ");
  REQUIRE(!!r);
  REQUIRE(r.config.contains("default"));

  r = confetti::parse_string("[default");
  REQUIRE_EQ(r.error.code(), confetti::make_error_code(confetti::error::invalid_section_name));
  REQUIRE_EQ(r.line_no, 1);
}



TEST_CASE("parse outside sections") {

  confetti::result r = confetti::parse_string(" X = foo ");
  REQUIRE(!!r);
  REQUIRE(r.config.contains("default"));

  auto const& def = r.config["default"];
  REQUIRE_EQ(def.size(), 1);

  REQUIRE(def.contains("x"));
  auto const maybe_foo = def["x"].to<std::string>();
  REQUIRE(maybe_foo);
  REQUIRE_EQ(*maybe_foo, "foo");
  auto const foo = def["x"] | "";
  REQUIRE_EQ(foo, "foo");

  REQUIRE_FALSE(def.contains("y"));
  auto const maybe_nothing = def["y"].to<std::string>();
  REQUIRE_FALSE(maybe_nothing);
  auto const bar = def["y"] | "bar";
  REQUIRE_EQ(bar, "bar");
}



TEST_CASE("parse inside section") {

  confetti::result r = confetti::parse_string("[sectioN]\n" "keY = value");
  REQUIRE(r);
  REQUIRE(r.config.contains("section"));

  auto const& section = r.config["section"];
  REQUIRE(section.contains("key"));
  auto const value = section["key"] | "";
  REQUIRE_EQ(section["key"] | "", "value");
}



TEST_CASE("parse invalid configs") {
  confetti::result r = confetti::parse_string("key = 'foo' bar");
  REQUIRE(!r);
  REQUIRE_EQ(r.error.code(), confetti::make_error_code(confetti::error::unexpected_chars_after_value));

  r = confetti::parse_string("k1 = v1 k2 = v2");
  REQUIRE(!r);
  REQUIRE_EQ(r.error.code(), confetti::make_error_code(confetti::error::unexpected_equal_sign));

  r = confetti::parse_string("k1 = v1\n k1 = v2");
  REQUIRE(!r);
  REQUIRE_EQ(r.error.code(), confetti::make_error_code(confetti::error::duplicated_item));

  r = confetti::parse_string("k1 = v1\n k2 = ");
  REQUIRE(!r);
  REQUIRE_EQ(r.error.code(), confetti::make_error_code(confetti::error::expected_value));

}


TEST_CASE("parse boolean") {

  confetti::result r = confetti::parse_string(
                         "k1 = true\n" "k2 = false\n"
                         "k3 = on\n" "k4 = off\n"
                         "k5 = True\n" "k6 = False\n"
                         "k7 = ON\n" "k8 = OFF\n");
  REQUIRE(!!r);
  auto const& section = r.config["default"];
  auto const k1 = section["k1"] | false;
  REQUIRE(k1);
  auto const k2 = section["k2"] | true;
  REQUIRE_FALSE(k2);
  auto const k3 = section["k3"] | false;
  REQUIRE(k3);
  auto const k4 = section["k4"] | true;
  REQUIRE_FALSE(k4);
  auto const k5 = section["k5"] | false;
  REQUIRE(k5);
  auto const k6 = section["k6"] | true;
  REQUIRE_FALSE(k6);
  auto const k7 = section["k7"] | false;
  REQUIRE(k7);
  auto const k8 = section["k8"] | true;
  REQUIRE_FALSE(k8);
}


TEST_CASE("parse integer") {

  confetti::result r = confetti::parse_string(
                         "k1 = -2147483648\n" "k2 = +2147483647\n"
                         "k3 = 0xFCED\n");
  REQUIRE(r);
  auto const& section = r.config["default"];
  REQUIRE_EQ(section["k1"] | -1, -2147483647 - 1);
  REQUIRE_EQ(section["k2"] | -1, 2147483647);
  REQUIRE_EQ(section["k3"] | -1, -1);
}


TEST_CASE("parse unsigned") {

  confetti::result r = confetti::parse_string(
                         "k1 = 4294967295\n" "k2 = 0xFCED\n"
                         "k3 = 0x");
  REQUIRE(r);
  auto const& section = r.config["default"];
  REQUIRE_EQ(section["k1"] | 1u, 4294967295);
  REQUIRE_EQ(section["k2"] | 1u, 0xFCED);
  REQUIRE_EQ(section["k3"] | 1u, 1u);
}


TEST_CASE("parse long integer") {

  confetti::result r = confetti::parse_string(
                         "k1 = -9223372036854775808\n"
                         "k2 = 9223372036854775807\n");
  REQUIRE(r);
  auto const& section = r.config["defaults"];
  REQUIRE_EQ(section["k1"] | 0ll, -9223372036854775807 - 1);
  REQUIRE_EQ(section["k2"] | 0ll, 9223372036854775807);
}


TEST_CASE("parse double") {

  confetti::result r = confetti::parse_string(
                         "k1 = -3.14E+2\n");
  REQUIRE(r);
  auto const& section = r.config["default"];
  REQUIRE_EQ(section["k1"] | -1., -314.);
}



TEST_CASE("parse string") {

  confetti::result r = confetti::parse_string(
                         "k1 = \"\"\n" "k2 = ''\n"
                         "k3 = \"'foo'\"\n" "k4 = '\"bar\"'\n"
                         "k5 = foo' bar'\n");
  REQUIRE(r);
  auto const& section = r.config["default"];
  REQUIRE_EQ(section["k1"] | "foo", "");
  REQUIRE_EQ(section["k2"] | "bar", "");
  REQUIRE_EQ(section["k3"] | "", "'foo'");
  REQUIRE_EQ(section["k4"] | "", "\"bar\"");
  REQUIRE_EQ(section["k5"] | "", "foo' bar'");
}



TEST_CASE("parse string view") {

  using namespace std::string_view_literals;

  confetti::result r = confetti::parse_string(
                         "k1 = \"\"\n" "k2 = ''\n"
                         "k3 = \"'foo'\"\n" "k4 = '\"bar\"'\n");
  REQUIRE(r);
  auto const& section = r.config["default"];
  REQUIRE_EQ(section["k1"] | "foo"sv, "");
  REQUIRE_EQ(section["k2"] | "bar"sv, "");
  REQUIRE_EQ(section["k3"] | ""sv, "'foo'");
  REQUIRE_EQ(section["k4"] | ""sv, "\"bar\"");
}


TEST_CASE("parse array") {

  confetti::result r = confetti::parse_string(
                         "k1 = []\n" "k2 = [\n 1\n, 2\n]\n");

  REQUIRE(r);

  auto const& section = r.config["default"];
  std::vector<int> none = {-1};

  auto const k1 = section["k1"] | none;
  REQUIRE(k1.empty());
  auto const k2 = section["k2"] | none;
  REQUIRE_EQ(k2.size(), 2); REQUIRE_EQ(k2[0], 1); REQUIRE_EQ(k2[1], 2);
}


TEST_CASE("parse inline table") {

  confetti::result r = confetti::parse_string(
                         "k1 = {\n x = foo bar, y = [\n1,\n2]\n}\n");

  REQUIRE(r);
  auto const& table = r.config["default"]["k1"];
  REQUIRE_EQ(table["x"] | "", "foo bar");
  auto const& array = table["y"];
  REQUIRE_EQ(array.size(), 2);
  REQUIRE_EQ(array[0] | 0, 1);
  REQUIRE_EQ(array[1] | 1, 2);
}


TEST_CASE("parse inline array of tables") {

  confetti::result r = confetti::parse_string(
                         "data = [\n{k = foo, v = bar},\n{k = bar, v = foo}]");

  REQUIRE(r);
  auto const& data = r.config["default"]["data"];
  REQUIRE_EQ(data.size(), 2);
  auto const& table1 = data[0];
  REQUIRE_EQ(table1["k"] | "", "foo");
  REQUIRE_EQ(table1["v"] | "", "bar");
  auto const& table2 = data[1];
  REQUIRE_EQ(table2["k"] | "", "bar");
  REQUIRE_EQ(table2["v"] | "", "foo");
}


TEST_CASE("parse array of tables") {

  confetti::result r = confetti::parse_string(
                         "[[data]]\n"
                         "k = foo\nv = bar\n"
                         "[[data]]\n"
                         "k = bar\nv = foo\n");

  REQUIRE(r);
  auto const& data = r.config["default"]["data"];
  REQUIRE_EQ(data.size(), 2);
  auto const& table1 = data[0];
  REQUIRE_EQ(table1["k"] | "", "foo");
  REQUIRE_EQ(table1["v"] | "", "bar");
  auto const& table2 = data[1];
  REQUIRE_EQ(table2["k"] | "", "bar");
  REQUIRE_EQ(table2["v"] | "", "foo");
}


TEST_CASE("parse matrix") {
    confetti::result r = confetti::parse_string(
                            "matrix = [[1, 2],\n"
                            "          [3, 4]]\n");
    REQUIRE(r);
    auto const& matrix = r.config["default"]["matrix"];
    auto const d1 = (matrix[0][0] | 0) + (matrix[1][1] | 0);
    REQUIRE_EQ(d1, 5);
    auto const d2 = (matrix[0][1] | 0) + (matrix[1][0] | 0);
    REQUIRE_EQ(d2, 5);
}

