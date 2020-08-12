#pragma once


#include <memory>
#include <cstdio>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>
#include <optional>
#include <charconv>


#ifdef _MSC_VER
#pragma warning(disable:4996) // "unsafe" CRT functions
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



  inline void lower_case(char* head, char* tail) {
    for(; head != tail; ++head)
      *head = lower_case(*head);
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

  template<std::size_t N>
  value const& operator [](char const (&name)[N]) const noexcept {
    return operator [](std::string_view{name, N - 1});
  }

  value const& operator [](std::string_view const& name) const noexcept;


  size_type size() const noexcept;


  template<std::size_t N>
  bool contains(char const (&name)[N]) const noexcept {
    return contains(std::string_view{name, N - 1});
  }


  bool contains(std::string_view const& name) const noexcept;


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
  array& operator = (array&&) = default;

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
  table& operator = (table&&) = default;
  size_type size() const noexcept { return map_.size(); }
  bool empty() const noexcept { return map_.empty(); }


  template<std::size_t N>
  value const& operator [] (char const (&name)[N]) const noexcept {
    return operator [](std::string_view{name, N - 1});
  }


  value const& operator [] (std::string_view const& name) const noexcept {
    auto const found = map_.find(name);
    if(found == map_.end())
      return value::none();
    return found->second;
  }


  void put(std::string_view const& name, value value) {
    map_[name] = std::move(value);
  }


  array* find_array(std::string_view const& name) noexcept {
    auto const found = map_.find(name);
    if(found == map_.end()) {
      array_ptr new_array = std::make_unique<array>();
      array* p = new_array.get();
      map_[name] = std::move(new_array);
      return p;
    }
    return found->second.to_array();
  }


  template<std::size_t N>
  bool contains(char const (&name)[N]) const noexcept {
    return contains(std::string_view{name, N - 1});
  }


  bool contains(std::string_view const& name) const noexcept {
    return map_.find(name) != map_.end();
  }


private:

  using map_type = std::unordered_map<std::string_view, value>;

  map_type map_;

}; // table



inline value const& value::operator [](std::string_view const& name) const noexcept {
  table_ptr const* p = std::get_if<table_ptr>(&holder_);
  if(p == nullptr)
    return value::none();
  table const& table = *p->get();
  return table[name];
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


inline bool value::contains(std::string_view const& name) const noexcept {
  table_ptr const* p = std::get_if<table_ptr>(&holder_);
  if(p == nullptr)
    return false;
  table const& table = *p->get();
  return table.contains(name);
}



namespace detail { struct parser; }



struct config {

  friend struct detail::parser;

  config() = default;
  config(config const&) = delete;
  config& operator = (config const&) = delete;  
  config(config&& other) = default;
  config& operator = (config&&) = default;

  table const& defaults() const noexcept {
    return defaults_;
  }

  template<std::size_t N>
  bool contains(char const (&name)[N]) const noexcept {
    if (N == 8 && strcmp(name, "default") == 0)
      return true;
    return contains(std::string_view{name, N - 1});
  }

  template<std::size_t N>
  table const& operator [] (char const (&name)[N]) const noexcept {
    if (N == 8 && strcmp(name, "default") == 0)
      return defaults_;
    std::string_view key{name, N - 1};
    auto const found = sections_.find(key);
    if(found == sections_.end())
      return table::none();
    return *found->second;
  }


private:

  using sections_type = std::unordered_map<std::string_view, table_ptr>;

  sections_type sections_;
  std::unique_ptr<char[]> source_;
  table defaults_;

  void source(std::unique_ptr<char[]> source) {
    source_ = std::move(source);
  }

  void put(std::string_view const& name, table_ptr ptr) {
    sections_[name] = std::move(ptr);
  }

  bool contains(std::string_view const& name) const noexcept {
    return sections_.find(name) != sections_.end();
  }

  table* default_section() noexcept {
    return &defaults_;
  }


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
    ok, unable_to_read_file, invalid_section_name, invalid_item_name, expected_equal_sign,
    duplicated_item, unexpected_chars_after_value, expected_value, expected_quote,
    unexpected_equal_sign, expected_closing_array, expected_comma_or_closing_array,
    expected_closing_table, expected_comma_or_closing_table,
    expected_table_array_name, expected_closing_table_array, invalid_byte_order_mark
  }; // code

  enum code code{ok};
  char const* message{"Ok"};
  struct config config;
  source_line source;

  result() = default;
  result(result const&) = delete;
  result& operator = (result const&) = delete;
  result(result&&) = default;
  result& operator = (result&&) = default;
  explicit operator bool () const noexcept { return code == ok; }

  explicit result(struct config config):
    config{std::move(config)}
  { }


  result(enum code code, source_line const& source) noexcept:
    code{code}, message{message_for_code(code)}, source{source}
  { }


  template<typename charT, typename traits> friend
    std::basic_ostream<charT, traits>&
      operator << (std::basic_ostream<charT, traits>& os, result const& r) noexcept {
        os << r.message;
        return os;
      }

private:

  static char const* message_for_code(enum code code) noexcept {
    switch(code) {
      case ok:
        return "Ok";
      case unable_to_read_file:
        return "Unable to read file";
      case invalid_section_name:
        return "Invalid section name";
      case invalid_item_name:
        return "Invalid item name";
      case expected_equal_sign:
        return "Expected '=' after item name";
      case duplicated_item:
        return "Duplicated item name";
      case unexpected_chars_after_value:
        return "Unexpected characters after item value";
      case expected_value:
        return "Expected item value after '='";
      case expected_quote:
        return "Expected closing quote for string";
      case unexpected_equal_sign:
        return "Unexpected '=' in item value";
      case expected_closing_array:
        return "There is opened array, so expected ']'";
      case expected_comma_or_closing_array:
        return "There is opened array, so expected ',' or ']'";
      case expected_closing_table:
        return "There is opened inline table, so expected '}'";
      case expected_comma_or_closing_table:
        return "There is opened inline table, so expected ',' or '}'";
      case expected_table_array_name:
        return "Expected name of the table array inside [[ ]]";
      case expected_closing_table_array:
        return "Expected ']]' after name of the table array";
      case invalid_byte_order_mark:
        return "Invalid byte order mark, only UTF-8 is supported (0xEFBBBF)";
    }
    return "Unknown";
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
    table* current_section = config.default_section();

    if(!source)
      return result{std::move(config)};

    cursor_ = source.get();
    switch(*cursor_) {
      case char(0xEF):
        ++cursor_;
        if(*cursor_ != char(0xBB))
          return result{result::invalid_byte_order_mark, get_current_position()};
        ++cursor_;
        if(*cursor_ != char(0xBF))
          return result{result::invalid_byte_order_mark, get_current_position()};
        ++cursor_;
        break;
      case char(0xFE): case char(0xFF):
        return result{result::invalid_byte_order_mark, get_current_position()};
      default:
        break;
    }

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
              return result{result::invalid_section_name, get_current_position()};
          }
          continue;
        case '\0':
          return result{std::move(config)};
        default:
          if((rc = parse_key_value(*current_section)) != result::ok)
            return result{rc, get_current_position()};
          if(!skip_line())
            return result{result::unexpected_chars_after_value, get_current_position()};
          continue;
      }
  }



private:

  char* cursor_{nullptr};
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
        case ' ': case '\r': case '\t':
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
      return result::expected_table_array_name;

    char* head = cursor_;

    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char* tail = cursor_;

    skip_spaces();
    if(*cursor_++ != ']')
      return result::expected_closing_table_array;
    if(*cursor_++ != ']')
      return result::expected_closing_table_array;


    detail::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};

    array* p = section.find_array(name);
    if(p == nullptr)
      return result::duplicated_item;

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
            return result::unexpected_chars_after_value;
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

    char* head = cursor_++;
    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char* tail = cursor_;

    skip_spaces();
    if(*cursor_ != ']')
      return nullptr;

    detail::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};


    if(name != "default" && config.contains(name))
      return nullptr;

    ++cursor_;
    if(!skip_line())
      return nullptr;

    table_ptr section = std::make_unique<table>();
    table* current = section.get();

    config.put(name, std::move(section));
    return current;
  }



  enum result::code parse_key_value(table& section) {

    if(!is_letter(*cursor_))
      return result::invalid_item_name;

    char* head = cursor_;
    while(is_valid_for_identifier(*cursor_))
      ++cursor_;
    char* tail = cursor_;

    skip_spaces();
    if(*cursor_ != '=')
      return result::expected_equal_sign;

    detail::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};


    if(section.contains(name))
      return result::duplicated_item;

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
        return result::expected_value;
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
          return result::expected_quote;
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
        return result::expected_closing_array;

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
          return result::expected_comma_or_closing_array;
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
        return result::expected_closing_table;

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
          return result::expected_comma_or_closing_table;
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
          return result::unexpected_equal_sign;
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
  unique_ptr<FILE, int(*)(FILE*)> file{fopen(file_name, "rb"), fclose};
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
    printf("file size %d was read %zu\n", file_size, was_read);
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
    return result{result::unable_to_read_file, source_line{}};
  detail::parser p;
  return p.parse(std::move(source));
}


inline result parse(std::string const& file_name) {
  return parse(file_name.data());
}



}
