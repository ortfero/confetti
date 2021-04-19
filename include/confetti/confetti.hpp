#pragma once

#include <charconv>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4996) // "unsafe" CRT functions
#endif

namespace confetti {

namespace detail::ascii {

inline char lower_case(char c) {
  switch (c) {
  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
  case 'G':
  case 'H':
  case 'I':
  case 'J':
  case 'K':
  case 'L':
  case 'M':
  case 'N':
  case 'O':
  case 'P':
  case 'Q':
  case 'R':
  case 'S':
  case 'T':
  case 'U':
  case 'V':
  case 'W':
  case 'X':
  case 'Y':
  case 'Z':
    return c + 32;
  default:
    return c;
  }
}

inline void lower_case(char *head, char *tail) {
  for (; head != tail; ++head)
    *head = lower_case(*head);
}

} // namespace detail::ascii

class value {
public:
  using size_type = size_t;
  using array = std::vector<value>;
  using array_ptr = std::unique_ptr<array>;
  using table = std::unordered_map<std::string_view, value>;
  using table_ptr = std::unique_ptr<table>;

  static value const none;

  static value make_array() { return value{std::make_unique<array>()}; }

  static value make_table() { return value{std::make_unique<table>()}; }

  value() noexcept = default;
  value(value const &) = delete;
  value &operator=(value const &) = delete;
  value(value &&) noexcept = default;
  value &operator=(value &&) noexcept = default;
  value(std::string_view const &sv) noexcept : holder_{sv} {}
  value(array_ptr p) noexcept : holder_{std::move(p)} {}
  value(table_ptr p) noexcept : holder_{std::move(p)} {}

  value &operator=(std::string_view const &sv) noexcept {
    holder_ = sv;
    return *this;
  }

  value &operator=(array_ptr p) noexcept {
    holder_ = std::move(p);
    return *this;
  }

  value &operator=(table_ptr p) noexcept {
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

  value const &operator[](size_type i) const noexcept {
    array_ptr const *p = std::get_if<array_ptr>(&holder_);
    if (p == nullptr)
      return value::none;
    array const &array = *p->get();
    return array[i];
  }

  template <std::size_t N>
  value const &operator[](char const (&name)[N]) const noexcept {
    return operator[](std::string_view{name, N - 1});
  }

  bool emplace_back(value &&v) noexcept {
    try {
      array_ptr const *p = std::get_if<array_ptr>(&holder_);
      if (p == nullptr)
        return false;
      array &array = *p->get();
      array.emplace_back(std::move(v));
      return true;
    } catch (std::bad_alloc const &) {
      return false;
    }
  }

  value const &operator[](std::string_view const &name) const noexcept {
    table_ptr const *p = std::get_if<table_ptr>(&holder_);
    if (p == nullptr)
      return value::none;
    table const &table = *p->get();
    auto const found = table.find(name);
    if (found == table.end())
      return value::none;
    return found->second;
  }

  size_type size() const noexcept {
    array_ptr const *ap = std::get_if<array_ptr>(&holder_);
    if (ap != nullptr)
      return ap->get()->size();
    table_ptr const *tp = std::get_if<table_ptr>(&holder_);
    if (tp != nullptr)
      return tp->get()->size();
    return 0;
  }

  template <std::size_t N> bool contains(char const (&name)[N]) const noexcept {
    return contains(std::string_view{name, N - 1});
  }

  bool contains(std::string_view const &name) const noexcept {
    table_ptr const *p = std::get_if<table_ptr>(&holder_);
    if (p == nullptr)
      return false;
    table const &table = *p->get();
    auto const found = table.find(name);
    return found != table.end();
  }

  value *insert(std::string_view const &name, value &&value) {
    table_ptr const *p = std::get_if<table_ptr>(&holder_);
    if (p == nullptr)
      return nullptr;
    table &table = *p->get();
    auto const found = table.find(name);
    if (found != table.end())
      return nullptr;
    auto emplaced = table.try_emplace(name, std::move(value));
    if (!emplaced.second)
      return nullptr;
    return &emplaced.first->second;
  }

  template <std::size_t N> value *find(char const (&name)[N]) noexcept {
    return find(std::string_view{name, N - 1});
  }

  value *find(std::string_view const &name) noexcept {
    table_ptr const *p = std::get_if<table_ptr>(&holder_);
    if (p == nullptr)
      return nullptr;
    table &table = *p->get();
    auto const found = table.find(name);
    if (found == table.end())
      return nullptr;
    return &found->second;
  }

  template <typename T> std::optional<T> to() const = delete;

private:
  std::variant<std::monostate, std::string_view, array_ptr, table_ptr> holder_;

  template <typename T> std::optional<std::vector<T>> try_parse_array() const;

  template <typename T> std::optional<T> try_parse_signed() const noexcept {

    std::optional<T> result;

    std::string_view const *p = std::get_if<std::string_view>(&holder_);
    if (p == nullptr)
      return result;

    char const *head = p->data();
    char const *tail = p->data() + p->size();

    if (*head == '+')
      ++head;

    if (head == tail)
      return result;

    T number;
    auto const parsed = std::from_chars(head, tail, number);

    if (parsed.ec == std::errc::invalid_argument || parsed.ptr != tail)
      return result;

    return result = number;
  }

  template <typename T> std::optional<T> try_parse_unsigned() const noexcept {

    std::optional<T> result;

    std::string_view const *p = std::get_if<std::string_view>(&holder_);
    if (p == nullptr)
      return result;

    char const *head = p->data();
    char const *tail = p->data() + p->size();
    int radix = 10;

    switch (*head) {
    case '+':
    case '-':
      return result;
    case '0':
      if (head[1] != 'x')
        break;
      if (tail - head < 3)
        return result;
      head += 2;
      radix = 16;
      break;
    default:
      break;
    }

    T number;
    auto const parsed = std::from_chars(head, tail, number, radix);

    if (parsed.ec == std::errc::invalid_argument || parsed.ptr != tail)
      return result;

    return result = number;
  }

}; // value

inline value const value::none;

template <> std::optional<bool> value::to() const {
  using detail::ascii::lower_case;
  std::optional<bool> result;

  std::string_view const *p = std::get_if<std::string_view>(&holder_);
  if (p == nullptr)
    return result;
  bool boolean;
  char const *cc = p->data();

  switch (p->size()) {
  case 2:
    boolean = lower_case(cc[0]) == 'o' && lower_case(cc[1]) == 'n';
    return boolean ? (result = true) : result;
  case 3:
    boolean = lower_case(cc[0]) == 'o' && lower_case(cc[1]) == 'f' &&
              lower_case(cc[2]) == 'f';
    return boolean ? (result = false) : result;
  case 4:
    boolean = lower_case(cc[0]) == 't' && lower_case(cc[1]) == 'r' &&
              lower_case(cc[2]) == 'u' && lower_case(cc[3]) == 'e';
    return boolean ? (result = true) : result;
  case 5:
    boolean = lower_case(cc[0]) == 'f' && lower_case(cc[1]) == 'a' &&
              lower_case(cc[2]) == 'l' && lower_case(cc[3]) == 's' &&
              lower_case(cc[4]) == 'e';
    return boolean ? (result = false) : result;
  default:
    return result;
  }
}

template <> std::optional<int> value::to() const {
  return try_parse_signed<int>();
}

template <> std::optional<unsigned> value::to() const {
  return try_parse_unsigned<unsigned>();
}

template <> std::optional<long long> value::to() const {
  return try_parse_signed<long long>();
}

template <> std::optional<unsigned long long> value::to() const {
  return try_parse_unsigned<unsigned long long>();
}

template <> std::optional<double> value::to() const {
  std::optional<double> result;

  std::string_view const *p = std::get_if<std::string_view>(&holder_);
  if (p == nullptr)
    return result;

  char const *head = p->data();
  char const *tail = p->data() + p->size();

  if (*head == '+')
    ++head;

#ifdef _MSC_VER
  double number;
  auto const parsed = std::from_chars(head, tail, number);

  if (parsed.ec == std::errc::invalid_argument)
    return result;
#else
  char *endptr;
  double number = std::strtod(head, &endptr);
  if (endptr != tail)
    return result;
#endif

  return result = number;
}

template <> std::optional<std::string> value::to() const {
  std::optional<std::string> result;

  std::string_view const *p = std::get_if<std::string_view>(&holder_);
  if (p == nullptr)
    return result;

  return result = std::string{p->begin(), p->end()};
}

template <> std::optional<std::string_view> value::to() const {
  std::optional<std::string_view> result;

  std::string_view const *p = std::get_if<std::string_view>(&holder_);
  if (p == nullptr)
    return result;

  return result = *p;
}

template <> std::optional<std::vector<bool>> value::to() const {
  return try_parse_array<bool>();
}

template <> std::optional<std::vector<int>> value::to() const {
  return try_parse_array<int>();
}

template <> std::optional<std::vector<unsigned>> value::to() const {
  return try_parse_array<unsigned>();
}

template <> std::optional<std::vector<long long>> value::to() const {
  return try_parse_array<long long>();
}

template <> std::optional<std::vector<unsigned long long>> value::to() const {
  return try_parse_array<unsigned long long>();
}

template <> std::optional<std::vector<double>> value::to() const {
  return try_parse_array<double>();
}

template <> std::optional<std::vector<std::string>> value::to() const {
  return try_parse_array<std::string>();
}

template <> std::optional<std::vector<std::string_view>> value::to() const {
  return try_parse_array<std::string_view>();
}

std::string operator|(value const &v, char const *bydefault) {
  auto const maybe = v.to<std::string>();
  if (!maybe)
    return std::string{bydefault};
  return *maybe;
}

template <typename T> T operator|(value const &v, T const &bydefault) {
  auto maybe = v.to<T>();
  if (!maybe)
    return bydefault;
  return *maybe;
}

template <typename T>
std::optional<std::vector<T>> value::try_parse_array() const {

  std::optional<std::vector<T>> result;

  array_ptr const *p = std::get_if<array_ptr>(&holder_);
  if (p == nullptr)
    return result;

  std::vector<T> array;
  array.reserve((*p)->size());

  for (value const &each : **p) {
    std::optional<T> const parsed = each.to<T>();
    if (!parsed)
      return result;
    array.emplace_back(std::move(*parsed));
  }

  return result = std::move(array);
}

namespace detail {
class parser;
}

enum struct error {

  ok,
  unable_to_read_file,
  invalid_section_name,
  invalid_item_name,
  expected_equal_sign,
  duplicated_item,
  unexpected_chars_after_value,
  expected_value,
  expected_quote,
  unexpected_equal_sign,
  expected_closing_array,
  expected_comma_or_closing_array,
  expected_closing_table,
  expected_comma_or_closing_table,
  expected_table_array_name,
  expected_closing_table_array,
  invalid_byte_order_mark

};

struct error_category : std::error_category {

  char const *name() const noexcept override { return "confetti"; }

  std::string message(int code) const noexcept override {
    switch (error(code)) {
    case error::ok:
      return "Ok";
    case error::unable_to_read_file:
      return "Unable to read file";
    case error::invalid_section_name:
      return "Invalid section name";
    case error::invalid_item_name:
      return "Invalid item name";
    case error::expected_equal_sign:
      return "Expected '=' after item name";
    case error::duplicated_item:
      return "Duplicated item name";
    case error::unexpected_chars_after_value:
      return "Unexpected characters after item value";
    case error::expected_value:
      return "Expected item value after '='";
    case error::expected_quote:
      return "Expected closing quote for string";
    case error::unexpected_equal_sign:
      return "Unexpected '=' in item value";
    case error::expected_closing_array:
      return "There is opened array, so expected ']'";
    case error::expected_comma_or_closing_array:
      return "There is opened array, so expected ',' or ']'";
    case error::expected_closing_table:
      return "There is opened inline table, so expected '}'";
    case error::expected_comma_or_closing_table:
      return "There is opened inline table, so expected ',' or '}'";
    case error::expected_table_array_name:
      return "Expected name of the table array inside [[ ]]";
    case error::expected_closing_table_array:
      return "Expected ']]' after name of the table array";
    case error::invalid_byte_order_mark:
      return "Invalid byte order mark, only UTF-8 is supported (0xEFBBBF)";
    default:
      return "Unknown";
    }
  }
};

inline error_category const confetti_category;

} // namespace confetti

namespace std {

template <> struct is_error_code_enum<confetti::error> : true_type {};

} // namespace std

namespace confetti {

inline std::error_code make_error_code(error e) noexcept {
  return {int(e), confetti_category};
}

struct result {

  std::unique_ptr<char[]> source;
  std::system_error error;
  unsigned line_no{0};
  value config;

  result() = default;
  result(result const &) = delete;
  result &operator=(result const &) = delete;
  result(result &&) = default;
  result &operator=(result &&) = default;
  explicit operator bool() const noexcept { return !error.code(); }

  explicit result(enum error ec) : error{make_error_code(ec)} {}

  explicit result(std::unique_ptr<char[]> source)
      : source{std::move(source)}, error{make_error_code(error::ok)},
        config{value::make_table()} {
    config.insert("default", value::make_table());
  }

  template <typename Stream>
  friend Stream &operator<<(Stream &os, result const &r) noexcept {
    os << r.error.what();
    return os;
  }
}; // result

namespace detail {

inline bool failed(result &r, error e) {
  r.error = make_error_code(e);
  return false;
}

class parser {
public:
  parser() = default;
  parser(parser const &) = delete;
  parser &operator=(parser const &) = delete;
  parser(parser &&) = default;
  parser &operator=(parser &&) noexcept = default;

  result parse(std::unique_ptr<char[]> source) {

    result r{std::move(source)};
    if (!r.source)
      return r;

    value *current_section = r.config.find("default");
    if (current_section == nullptr || !current_section->is_table())
      return r;

    cursor_ = r.source.get();
    r.line_no = 1;
    using uchar = unsigned char;
    switch (uchar(*cursor_)) {
    case uchar(0xEF):
      ++cursor_;
      if (uchar(*cursor_) != uchar(0xBB))
        return r.error = make_error_code(error::invalid_byte_order_mark),
               std::move(r);
      ++cursor_;
      if (uchar(*cursor_) != uchar(0xBF))
        return r.error = make_error_code(error::invalid_byte_order_mark),
               std::move(r);
      ++cursor_;
      break;
    case uchar(0xFE):
    case uchar(0xFF):
      return r.error = make_error_code(error::invalid_byte_order_mark),
             std::move(r);
    default:
      break;
    }

    for (;;)
      switch (*cursor_) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        skip_spaces_and_lines(r);
        continue;
      case '#':
      case ';':
        skip_comment(r);
        continue;
      case '[':

        if (cursor_[1] == '[') {
          if (!parse_table_array_item(r, *current_section))
            return r;
        } else {
          current_section = parse_section_name(r);
          if (!current_section)
            return r;
        }
        continue;
      case '\0':
        return r;
      default:
        if (!parse_key_value(r, *current_section))
          return r;
        if (!skip_line(r))
          return r;
        continue;
      }
  }

private:
  char *cursor_{nullptr};

  static std::string_view view(char const *from, char const *to) {
    return std::string_view{from, size_t(to - from)};
  }

  static bool is_eol(char c) {
    switch (c) {
    case '\r':
    case '\n':
    case '\0':
      return true;
    default:
      return false;
    }
  }

  static bool is_space(char c) {
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '\0':
      return true;
    default:
      return false;
    }
  }

  static bool is_comment_starts(char c) {
    switch (c) {
    case '#':
    case ';':
      return true;
    default:
      return false;
    }
  }

  static bool is_letter(char c) {
    switch (c) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '_':
      return true;
    default:
      return false;
    }
  }

  static bool is_valid_for_identifier(char c) {
    switch (c) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '_':
    case '.':
      return true;
    default:
      return false;
    }
  }

  void skip_spaces_and_lines(result &r) {
    for (;;)
      switch (*cursor_) {
      case ' ':
      case '\t':
      case '\r':
        ++cursor_;
        continue;
      case '\n':
        ++cursor_;
        get_current_line(r);
        continue;
      default:
        return;
      }
  }

  void skip_spaces(result &) {
    for (;;)
      switch (*cursor_) {
      case ' ':
      case '\r':
      case '\t':
        ++cursor_;
        continue;
      default:
        return;
      }
  }

  void skip_comment(result &r) {
    ++cursor_;
    for (;;)
      switch (*cursor_) {
      case '\n':
        ++cursor_;
        get_current_line(r);
        skip_spaces_and_lines(r);
        return;
      case '\0':
        return;
      default:
        ++cursor_;
        continue;
      }
  }

  void skip_lines_and_comments(result &r) {
    for (;;)
      switch (*cursor_) {
      case ' ':
      case '\t':
      case '\r':
        ++cursor_;
        continue;
      case '\n':
        ++cursor_;
        get_current_line(r);
        skip_spaces_and_lines(r);
        continue;
      case ';':
      case '#':
        skip_comment(r);
      default:
        return;
      }
  }

  bool skip_line(result &r) {
    skip_spaces(r);
    if (is_comment_starts(*cursor_))
      skip_comment(r);
    switch (*cursor_) {
    case '\n':
      skip_spaces_and_lines(r);
      return true;
    case '\0':
      return true;
    default:
      return false;
    }
  }

  void get_current_line(result &r) { ++r.line_no; }

  bool parse_table_array_item(result &r, value &section) {

    cursor_ += 2;
    skip_spaces(r);
    if (!is_letter(*cursor_)) {
      r.error = make_error_code(error::expected_table_array_name);
      return false;
    }

    char *head = cursor_;

    while (is_valid_for_identifier(*cursor_))
      ++cursor_;
    char *tail = cursor_;

    skip_spaces(r);
    if (*cursor_++ != ']') {
      r.error = make_error_code(error::expected_closing_table_array);
      return false;
    }

    if (*cursor_++ != ']') {
      r.error = make_error_code(error::expected_closing_table_array);
      return false;
    }

    detail::ascii::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};

    value *p = section.find(name);
    if (p == nullptr || !p->is_table()) {
      r.error = make_error_code(error::duplicated_item);
      return false;
    }

    value new_table = value::make_table();

    bool end_of_table = false;
    while (!end_of_table) {
      skip_lines_and_comments(r);

      switch (*cursor_) {
      case '[':
      case '\0':
        end_of_table = true;
        continue;
      default:
        if (!parse_key_value(r, new_table))
          return false;
        if (!skip_line(r)) {
          r.error = make_error_code(error::unexpected_chars_after_value);
          return false;
        }
        continue;
      }
    }

    p->emplace_back(std::move(new_table));
    return true;
  }

  value *parse_section_name(result &r) {

    ++cursor_;
    skip_spaces(r);

    if (!is_letter(*cursor_))
      return nullptr;

    char *head = cursor_++;
    while (is_valid_for_identifier(*cursor_))
      ++cursor_;
    char *tail = cursor_;

    skip_spaces(r);
    if (*cursor_ != ']')
      return nullptr;

    detail::ascii::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};

    if (name != "default" && r.config.contains(name))
      return nullptr;

    ++cursor_;
    if (!skip_line(r))
      return nullptr;

    return r.config.insert(name, value::make_table());
  }

  bool parse_key_value(result &r, value &section) {

    if (!is_letter(*cursor_)) {
      r.error = make_error_code(error::invalid_item_name);
      return false;
    }

    char *head = cursor_;
    while (is_valid_for_identifier(*cursor_))
      ++cursor_;
    char *tail = cursor_;

    skip_spaces(r);
    if (*cursor_ != '=') {
      r.error = make_error_code(error::expected_equal_sign);
      return false;
    }

    detail::ascii::lower_case(head, tail);
    auto name = std::string_view{head, std::size_t(tail - head)};

    if (section.contains(name)) {
      r.error = make_error_code(error::duplicated_item);
      return false;
    }

    ++cursor_;

    value v;
    if (!parse_value(r, v))
      return false;

    section.insert(name, std::move(v));

    return true;
  }

  bool parse_value(result &r, value &v) {

    skip_spaces(r);

    switch (*cursor_) {
    case '\"':
    case '\'':
      return parse_string(r, v);
    case '[':
      return parse_array(r, v);
    case '{':
      return parse_inline_table(r, v);
    case '\n':
    case '\0':
    case '#':
    case ';':
      r.error = make_error_code(error::expected_value);
      return false;
    default:
      return parse_sentense(r, v);
    }
  }

  bool parse_string(result &r, value &v) {

    char const quote = *cursor_;
    char const *head = ++cursor_;

    bool eos = false;

    while (!eos)
      switch (*cursor_) {
      case '\'':
      case '\"':
        eos = (*cursor_ == quote);
        if (eos)
          continue;
        ++cursor_;
        continue;
      case '\r':
      case '\n':
      case '\0':
        r.error = make_error_code(error::expected_quote);
        return false;
      default:
        ++cursor_;
        continue;
      }

    v = view(head, cursor_);
    ++cursor_;

    return true;
  }

  bool parse_array(result &r, value &v) {

    value new_array = value::make_array();

    ++cursor_;
    skip_lines_and_comments(r);
    if (*cursor_ == ']') {
      ++cursor_;
      v = std::move(new_array);
      return true;
    }

    bool end_of_array = false;
    value item;

    while (!end_of_array) {

      if (*cursor_ == '\0') {
        r.error = make_error_code(error::expected_closing_array);
        return false;
      }

      if (!parse_value(r, item))
        return false;

      new_array.emplace_back(std::move(item));

      skip_lines_and_comments(r);
      switch (*cursor_) {
      case ',':
        ++cursor_;
        skip_lines_and_comments(r);
        continue;
      case ']':
        ++cursor_;
        end_of_array = true;
        continue;
      default:
        r.error = make_error_code(error::expected_comma_or_closing_array);
        return false;
      }
    }

    v = std::move(new_array);

    return true;
  }

  bool parse_inline_table(result &r, value &v) {

    value new_table = value::make_table();

    ++cursor_;
    skip_lines_and_comments(r);
    if (*cursor_ == '}') {
      ++cursor_;
      v = std::move(new_table);
      return true;
    }

    bool end_of_table = false;

    while (!end_of_table) {

      if (*cursor_ == '\0') {
        r.error = make_error_code(error::expected_closing_table);
        return false;
      }

      if (!parse_key_value(r, new_table))
        return false;

      skip_lines_and_comments(r);
      switch (*cursor_) {
      case ',':
        ++cursor_;
        skip_lines_and_comments(r);
        continue;
      case '}':
        ++cursor_;
        end_of_table = true;
        continue;
      default:
        r.error = make_error_code(error::expected_comma_or_closing_table);
        return false;
      }
    }

    v = std::move(new_table);

    return true;
  }

  bool parse_sentense(result &r, value &v) {

    char const *head = cursor_++;
    char const *last = head;

    bool end = false;
    while (!end)
      switch (*cursor_) {
      case ' ':
      case '\t':
        ++cursor_;
        continue;
      case '\r':
      case '\n':
      case '#':
      case ';':
      case ',':
      case '[':
      case ']':
      case '{':
      case '}':
        end = true;
        continue;
      case '=':
        r.error = make_error_code(error::unexpected_equal_sign);
        return false;
      case '\0':
        end = true;
        continue;
      default:
        last = cursor_;
        ++cursor_;
        continue;
      }

    v = view(head, last + 1);

    return true;
  }

}; // parser

inline std::unique_ptr<char[]> read_file(char const *file_name) {
  using namespace std;
  unique_ptr<char[]> source;
  unique_ptr<FILE, int (*)(FILE *)> file{fopen(file_name, "rb"), fclose};
  if (!file)
    return source;
  fseek(file.get(), 0, SEEK_END);
  auto const file_size = ftell(file.get());
  if (file_size == -1L)
    return source;
  source = make_unique<char[]>(file_size + 1);
  fseek(file.get(), 0, SEEK_SET);
  size_t const was_read = fread(source.get(), 1, size_t(file_size), file.get());
  if (was_read != size_t(file_size)) {
    source.reset();
    return source;
  }
  source[file_size] = '\0';
  return source;
}

} // namespace detail

inline result parse_string(char const *source) {
  detail::parser p;
  size_t const n = source == nullptr ? 0 : strlen(source);
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(n + 1);
  memcpy(buffer.get(), source, n);
  buffer[n] = '\0';
  return p.parse(std::move(buffer));
}

inline result parse(char const *file_name) {
  using namespace std;
  std::unique_ptr<char[]> source = detail::read_file(file_name);
  if (!source)
    return result{error::unable_to_read_file};
  detail::parser p;
  return p.parse(std::move(source));
}

inline result parse(std::string const &file_name) {
  return parse(file_name.data());
}

} // namespace confetti
