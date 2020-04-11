# confetti

C++17 one-header library to parse ini files with some toml extensions.


## Info

* Support for date and time in UTC
* Support for time duration
* Support for arrays, tables, inline tables
* Keys and section names are case insensitive
* Only ASCII
* No dependencies


## Example

```ini
[default]
; line comment
# another line comment


[strings]
string = text 
stringWithSpaces = text with spaces
quotedString = 'quoted text'
doubleQuotedString = "double quoted text"


[numbers]
integer = 0
positive = +1
negative = -9223372036854775808
hex = 0xDEAD
floating = 3.14e0


[booleans] # case insensitive
true = true
false = False 
alternativeTrue = on
alternativeFalse = OFF


[dates]
date = 1970/01/01
anotherDate = 1982-02-15
dateAndTime = 1982/02/15 06:00:00
timeAsDuration = 06:00:00  # parsed as chrono::duration
shortDuration = 10 seconds # microseconds, milliseconds, minutes, hours
longDuration = 1 day # week

[arrays]
array = [feature1, feature2,
         feature3]
         
matrix = [[1, 2],
          [3, 4]]
          
          
[tables]

inlineTable = {property1 = value1,
               property2 = value2}
               
features = {feature1:on, feature2:off}

data = [{name = Anubis, gender = male},
        {name = Isis, gender = female}]

[[sameData]]
name = Anubis
gender = male

[[sameData]]
name = Isis
gender = female

```

And how to read some of this

```cpp
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <optional>

#include <confetti/confetti.hpp>


int main() {
  using namespace std;
  using namespace std::string_view_literals;
  
  confetti::result const parsed = confetti::parse("example.ini");
  if(!parsed) {
    cout << "Error at line " << parsed.line_no << ": " << parsed.message() << endl;
    return -1;
  }
  
  std::string const as_string = parsed.config["strings"]["string"].either("");
  std::string_view as_view = parsed.config["strings"]["stringWithSpaces"].either(""sv);
  
  if(parsed.config.contains("NUMBERS"))
    cout << "Ok, section names and keys are case insensitive " << endl;
  
  confetti::table const&  numbers = parsed.config["numbers"]; // Will be empty if not exists
  auto const positive = numbers["Positive"].either(0);
  optional<int> const no_hex = numbers["hex"].to<int>();
  if(!no_hex)
    cout << "hex is unsigned" << endl;
  auto const sure_hex = numbers["hex"].either(0u);
  
  auto const& dates = parsed.config["dates"];
  auto const no_time = chrono::system_clock::time_point{}; 
  auto const in_utc = dates["dateAndTime"].either(no_time);
  auto const no_duration = chrono::system_clock::duration{};
  auto const six_hours = dates["timeAsDuration"].either(no_duration);
  auto const ten_secs = dates["shortDuration"].either(no_duration);
  
  auto const& arrays = parsed.config["arrays"];
  // Parse array as vector of strings
  auto const& features_list = arrays["array"].either(vector<string>{});
  auto const& matrix = arrays["matrix"];
  bool const is_diagonal =
    matrix[0][0].either(0) + matrix[1][1].either(0)
      == matrix[0][1].either(0) + matrix[1][0].either(0);
  
  auto const& tables = parsed.config["tables"];
  value const& features = tables["features"];
  if(tables["features"]["feature1"].either(false))
    cout << "Feature 1 is activated" << endl;
    
  auto const& data = tables["data"];
  auto const& same_data = tables["sameData"];
  
  bool const truly_the_same =
    data[0]["name"].either("") == same_data[0]["name"].either("");
  if(truly_the_same)
  cout << "Array tables are the same" << endl;
  
  return 0;
}

```
