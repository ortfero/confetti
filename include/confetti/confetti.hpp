#pragma once



#include <memory>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>
#include <optional>
#include <charconv>
#include <chrono>
#include <iosfwd>
#include <ctime>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(disable:4996) // "Unsafe" CRT Library functions
#endif


namespace confetti {


struct array;
using array_ptr = std::unique_ptr<array>;



struct table;
using table_ptr = std::unique_ptr<table>;



namespace detail {



  inline char lower_case(char c) {
    switch(c) {
      case 'A': case 'B': case 'C': case 'D': case 'E':
      case 'F': case 'G': case 'H': case 'I': case 'J':
      case 'K': case 'L': case 'M': case 'N': case 'O':
      case 'P': case 'Q': case 'R': case 'S': case 'T':
      case 'U': case 'V': case 'W': case 'X': case 'Y':
      case 'Z':
        return c + 32;
      default:
        return c;
    }
  }



  inline std::string& lower_case(std::string& s) {
    for(auto& c: s)
      c = lower_case(c);
    return s;
  }



} // detail



struct value {

  using size_type = size_t;

  static value const& none() { static value x; return x; }

  value() noexcept = default;
  value(value const&) = delete;
  value& operator = (value const&) = delete;
  value(value&&) noexcept = default;
  value& operator = (value&&) noexcept = default;
  value(std::string_view const& sv) noexcept: holder_{sv} { }
  value(array_ptr p) noexcept: holder_{std::move(p)} { }
  value(table_ptr p) noexcept: holder_{std::move(p)} { }


  value& operator = (std::string_view const& sv) noexcept {
    holder_ = sv;
    return *this;
  }


  value& operator = (array_ptr p) noexcept {
    holder_ = std::move(p);
    return *this;
  }


  value& operator = (table_ptr p) noexcept {
    holder_ = std::move(p);
    return *this;
  }


  bool is_single_value() const noexcept {
    return std::holds_alternative<std::string_view>(holder_);
  }


  bool is_array() const noexcept {
    return std::holds_alternative<array_ptr>(holder_);
  }


  bool is_table() const noexcept {
    return std::holds_alternative<table_ptr>(holder_);
  }


  value const& operator [](size_type i) const noexcept;

  array* to_array() noexcept;

  value const& operator [](std::string name) const noexcept;


  size_type size() const noexcept;

  bool contains(std::string name) const noexcept;


  std::string either(char const* bydefault) const {
    auto const maybe = to<std::string>();
    if(!maybe)
      return std::string{bydefault};
    return *maybe;
  }


  template<typename T> T either(T const& bydefault) const {

    auto maybe = to<T>();
    if(!maybe)
      return bydefault;

    return *maybe;
  }


  template<typename T> std::optional<T> to() const = delete;


  template<> std::optional<bool> to() const {

    using detail::lower_case;
    std::optional<bool> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;
    bool boolean;
    char const* cc = p->data();

    switch(p->size()) {
      case 2:
        boolean = lower_case(cc[0]) == 'o'
            && lower_case(cc[1]) == 'n';
        return boolean ? (result = true) : result;
      case 3:
        boolean = lower_case(cc[0]) == 'o'
            && lower_case(cc[1]) == 'f'
            && lower_case(cc[2]) == 'f';
        return boolean ? (result = false) : result;
      case 4:
        boolean = lower_case(cc[0]) == 't'
            && lower_case(cc[1]) == 'r'
            && lower_case(cc[2]) == 'u'
            && lower_case(cc[3]) == 'e';
        return boolean ? (result = true) : result;
      case 5:
        boolean = lower_case(cc[0]) == 'f'
            && lower_case(cc[1]) == 'a'
            && lower_case(cc[2]) == 'l'
            && lower_case(cc[3]) == 's'
            && lower_case(cc[4]) == 'e';
        return boolean ? (result = false) : result;
      default:
        return result;
    }

  }


  template<> std::optional<int> to() const {
    return try_parse_signed<int>();
  }


  template<> std::optional<unsigned> to() const {
    return try_parse_unsigned<unsigned>();
  }


  template<> std::optional<long long> to() const {
    return try_parse_signed<long long>();
  }


  template<> std::optional<unsigned long long> to() const {
    return try_parse_unsigned<unsigned long long>();
  }


  template<> std::optional<double> to() const {

    std::optional<double> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    char const* head = p->data();
    char const* tail = p->data() + p->size();

    if(*head == '+')
      ++head;

    double number;
    auto const parsed = std::from_chars(head, tail, number);

    if(parsed.ec == std::errc::invalid_argument)
      return result;

    return result = number;
  }


  template<> std::optional<std::string> to() const {

    std::optional<std::string> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    return result = std::string{p->begin(), p->end()};
  }


  template<> std::optional<std::string_view> to() const {

    std::optional<std::string_view> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    return result = *p;
  }


  template<> std::optional<std::chrono::system_clock::time_point> to() const {

    std::optional<std::chrono::system_clock::time_point> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    unsigned year, month, day, date_scanned;

    if(std::sscanf(p->data(), "%u/%u/%u%n", &year, &month, &day, &date_scanned) != 3 &&
       std::sscanf(p->data(), "%u-%u-%u%n", &year, &month, &day, &date_scanned) != 3)
      return result;

    if(date_scanned > p->size())
      return result;

    char const* after_date = p->data() + date_scanned;
    unsigned hour = 0, minute = 0, second = 0, time_scanned;

    if(date_scanned != p->size()) {

      if(std::sscanf(after_date, "%u:%u:%u%n", &hour, &minute, &second, &time_scanned) != 3
         || date_scanned + time_scanned != p->size())
        return result;

    }

    struct tm tm = {int(second), int(minute), int(hour),
                  int(day), int(month - 1), int(year - 1900), 0, 0, 0};
    time_t const tp =
#if _WIN32
      _mkgmtime(&tm);
#else
      timegm(&tm);
#endif

    if(tp == -1)
      return result;

    return result = std::chrono::system_clock::from_time_t(tp);
  }



  template<> std::optional<std::chrono::system_clock::duration> to() const {

    std::optional<std::chrono::system_clock::duration> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    unsigned hour, minute, second, scanned;
    if(std::sscanf(p->data(), "%u:%u:%u%n", &hour, &minute, &second, &scanned) == 3 &&
       scanned == p->size()) {
      return result = std::chrono::seconds{hour * 3600 + minute * 60 + second};
    }

    unsigned long long count; char unit[16];
    if(std::sscanf(p->data(), "%llu %13s%n", &count, &unit, &scanned) == 2 &&
       scanned == p->size()) {
      if(strcmp(unit, "microseconds") == 0  || strcmp(unit, "microsecond") == 0)
        return result = std::chrono::microseconds{count};
      else if(strcmp(unit, "milliseconds") == 0  || strcmp(unit, "millisecond") == 0)
        return result = std::chrono::milliseconds{count};
      else if(strcmp(unit, "seconds") == 0 || strcmp(unit, "second") == 0)
        return result = std::chrono::seconds{count};
      else if(strcmp(unit, "minutes") == 0  || strcmp(unit, "minute") == 0)
        return result = std::chrono::minutes{count};
      else if(strcmp(unit, "hours") == 0 || strcmp(unit, "hour") == 0)
        return result = std::chrono::hours{count};
      else if(strcmp(unit, "days") == 0 || strcmp(unit, "day") == 0)
        return result = std::chrono::hours{count * 24};
      else if(strcmp(unit, "weeks") == 0 || strcmp(unit, "week") == 0)
        return result = std::chrono::hours{count * 24 * 7};
    }

    return result;
  }


  template<> std::optional<std::vector<bool>> to() const {
    return try_parse_array<bool>();
  }


  template<> std::optional<std::vector<int>> to() const {
    return try_parse_array<int>();
  }


  template<> std::optional<std::vector<unsigned>> to() const {
    return try_parse_array<unsigned>();
  }


  template<> std::optional<std::vector<long long>> to() const {
    return try_parse_array<long long>();
  }


  template<> std::optional<std::vector<unsigned long long>> to() const {
    return try_parse_array<unsigned long long>();
  }


  template<> std::optional<std::vector<double>> to() const {
    return try_parse_array<double>();
  }


  template<> std::optional<std::vector<std::string>> to() const {
    return try_parse_array<std::string>();
  }


  template<> std::optional<std::vector<std::string_view>> to() const {
    return try_parse_array<std::string_view>();
  }


  template<> std::optional<std::vector<std::chrono::system_clock::time_point>>
    to() const {
      return try_parse_array<std::chrono::system_clock::time_point>();
  }


  template<> std::optional<std::vector<std::chrono::system_clock::duration>>
    to() const {
      return try_parse_array<std::chrono::system_clock::duration>();
  }


private:

  std::variant<std::monostate, std::string_view, array_ptr, table_ptr> holder_;



  template <typename T> std::optional<std::vector<T>> try_parse_array() const;



  template<typename T> std::optional<T> try_parse_signed() const noexcept {

    std::optional<T> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    char const* head = p->data();
    char const* tail = p->data() + p->size();

    if(*head == '+')
        ++head;

    if(head == tail)
      return result;

    T number;
    auto const parsed = std::from_chars(head, tail, number);

    if(parsed.ec == std::errc::invalid_argument
       || parsed.ptr != tail)
      return result;

    return result = number;
  }



  template<typename T> std::optional<T> try_parse_unsigned() const noexcept {

    std::optional<T> result;

    std::string_view const* p = std::get_if<std::string_view>(&holder_);
    if(p == nullptr)
      return result;

    char const* head = p->data();
    char const* tail = p->data() + p->size();
    int radix = 10;

    switch(*head) {
      case '+': case '-':
        return result;
      case '0':
        if(head[1] != 'x')
          break;
        if(tail - head < 3)
          return result;
        head += 2;
        radix = 16;
        break;
      default:
        break;
    }

    T number;
    auto const parsed = std::from_chars(head, tail, number, radix);

    if(parsed.ec == std::errc::invalid_argument
       || parsed.ptr != tail)
      return result;

    return result = number;
  }



}; // value



struct array: std::vector<value> {

  using base = std::vector<value>;
  using size_type = value::size_type;

  static array const& none() { static array x; return x; }

  array() = default;
  array(array const&) = delete;
  array& operator = (array const&) = delete;
  array(array&&) = default;
  array& operator = (array&&) noexcept = default;

  value const& operator [](size_type i) const noexcept {
    if(i >= base::size())
      return value::none();
    return base::operator[](i);
  }


}; // array



inline value const& value::operator [](size_type i) const noexcept {
  array_ptr const* p = std::get_if<array_ptr>(&holder_);
  if(p == nullptr)
    return value::none();
  array const& array = *p->get();
  return array[i];
}


inline array* value::to_array() noexcept {
  array_ptr const* p = std::get_if<array_ptr>(&holder_);
  if(p == nullptr)
    return nullptr;
  return p->get();
}


template<typename T> std::optional<std::vector<T>> value::try_parse_array() const {

  std::optional<std::vector<T>> result;

  array_ptr const* p = std::get_if<array_ptr>(&holder_);
  if(p == nullptr)
    return result;

  std::vector<T> array;
  array.reserve((*p)->size());

  for(value const& each: **p) {
    std::optional<T> const parsed = each.to<T>();
    if(!parsed)
      return result;
    array.emplace_back(std::move(*parsed));
  }

  return result = std::move(array);
}



struct table {

  using size_type = size_t;

  static table const& none() { static table x; return x; }

  table() = default;
  table(table const&) = delete;
  table& operator = (table const&) = delete;
  table(table&&) = default;
  table& operator = (table&&) noexcept = default;
  size_type size() const noexcept { return map_.size(); }
  bool empty() const noexcept { return map_.empty(); }

  value const& operator [] (std::string name) const noexcept {
    auto const found = map_.find(detail::lower_case(name));
    if(found == map_.end())
      return value::none();
    return found->second;
  }


  void put(std::string const& name, value value) {
    map_[name] = std::move(value);
  }


  array* find_array(std::string const& name) noexcept {
    auto const found = map_.find(name);
    if(found == map_.end()) {
      array_ptr new_array = std::make_unique<array>();
      array* p = new_array.get();
      map_[name] = std::move(new_array);
      return p;
    }
    return found->second.to_array();
  }


  bool contains(std::string name) const noexcept {
    return map_.find(detail::lower_case(name)) != map_.end();
  }


private:

  std::unordered_map<std::string, value> map_;

}; // table



inline value const& value::operator [](std::string name) const noexcept {
  table_ptr const* p = std::get_if<table_ptr>(&holder_);
  if(p == nullptr)
    return value::none();
  table const& table = *p->get();
  return table[std::move(name)];
}


inline value::size_type value::size() const noexcept {
  array_ptr const* ap = std::get_if<array_ptr>(&holder_);
  if(ap != nullptr)
    return ap->get()->size();
  table_ptr const* tp = std::get_if<table_ptr>(&holder_);
  if(tp != nullptr)
    return tp->get()->size();
  return 0;
}


inline bool value::contains(std::string name) const noexcept {
  table_ptr const* p = std::get_if<table_ptr>(&holder_);
  if(p == nullptr)
    return false;
  table const& table = *p->get();
  return table.contains(std::move(name));
}




struct config {

  config() = default;
  config(config const&) = delete;
  config& operator = (config const&) = delete;
  config(config&&) = default;
  config& operator = (config&&) noexcept = default;

  void source(std::unique_ptr<char[]> source) {
    source_ = std::move(source);
  }

  void section(std::string name, table_ptr ptr) {
    sections_[detail::lower_case(name)] = std::move(ptr);
  }

  bool contains(std::string s) const noexcept {
    return sections_.find(detail::lower_case(s)) != sections_.end();
  }

  table const& operator [] (std::string name) const noexcept {
    auto const found = sections_.find(detail::lower_case(name));
    if(found == sections_.end())
      return table::none();
    return *found->second;
  }


private:

  std::unordered_map<std::string, table_ptr> sections_;
  std::unique_ptr<char[]> source_;
}; // config



struct source_line {
  source_line() noexcept = default;
  source_line(source_line const&) noexcept = default;
  source_line& operator = (source_line const&) = default;

  source_line(std::string_view const& text, unsigned line_no,
              unsigned column_no) noexcept:
    text{text}, line_no{line_no}, column_no{column_no}
  { }

  std::string_view text;
  unsigned line_no{0};
  unsigned column_no{0};

}; // source_line



struct result {

  enum code {
    ok, unableToReadFile, invalidSectionName, invalidItemName, expectedEqualSign,
    duplicatedItem, unexpectedCharsAfterValue, expectedValue, expectedQuote,
    unexpectedEqualSign, expectedClosingArray, expectedCommaOrClosingArray,
    expectedClosingTable, expectedCommaOrClosingTable,
    expectedTableArrayName, expectedClosingTableArray
  }; // code

  enum code code{ok};
  struct config config;
  source_line source;

  result() = default;
  result(result const&) = delete;
  result& operator = (result const&) = delete;
  result(result&&) = default;
  result& operator = (result&&) = default;
  explicit operator bool () const noexcept { return code == ok; }

  explicit result(struct config config):
    code{ok}, config{std::move(config)}
  { }

  result(enum code code, source_line const& source) noexcept:
    code{code}, source{source}
  { }



  char const* message() const noexcept {
    switch(code) {
      case ok:
        return "Ok";
      case unableToReadFile:
        return "Unable to read file";
      case invalidSectionName:
        return "Invalid section name";
      case invalidItemName:
        return "Invalid item name";
      case expectedEqualSign:
        return "Expected '=' after item name";
      case duplicatedItem:
        return "Duplicated item name";
      case unexpectedCharsAfterValue:
        return "Unexpected characters after item value";
      case expectedValue:
        return "Expected item value after '='";
      case expectedQuote:
        return "Expected closing quote for string";
      case unexpectedEqualSign:
        return "Unexpected '=' in item value";
      case expectedClosingArray:
        return "There is opened array, so expected ']'";
      case expectedCommaOrClosingArray:
        return "There is opened array, so expected ',' or ']'";
      case expectedClosingTable:
        return "There is opened inline table, so expected '}'";
      case expectedCommaOrClosingTable:
        return "There is opened inline table, so expected ',' or '}'";
      case expectedTableArrayName:
        return "Expected name of the table array inside [[ ]]";
      case expectedClosingTableArray:
        return "Expected ']]' after name of the table array";
    }
    return "Unknown";
  }


  template<typename charT, typename traits> friend
    std::basic_ostream<charT, traits>&
      operator << (std::basic_ostream<charT, traits>& os, result const& r) noexcept {
        os << r.message();
        return os;
      }

}; // result



namespace detail {



struct parser {

  parser() = default;
  parser(parser const&) = delete;
  parser& operator = (parser const&) = delete;
  parser(parser&&) = default;
  parser& operator = (parser&&) noexcept = default;



  result parse(std::unique_ptr<char[]> source) {

    config config;
    table_ptr default_section = std::make_unique<table>();
    table* current_section = default_section.get();
    config.section("default", std::move(default_section));

    if(!source)
      return result{std::move(config)};

    cursor_ = source.get();
    config.source(std::move(source));
    get_current_line();

    enum result::code rc;

    for(;;)
      switch (*cursor_) {
        case ' ': case '\t': case '\r': case '\n':
          skip_spaces_and_lines(); continue;
        case '#': case ';':
          skip_comment(); continue;
        case '[':
          if(cursor_[1] == '[') {
            rc = parse_table_array_item(*current_section);
            if(rc != result::ok)
              return result{rc, get_current_position()};
          } else {
            current_section = parse_section_name(config);
            if(!current_section)
              return result{result::invalidSectionName, get_current_position()};
          }
          continue;
        case '\0':
          return result{std::move(config)};
        default:
          if((rc = parse_key_value(*current_section)) != result::ok)
            return result{rc, get_current_position()};
          if(!skip_line())
            return result{result::unexpectedCharsAfterValue, get_current_position()};
          continue;
      }
  }



private:

  char const* cursor_{nullptr};
  source_line line_;


  static std::string_view view(char const* from, char const* to) {
    return std::string_view{from, size_t(to - from)};
  }



  static bool is_eol(char c) {
    switch(c) {
      case '\r': case '\n': case '\0':
        return true;
      default:
        return false;
    }
  }



  static bool is_space(char c) {
    switch(c) {
      case ' ': case '\t': case '\r': case '\n': case '\0':
        return true;
      default:
        return false;
    }
  }



  static bool is_comment_starts(char c) {
    switch(c) {
      case '#': case ';':
        return true;
      default:
        return false;
    }
  }



  static bool is_letter(char c) {
    switch(c) {
      case 'A': case 'B': case 'C': case 'D': case 'E':
      case 'F': case 'G': case 'H': case 'I': case 'J':
      case 'K': case 'L': case 'M': case 'N': case 'O':
      case 'P': case 'Q': case 'R': case 'S': case 'T':
      case 'U': case 'V': case 'W': case 'X': case 'Y':
      case 'Z':
      case 'a': case 'b': case 'c': case 'd': case 'e':
      case 'f': case 'g': case 'h': case 'i': case 'j':
      case 'k': case 'l': case 'm': case 'n': case 'o':
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'w': case 'x': case 'y':
      case 'z':
      case '_':
        return true;
      default:
        return false;
    }
  }


  static bool is_valid_for_identifier(char c) {
    switch(c) {
      case 'A': case 'B': case 'C': case 'D': case 'E':
      case 'F': case 'G': case 'H': case 'I': case 'J':
      case 'K': case 'L': case 'M': case 'N': case 'O':
      case 'P': case 'Q': case 'R': case 'S': case 'T':
      case 'U': case 'V': case 'W': case 'X': case 'Y':
      case 'Z':
      case 'a': case 'b': case 'c': case 'd': case 'e':
      case 'f': case 'g': case 'h': case 'i': case 'j':
      case 'k': case 'l': case 'm': case 'n': case 'o':
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'w': case 'x': case 'y':
      case 'z':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      case '_': case '.':
        return true;
      default:
        return false;
    }
  }



  void skip_spaces_and_lines() {
    for(;;)
      switch(*cursor_) {
        case ' ': case '\t': case '\r':
          ++cursor_; continue;
        case '\n':
          ++cursor_; get_current_line(); continue;
        default:
          return;
      }
  }



  void skip_spaces() {
    for(;;)
      switch(*cursor_) {
        case ' ': case '\t':
          ++cursor_; continue;
        default:
          return;
      }
  }



  void skip_comment() {
    ++cursor_;
    for(;;)
      switch (*cursor_) {
        case '\n':
          ++cursor_; get_current_line();
          skip_spaces_and_lines();
          return;
        case '\0':
          return;
        default:
          ++cursor_; continue;

      }
  }


  void skip_lines_and_comments() {
    for(;;)
      switch(*cursor_) {
        case ' ': case '\t': case '\r':
          ++cursor_; continue;
        case '\n':
          ++cursor_; get_current_line();
          skip_spaces_and_lines();
          continue;
        case ';': case '#':
          skip_comment();
        default:
          return;
      }
  }


  bool skip_line() {
    skip_spaces();
    if(is_comment_starts(*cursor_))
      skip_comment();
    switch(*cursor_) {
      case '\n':
        skip_spaces_and_lines();
        return true;
      case '\0':
        return true;
      default:
        return false;
    }
  }



  void get_current_line() {
    ++line_.line_no;
    char const* tail = cursor_;
    while(!is_eol(*tail))
          ++tail;
    line_.text = view(cursor_, tail);
  }



  source_line get_current_position() const {
    source_line line{line_};
    line.column_no = unsigned(cursor_ - line.text.data()) + 1;
    return line;
  }



  enum result::code parse_table_array_item(table& section) {

    cursor_ += 2;
    skip_spaces();
    if(!is_letter(*cursor_))
      return result::expectedTableArrayName;

    char const* head = cursor_;

    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char const* tail = cursor_;

    skip_spaces();
    if(*cursor_++ != ']')
      return result::expectedClosingTableArray;
    if(*cursor_++ != ']')
      return result::expectedClosingTableArray;


    auto name = std::string{head, tail};
    detail::lower_case(name);

    array* p = section.find_array(name);
    if(p == nullptr)
      return result::duplicatedItem;

    table_ptr new_table = std::make_unique<table>();
    enum result::code rc;

    bool end_of_table = false;
    while(!end_of_table) {
      skip_lines_and_comments();

      switch (*cursor_) {
        case '[': case '\0':
          end_of_table = true;
          continue;
        default:
          rc = parse_key_value(*new_table);
          if(rc != result::ok)
            return rc;
          if(!skip_line())
            return result::unexpectedCharsAfterValue;
          continue;
      }
    }

    p->emplace_back(value{std::move(new_table)});
    return result::ok;
  }


  table* parse_section_name(config& config) {

    ++cursor_;
    skip_spaces();

    if(!is_letter(*cursor_))
      return nullptr;

    char const* head = cursor_++;
    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char const* tail = cursor_;

    skip_spaces();
    if(*cursor_ != ']')
      return nullptr;

    auto name = std::string{head, tail};
    lower_case(name);

    if(name != "default" && config.contains(name))
      return nullptr;

    ++cursor_;
    if(!skip_line())
      return nullptr;

    table_ptr section = std::make_unique<table>();
    table* current = section.get();

    config.section(name, std::move(section));
    return current;
  }



  enum result::code parse_key_value(table& section) {

    if(!is_letter(*cursor_))
      return result::invalidItemName;

    char const* head = cursor_;
    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char const* tail = cursor_;

    skip_spaces();
    if(*cursor_ != '=')
      return result::expectedEqualSign;

    auto name = std::string{head, tail};
    detail::lower_case(name);

    if(section.contains(name))
      return result::duplicatedItem;

    ++cursor_;

    value v;
    enum result::code const parsed = parse_value(v);
    if(parsed != result::ok)
      return parsed;

    section.put(name, std::move(v));

    return result::ok;
  }



  enum result::code parse_value(value& v) {

    skip_spaces();

    switch(*cursor_) {
      case '\"': case '\'':
        return parse_string(v);
      case '[':
        return parse_array(v);
      case '{':
        return parse_inline_table(v);
      case '\n': case '\0':
      case '#': case ';':
        return result::expectedValue;
      default:
        return parse_sentense(v);
    }
  }



  enum result::code parse_string(value& v) {

    char const quote = *cursor_;
    char const* head = ++cursor_;

    bool eos = false;

    while(!eos)
      switch(*cursor_) {
        case '\'': case '\"':
          eos = (*cursor_ == quote);
          if(eos)
            continue;
          ++cursor_;
          continue;
        case '\r': case '\n': case '\0':
          return result::expectedQuote;
        default:
          ++cursor_;
          continue;
      }

    v = view(head, cursor_);
    ++cursor_;

    return result::ok;
  }



  enum result::code parse_array(value& v) {

    array_ptr new_array = std::make_unique<array>();

    ++cursor_;
    skip_lines_and_comments();
    if(*cursor_ == ']') {
      ++cursor_;
      v = std::move(new_array);
      return result::ok;
    }

    bool end_of_array = false;
    value item;

    while(!end_of_array) {

      if(*cursor_ == '\0')
        return result::expectedClosingArray;

      enum result::code const rc = parse_value(item);
      if(rc != result::ok)
        return rc;

      new_array->emplace_back(std::move(item));

      skip_lines_and_comments();
      switch(*cursor_) {
        case ',':
          ++cursor_; skip_lines_and_comments();
          continue;
        case ']':
          ++cursor_; end_of_array = true;
          continue;
        default:
          return result::expectedCommaOrClosingArray;
      }

    }

    v = std::move(new_array);

    return result::ok;
  }



  enum result::code parse_inline_table(value& v) {

    table_ptr new_table = std::make_unique<table>();

    ++cursor_;
    skip_lines_and_comments();
    if(*cursor_ == '}') {
      ++cursor_;
      v = std::move(new_table);
      return result::ok;
    }

    bool end_of_table = false;

    while(!end_of_table) {

      if(*cursor_ == '\0')
        return result::expectedClosingTable;

      enum result::code const rc = parse_key_value(*new_table);
      if(rc != result::ok)
        return rc;

      skip_lines_and_comments();
      switch(*cursor_) {
        case ',':
          ++cursor_; skip_lines_and_comments();
          continue;
        case '}':
          ++cursor_; end_of_table = true;
          continue;
        default:
          return result::expectedCommaOrClosingTable;
      }

    }

    v = std::move(new_table);

    return result::ok;

  }



  enum result::code parse_sentense(value& v) {

    char const* head = cursor_++;
    char const* last = head;

    bool end = false;
    while(!end)
      switch (*cursor_) {
        case ' ': case '\t':
          ++cursor_;
          continue;
        case '\r': case '\n':
        case '#': case ';': case ',':
        case '[': case ']':
        case '{': case '}':
          end = true;
          continue;
        case '=':
          return result::unexpectedEqualSign;
        case '\0':
          end = true;
          continue;
        default:
          last = cursor_;
          ++cursor_;
          continue;
      }

    v = view(head, last + 1);

    return result::ok;
  }

}; // parser



inline std::unique_ptr<char[]> read_file(char const* file_name) {
  using namespace std;
  unique_ptr<char[]> source;
  unique_ptr<FILE, int(*)(FILE*)> file{fopen(file_name, "r"), fclose};
  if(!file)
    return source;
  fseek(file.get(), 0, SEEK_END);
  auto const file_size = ftell(file.get());
  if(file_size == -1L)
    return source;
  source = make_unique<char[]>(file_size + 1);
  fseek(file.get(), 0, SEEK_SET);
  size_t const was_read = fread(source.get(), 1, size_t(file_size), file.get());
  if(was_read != size_t(file_size)) {
    source.reset();
    return source;
  }
  source[file_size] = '\0';
  return source;
}



} // detail



inline result parse_string(char const* source) {
  detail::parser p;
  size_t const n = source == nullptr ? 0 : strlen(source);
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(n + 1);
  memcpy(buffer.get(), source, n);
  buffer[n] = '\0';
  return p.parse(std::move(buffer));
}



inline result parse(char const* file_name) {
  using namespace std;
  std::unique_ptr<char[]> source = detail::read_file(file_name);
  if(!source)
    return result{result::unableToReadFile, source_line{}};
  detail::parser p;
  return p.parse(std::move(source));
}


}
