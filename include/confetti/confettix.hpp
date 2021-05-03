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


enum struct error {
	ok,
	unable_to_read_file,
	not_enough_memory,
	invalid_byte_order_mark,
	expected_section_or_parameter,
	invalid_section_name,
	duplicated_section,
	invalid_parameter_value,
	duplicated_parameter,
	expected_equal_after_parameter_name,
	expected_comma_or_closed_square_brace,
	expected_parameter_in_table,
	expected_comma_or_closed_figure_brace,
	unclosed_string
}; // error


struct error_category : std::error_category {

	char const* name() const noexcept override {
		return "confetti";
	}

	std::string message(int code) const noexcept override {
		switch(error(code)) {
		case error::ok:
			return "Ok";
		case error::unable_to_read_file:
			return "Unable to read file";
		case error::not_enough_memory:
			return "Not enough memory";
		case error::invalid_byte_order_mark:
			return "Invalid BOM, only UTF-8 is supported (0xEFBBBF)";
		case error::expected_section_or_parameter:
			return "Expected section or parameter";
		case error::invalid_section_name:
			return "Invalid section name";
		case error::duplicated_section:
			return "Duplicated section";
		case error::invalid_parameter_value:
			return "Invalid parameter value";
		case error::duplicated_parameter:
			return "Duplicated parameter";
		case error::expected_equal_after_parameter_name:
			return "Expected '=' after parameter name";
		case error::expected_comma_or_closed_square_brace:
			return "Expected ',' or ']'";
		case error::expected_parameter_in_table:
			return "Expected parameter inside table";
		case error::expected_comma_or_closed_figure_brace:
			return "Expected ',' or '}'";
		case error::unclosed_string:
			return "Unclosed string";
		default:
			return "Unknown";
		}
	}
};


inline error_category const confetti_category;


inline std::error_code make_error_code(error e) noexcept {
  return {int(e), confetti_category};
}

} // confetti


namespace std {

	template <> struct is_error_code_enum<confetti::error> : true_type {};
	
} // std


namespace confetti {

namespace detail::ascii {

	inline char lower_case(char c) {
		// clang-format off
		switch (c) {
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
			case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
			case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
			case 'V': case 'W': case 'X': case 'Y': case 'Z':
				return c + 32;
		   	default:
		    	return c;
		 }
		 // clang-format on
	}

	inline void lower_case(char* head, char* tail) {
		for (; head != tail; ++head)
			*head = lower_case(*head);
	}

} // namespace detail::ascii


class value {
	using array = std::vector<value>;
	using array_ptr = std::unique_ptr<array>;
	using table = std::unordered_map<std::string_view, value>;
	using table_ptr = std::unique_ptr<table>;
public:
	using size_type = size_t;
	
	static value const none;

	static value make(std::string_view const& sv) noexcept { return value{sv}; }
	static value make_array() noexcept { return value{std::make_unique<array>()}; }
	static value make_table() noexcept { return value{std::make_unique<table>()}; }

	value() noexcept = default;
	value(value const &) = delete;
	value &operator=(value const &) = delete;
	value(value &&) noexcept = default;
	value &operator=(value &&) noexcept = default;

	bool is_single_value() const noexcept {
		return std::holds_alternative<std::string_view>(holder_);
  	}

	bool is_array() const noexcept {
 		return std::holds_alternative<array_ptr>(holder_);
  	}

	bool is_table() const noexcept {
		return std::holds_alternative<table_ptr>(holder_);
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

 	bool empty() const noexcept {
 		return size() == 0;
 	}

 	bool emplace_back(value &&v) noexcept {
 	    try {
			array_ptr const *p = std::get_if<array_ptr>(&holder_);
			if (p == nullptr)
		   		return false;
			array &data = *p->get();
			data.emplace_back(std::move(v));
			return true;
 	    } catch (std::bad_alloc const &) {
 	      return false;
 	    }
	}

 	value const& operator [] (size_type i) const noexcept {
 		array_ptr const* p = std::get_if<array_ptr>(&holder_);
 		if (p == nullptr)
 			return none;
 		array const &data = *p->get();
 		return data[i];
 	}

 	template<std::size_t N>
 	value const& operator [] (char const (&name)[N]) const noexcept {
 		table_ptr const *p = std::get_if<table_ptr>(&holder_);
 		if (p == nullptr)
	    	return none;
	    table const& table = *p->get();
	   	auto const found = table.find(std::string_view{name, N - 1});
	   	if (found == table.end())
	    	return none;
	   	return found->second;		
 	} 
	
	value *insert(std::string_view const &name, value &&value) {
	    table_ptr const *p = std::get_if<table_ptr>(&holder_);
	    if (p == nullptr)
	    	return nullptr;
	    table &data = *p->get();
	    auto const found = data.find(name);
	    if (found != data.end())
	    	return nullptr;
	    auto emplaced = data.try_emplace(name, std::move(value));
	    if (!emplaced.second)
	    	return nullptr;
	    return &emplaced.first->second;
	}

	bool contains(std::string_view const &name) const noexcept {
	    table_ptr const *p = std::get_if<table_ptr>(&holder_);
	   	if (p == nullptr)
	    	return false;
	    table const& table = *p->get();
	   	auto const found = table.find(name);
	   	if (found == table.end())
	    	return false;
	   	return true;
	}

	template <std::size_t N> bool contains(char const (&name)[N]) const noexcept {
	    return contains(std::string_view{name, N - 1});
	}

	template<typename T> std::optional<T> to() const;
	std::string operator | (char const* bydefault) const;
	template<typename T> T operator | (T const& bydefault) const;

private:

	using holder_type = std::variant<std::monostate, std::string_view,
	                                 array_ptr, table_ptr>;

	holder_type holder_;

	value(std::string_view const& sv) noexcept: holder_{sv} { }
	value(array_ptr p) noexcept: holder_{std::move(p)} { }
	value(table_ptr p) noexcept: holder_{std::move(p)} { }	

	template<typename T> std::optional<T> parse_unsigned() const noexcept {
			std::string_view const *p = std::get_if<std::string_view>(&holder_);
			if (p == nullptr)
					return std::nullopt;

			char const *head = p->data();
			char const *tail = p->data() + p->size();
			int radix = 10;
			
	    switch (*head) {
	    case '+':
	    		++head;
	    		break;
	    case '-':
					return std::nullopt;
	    case '0':
					if (head[1] != 'x')
							break;
					if (tail - head < 3)
							return std::nullopt;
					head += 2;
					radix = 16;
					break;
	    default:
					break;
	    }

			T number;
			auto const parsed = std::from_chars(head, tail, number, radix);
				
			if (parsed.ec == std::errc::invalid_argument || parsed.ptr != tail)
					return std::nullopt;
				
			return {number};		
	}

	template<typename T> std::optional<T> parse_signed() const noexcept {
		std::string_view const *p = std::get_if<std::string_view>(&holder_);
		if (p == nullptr)
			return std::nullopt;
			
		char const *head = p->data();
		char const *tail = p->data() + p->size();
			
		if (*head == '+')
			++head;
			
		if (head == tail)
			return std::nullopt;
			
		T number;
		auto const parsed = std::from_chars(head, tail, number);
			
		if (parsed.ec == std::errc::invalid_argument || parsed.ptr != tail)
			return std::nullopt;
			
		return {number};	
	}
}; // value


inline value const value::none;


template<> std::optional<bool> value::to() const {
	using detail::ascii::lower_case;
	
	std::string_view const *p = std::get_if<std::string_view>(&holder_);
	if (p == nullptr)
		return std::nullopt;
	bool parsed;
	char const *cc = p->data();

	switch(p->size()) {
	case 4:
		parsed = lower_case(cc[0]) == 't' && lower_case(cc[1] == 'r') &&
		         lower_case(cc[2]) == 'u' && lower_case(cc[3] == 'e');
		if(!parsed)
			return std::nullopt;
		return {true};
	case 5:
		parsed = lower_case(cc[0]) == 'f' && lower_case(cc[1]) == 'a' &&
		         lower_case(cc[2]) == 'l' && lower_case(cc[3]) == 's' &&
		         lower_case(cc[4]) == 'e';
		if(!parsed)
			return std::nullopt;
    return { false };
	default:
		return std::nullopt;
	}
}


template<> std::optional<int> value::to() const {
	return parse_signed<int>();	
}


template<> std::optional<unsigned> value::to() const {
	return parse_unsigned<unsigned>();
}


template<> std::optional<long long> value::to() const {
	return parse_signed<long long>();
}


template<> std::optional<unsigned long long> value::to() const {
	return parse_unsigned<unsigned long long>();
}


template<> std::optional<double> value::to() const {
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
    return std::nullopt;
#else
  char *endptr;
  double number = std::strtod(head, &endptr);
  if (endptr != tail)
    return std::nullopt;
#endif

  return {number};	
}


template<> std::optional<std::string_view> value::to() const {
	std::string_view const *p = std::get_if<std::string_view>(&holder_);
	if (p == nullptr)
    	return std::nullopt;

	return {*p};
}


template<> std::optional<std::string> value::to() const {
  std::string_view const *p = std::get_if<std::string_view>(&holder_);
  if (p == nullptr)
    return std::nullopt;

  return {std::string{p->begin(), p->end()}};
}


std::string value::operator | (char const* bydefault) const {
	auto const parsed = to<std::string>();
	if(!parsed)
		return std::string{bydefault};
	return *parsed;
}


template<typename T> T value::operator | (T const& bydefault) const {
	auto const parsed = to<T>();
	if(!parsed)
		return bydefault;
	return *parsed;
}


struct result {

	std::unique_ptr<char[]> source;
	std::error_code error_code;
	unsigned line_no{0};
	value config;

	result() = default;
	result(result const&) = delete;
	result& operator = (result const&) = delete;
	result(result&&) = default;
	result& operator = (result&&) = default;
	
	explicit result(error e) noexcept:
		error_code{int(e), confetti_category}
	{ }

	explicit result(std::unique_ptr<char[]> source) noexcept:
		source(std::move(source)), config{value::make_table()}
	{ }
		
	explicit operator bool() const noexcept {
		return !error_code;
	}
}; // result


namespace detail {

// clang-format off
enum struct token {
	undefined, end, opened_square_brace, closed_square_brace,
	opened_figure_brace, closed_figure_brace, equal, comma, text,
	unclosed_string
}; // token
// clang-format on

class scaner {
public:
	scaner() noexcept = default;
	scaner(scaner const&) noexcept = default;
	scaner& operator = (scaner const&) noexcept = default;
	explicit scaner(char* source): cursor_{source} { }
	int line_no() const noexcept { return line_no_; }
	char* head() noexcept { return head_; }
	char* tail() noexcept { return tail_; }

	std::string_view text() const noexcept {
		return std::string_view{head_, std::size_t(tail_ - head_)};
	}

	bool scan_byte_order_mark() {
		switch(int(*cursor_)) {
		case 0xEF:
			++cursor_;
			if(int(*cursor_) != 0xBB)
				return false;
			++cursor_;
			if(int(*cursor_) != 0xBF)
				return false;
			return true;
		case 0xFE:
		case 0xFF:
			return false;
		default:
			return true;
		}
	}

	token next() {
		// clang-format off
		for(;;)
			switch(*cursor_) {
			case ' ': case '\t': case '\r':
				++cursor_; continue;
			case '\n':
				++cursor_; ++line_no_; continue;
			case '#': case ';':
				skip_comment(); continue;
			default:
				goto skipped_whitespaces;
			}
skipped_whitespaces:
		switch(*cursor_) {
		case '[':
			++cursor_;
			return token::opened_square_brace;
		case ']':
			++cursor_;
			return token::closed_square_brace;
		case '{':
			++cursor_;
			return token::opened_figure_brace;
		case '}':
			++cursor_;
			return token::closed_figure_brace;
		case '=':
			++cursor_;
			return token::equal;
		case ',':
			++cursor_;
			return token::comma;
		case '"':
			return scan_string<'"'>();
		case '\'':
			return scan_string<'\''>();
		case '\0':
			return token::end;
		default:
			return scan_word();
		}
		// clang-format on			
	}	

private:
	char* cursor_{nullptr};
	int line_no_{1};
	char* head_;
	char* tail_;

	void skip_comment() {
		++cursor_;
		for(;;)
			switch(*cursor_) {
			case '\n':
				++cursor_;
				++line_no_;
				return;
			case '\0':
				return;
			default:
				++cursor_;
				continue;
			}
	}

	template<char Q> token scan_string() {
		head_ = ++cursor_;
		for(;;)
			switch(*cursor_) {
			case '\n':
			case '\0':
				return token::unclosed_string;
			case Q:
				tail_ = cursor_;
				return token::text;				
			default:
				++cursor_;
				continue;
			}
	}

	token scan_word() {
		// clang-format off
		head_ = cursor_;
		for(;;)
			switch(*cursor_) {
			case ' ': case '\t': case '\r': case '\n': case '\0':
			case '[': case ']': case '{': case '}': case '=':
			case ',': case '"': case '\'': case '#': case ';':
				tail_ = cursor_;
				return token::text;
			default:
				++cursor_;
				continue;
			}
		// clang-format on
	}
}; //scaner


class parser {
public:
	parser() = default;
	parser(parser const&) = delete;
	parser& operator = (parser const&) = delete;
	parser(parser&&) = default;
	parser& operator = (parser&&) = default;

	parser(std::unique_ptr<char[]> source) noexcept:
		result_{std::move(source)}
	{ }

	result parse() {
		if(!result_.source)
			return std::move(result_);
		scaner_ = scaner{result_.source.get()};

		section_ = result_.config.insert("default",
			                             value::make_table());
		if(section_ == nullptr) {
			result_.error_code = make_error_code(error::not_enough_memory);
			return std::move(result_);
		}
	
		if(!scaner_.scan_byte_order_mark()) {
			result_.error_code = make_error_code(error::invalid_byte_order_mark);
			return std::move(result_);
		}

		for(;;)
			switch(scaner_.next()) {
			case token::opened_square_brace:
				if(!parse_section_name())
					return std::move(result_);
				continue;
			case token::text:
				if(!parse_property(*section_, scaner_.head(), scaner_.tail()))
					return std::move(result_);
				continue;
			case token::end:
				return std::move(result_);
			default:
				failed(error::expected_section_or_parameter);
				return std::move(result_);
			}
	}

private:

	result result_;
	scaner scaner_;
	value* section_{nullptr};

	bool failed(error e) {
		result_.error_code = make_error_code(e);
		result_.line_no = scaner_.line_no();
		return false;
	}

	bool parse_section_name() {
		if(scaner_.next() != token::text)
			return failed(error::invalid_section_name);
		ascii::lower_case(scaner_.head(), scaner_.tail());
		auto const name = scaner_.text();
		if(name != "default" && result_.config.contains(name))
			return failed(error::duplicated_section);
		if(scaner_.next() != token::closed_square_brace)
			return failed(error::invalid_section_name);
		value* section = name == "default" ?
				result_.config.
				result_.config.insert(name, value::make_table());
		if(section == nullptr)
			return failed(error::not_enough_memory);
		section_ = section;
		return true;
	}

	bool parse_property(value& table, char* head, char* tail) {
		ascii::lower_case(head, tail);
		auto const name = std::string_view{head, std::size_t(tail - head)};
		if(section_->contains(name))
			return failed(error::duplicated_parameter);
		if(scaner_.next() != token::equal)
			return failed(error::expected_equal_after_parameter_name);
		value parsed;
		if(!parse_property_value(scaner_.next(), parsed))
			return false;
		value* inserted = table.insert(name, std::move(parsed));
		if(inserted == nullptr)
			return failed(error::not_enough_memory);
		return true;
	}

	bool parse_property_value(token tk, value& v) {
		switch(tk) {
		case token::opened_square_brace:
			return parse_array(v);
		case token::opened_figure_brace:
			return parse_table(v);
		case token::text:
			v = value::make(scaner_.text());
			return true;
		case token::unclosed_string:
			return failed(error::unclosed_string);
		default:
			return failed(error::invalid_parameter_value);
		}
	}

	bool parse_array(value& array) {
		array = value::make_array();
		token tk = scaner_.next();
		if(tk == token::closed_square_brace)
			return true;
		for(;;) {
			value item;
			if(!parse_property_value(tk, item))
				return false;
			auto const emplaced = array.emplace_back(std::move(item));
			if(!emplaced)
				return failed(error::not_enough_memory);
			switch(scaner_.next()) {
			case token::comma:
				tk = scaner_.next();
				continue;
			case token::closed_square_brace:
				return true;
			default:
				return failed(error::expected_comma_or_closed_square_brace);
			}
		}
	}

	bool parse_table(value& table) {
		table = value::make_table();
		token tk = scaner_.next();
		if(tk == token::closed_figure_brace)
			return true;
		for(;;) {
			if(tk != token::text)
				return failed(error::expected_parameter_in_table);
			if(!parse_property(table, scaner_.head(), scaner_.tail()))
				return false;
			switch(scaner_.next()) {
			case token::comma:
				tk = scaner_.next();
				continue;
			case token::closed_square_brace:
				return true;
			default:
				return failed(error::expected_comma_or_closed_figure_brace);
			}
		}
	}

}; // parser


inline std::unique_ptr<char[]> read_file(char const *file_name) {
	using namespace std;
	unique_ptr<char[]> source;
	unique_ptr<FILE, int (*)(FILE *)>
		file{fopen(file_name, "rb"), fclose};
	if (!file)
    	return source;
  	fseek(file.get(), 0, SEEK_END);
  	auto const file_size = ftell(file.get());
  	if (file_size == -1L)
    	return source;
  	source = make_unique<char[]>(file_size + 1);
  	fseek(file.get(), 0, SEEK_SET);
  	size_t const read_ok = fread(source.get(), 1, size_t(file_size), file.get());
  	if (read_ok != size_t(file_size)) {
    	source.reset();
    	return source;
  	}
  	source[file_size] = '\0';
  	return source;
}

} // detail


inline result parse_text(char const* text) {
	size_t const n = (text == nullptr ? 0 : strlen(text));
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(n + 1);
	std::memcpy(buffer.get(), text, n);
	buffer[n] = '\0';
	detail::parser p{std::move(buffer)};
	return p.parse();
}


inline result parse(char const* filename) {
	std::unique_ptr<char[]> source = detail::read_file(filename);
	if(!source)
		return result{error::unable_to_read_file};
	detail::parser p{std::move(source)};
	return p.parse();
}


inline result parse(std::string const& filename) {
	return parse(filename.data());
}

} // confetti
