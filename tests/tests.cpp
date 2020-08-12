//#ifndef _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//#endif


#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <confetti/confetti.hpp>



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
  REQUIRE(r.code == confetti::result::invalid_section_name);
  REQUIRE(r.source.line_no == 1);
}



TEST_CASE("parse outside sections") {

  confetti::result r = confetti::parse_string(" X = foo ");
  REQUIRE(!!r);
  REQUIRE(r.config.contains("default"));

  auto const& def = r.config["default"];
  REQUIRE(def.size() == 1);

  REQUIRE(def.contains("x"));
  auto const maybe_foo = def["x"].to<std::string>();
  REQUIRE(!!maybe_foo);
  REQUIRE(*maybe_foo == "foo");
  auto const foo = def["x"].either("");
  REQUIRE(foo == "foo");

  REQUIRE(!def.contains("y"));
  auto const maybe_nothing = def["y"].to<std::string>();
  REQUIRE(!maybe_nothing);
  auto const bar = def["y"].either("bar");
  REQUIRE(bar == "bar");
}



TEST_CASE("parse inside section") {

  confetti::result r = confetti::parse_string("[sectioN]\n" "keY = value");
  REQUIRE(!!r);
  REQUIRE(r.config.contains("section"));

  auto const& section = r.config["section"];
  REQUIRE(section.contains("key"));
  REQUIRE(section["key"].either("") == "value");

}



TEST_CASE("parse invalid configs") {
  confetti::result r = confetti::parse_string("key = 'foo' bar");
  REQUIRE(!r);
  REQUIRE(r.code == confetti::result::unexpected_chars_after_value);

  r = confetti::parse_string("k1 = v1 k2 = v2");
  REQUIRE(!r);
  REQUIRE(r.code == confetti::result::unexpected_equal_sign);

  r = confetti::parse_string("k1 = v1\n k1 = v2");
  REQUIRE(!r);
  REQUIRE(r.code == confetti::result::duplicated_item);

  r = confetti::parse_string("k1 = v1\n k2 = ");
  REQUIRE(!r);
  REQUIRE(r.code == confetti::result::expected_value);

}


TEST_CASE("parse boolean") {

  confetti::result r = confetti::parse_string(
                         "k1 = true\n" "k2 = false\n"
                         "k3 = on\n" "k4 = off\n"
                         "k5 = True\n" "k6 = False\n"
                         "k7 = ON\n" "k8 = OFF\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either(false));
  REQUIRE(!section["k2"].either(true));
  REQUIRE(section["k3"].either(false));
  REQUIRE(!section["k4"].either(true));
  REQUIRE(section["k5"].either(false));
  REQUIRE(!section["k6"].either(true));
  REQUIRE(section["k7"].either(false));
  REQUIRE(!section["k8"].either(true));
}


TEST_CASE("parse integer") {

  confetti::result r = confetti::parse_string(
                         "k1 = -2147483648\n" "k2 = +2147483647\n"
                         "k3 = 0xFCED\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either(-1) == -2147483647 - 1);
  REQUIRE(section["k2"].either(-1) == 2147483647);
  REQUIRE(section["k3"].either(-1) == -1);
}


TEST_CASE("parse unsigned") {

  confetti::result r = confetti::parse_string(
                         "k1 = 4294967295\n" "k2 = 0xFCED\n"
                         "k3 = 0x");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either(1u) == 4294967295);
  REQUIRE(section["k2"].either(1u) == 0xFCED);
  REQUIRE(section["k3"].either(1u) == 1u);
}


TEST_CASE("parse long integer") {

  confetti::result r = confetti::parse_string(
                         "k1 = -9223372036854775808\n"
                         "k2 = 9223372036854775807\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either(0ll) == -9223372036854775807 - 1);
  REQUIRE(section["k2"].either(0ll) == 9223372036854775807);
}


TEST_CASE("parse double") {

  confetti::result r = confetti::parse_string(
                         "k1 = -3.14E+2\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either(-1.) == -314.);
}



TEST_CASE("parse string") {

  confetti::result r = confetti::parse_string(
                         "k1 = \"\"\n" "k2 = ''\n"
                         "k3 = \"'foo'\"\n" "k4 = '\"bar\"'\n"
                         "k5 = foo' bar'\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either("foo") == "");
  REQUIRE(section["k2"].either("bar") == "");
  REQUIRE(section["k3"].either("") == "'foo'");
  REQUIRE(section["k4"].either("") == "\"bar\"");
  REQUIRE(section["k5"].either("") == "foo' bar'");
}



TEST_CASE("parse string view") {

  using namespace std::string_view_literals;

  confetti::result r = confetti::parse_string(
                         "k1 = \"\"\n" "k2 = ''\n"
                         "k3 = \"'foo'\"\n" "k4 = '\"bar\"'\n");
  REQUIRE(!!r);
  auto const& section = r.config.defaults();
  REQUIRE(section["k1"].either("foo"sv) == "");
  REQUIRE(section["k2"].either("bar"sv) == "");
  REQUIRE(section["k3"].either(""sv) == "'foo'");
  REQUIRE(section["k4"].either(""sv) == "\"bar\"");
}


TEST_CASE("parse array") {

  confetti::result r = confetti::parse_string(
                         "k1 = []\n" "k2 = [\n 1\n, 2\n]\n");

  REQUIRE(!!r);

  auto const& section = r.config["default"];
  std::vector<int> none = {-1};

  auto const k1 = section["k1"].either(none);
  REQUIRE(k1.empty());
  auto const k2 = section["k2"].either(none);
  REQUIRE(k2.size() == 2); REQUIRE(k2[0] == 1); REQUIRE(k2[1] == 2);
}


TEST_CASE("parse inline table") {

  confetti::result r = confetti::parse_string(
                         "k1 = {\n x = foo bar, y = [\n1,\n2]\n}\n");

  REQUIRE(!!r);
  auto const& table = r.config["default"]["k1"];
  REQUIRE(table["x"].either("") == "foo bar");
  auto const& array = table["y"];
  REQUIRE(array.size() == 2);
  REQUIRE(array[0].either(0) == 1);
  REQUIRE(array[1].either(1) == 2);
}


TEST_CASE("parse inline array of tables") {

  confetti::result r = confetti::parse_string(
                         "data = [\n{k = foo, v = bar},\n{k = bar, v = foo}]");

  REQUIRE(!!r);
  auto const& data = r.config["default"]["data"];
  REQUIRE(data.size() == 2);
  auto const& table1 = data[0];
  REQUIRE(table1["k"].either("") == "foo");
  REQUIRE(table1["v"].either("") == "bar");
  auto const& table2 = data[1];
  REQUIRE(table2["k"].either("") == "bar");
  REQUIRE(table2["v"].either("") == "foo");
}


TEST_CASE("parse array of tables") {

  confetti::result r = confetti::parse_string(
                         "[[data]]\n"
                         "k = foo\nv = bar\n"
                         "[[data]]\n"
                         "k = bar\nv = foo\n");

  REQUIRE(!!r);
  auto const& data = r.config["default"]["data"];
  REQUIRE(data.size() == 2);
  auto const& table1 = data[0];
  REQUIRE(table1["k"].either("") == "foo");
  REQUIRE(table1["v"].either("") == "bar");
  auto const& table2 = data[1];
  REQUIRE(table2["k"].either("") == "bar");
  REQUIRE(table2["v"].either("") == "foo");
}

