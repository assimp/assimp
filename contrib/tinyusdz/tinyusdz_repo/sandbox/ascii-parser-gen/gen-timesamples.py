import os
import sys
import copy


basefilename = "ascii-parser-timesamples"

# typename in USD ascc, typename in TinyUSDZ C++
types = {
    "float": "float",
    "float2": "value::float2",
}

try_parse_timesamples_template = 
"""
nonstd::optional<AsciiParser::TimeSampleData<__ty>>
AsciiParser::TryParseTimeSamples() {
  // timeSamples = '{' ((int : T) sep)+ '}'
  // sep = ','(may ok to omit for the last element.

  TimeSampleData<__ty> data;

  if (!Expect('{')) {
    return nonstd::nullopt;
  }

  if (!SkipWhitespaceAndNewline()) {
    return nonstd::nullopt;
  }

  while (!Eof()) {
    char c;
    if (!Char1(&c)) {
      return nonstd::nullopt;
    }

    if (c == '}') {
      break;
    }

    Rewind(1);

    double timeVal;
    // -inf, inf and nan are handled.
    if (!ReadBasicType(&timeVal)) {
      PushError("Parse time value failed.");
      return nonstd::nullopt;
    }

    if (!SkipWhitespace()) {
      return nonstd::nullopt;
    }

    if (!Expect(':')) {
      return nonstd::nullopt;
    }

    if (!SkipWhitespace()) {
      return nonstd::nullopt;
    }

    nonstd::optional<__ty> value;
    if (!ReadBasicType(&value)) {
      return nonstd::nullopt;
    }

    // The last element may have separator ','
    {
      // Semicolon ';' is not allowed as a separator for timeSamples array
      // values.
      if (!SkipWhitespace()) {
        return nonstd::nullopt;
      }

      char sep;
      if (!Char1(&sep)) {
        return nonstd::nullopt;
      }

      DCOUT("sep = " << sep);
      if (sep == '}') {
        // End of item
        data.push_back({timeVal, value});
        break;
      } else if (sep == ',') {
        // ok
      } else {
        Rewind(1);

        // Look ahead Newline + '}'
        auto loc = CurrLoc();

        if (SkipWhitespaceAndNewline()) {
          char nc;
          if (!Char1(&nc)) {
            return nonstd::nullopt;
          }

          if (nc == '}') {
            // End of item
            data.push_back({timeVal, value});
            break;
          }
        }

        // Rewind and continue parsing.
        SeekTo(loc);
      }
    }

    if (!SkipWhitespaceAndNewline()) {
      return nonstd::nullopt;
    }

    data.push_back({timeVal, value});
  }

  DCOUT("Parse TimeSamples success. # of items = " << data.size());

  return std::move(data);
}

convert_ts_template = """
value::TimeSamples AsciiParser::ConvertToTimeSamples___Ty(
    const TimeSampleData<__ty> &ts) {
  value::TimeSamples dst;

  for (const auto &item : ts) {
    dst.times.push_back(std::get<0>(item));

    if (item.second) {
      dst.values.push_back(item.second.value());
    } else {
      // Blocked.
      dst.values.push_back(value::ValueBlock());
    }
  }

  return dst;
}
"""


def gen_header():

    ss = ""

    for k, v in types.items():
        print(k)
        #subs = ss.replace("__ty", v)
        #print(subs)

    with open(basefilename + ".hh", "w") as f:
        f.write(ss)


def gen_source():

    ss = ""

    for k, v in types.items():
        print(k)
        subs = copy.copy(convert_ts_template)
        subs = subs.replace("__Ty", k)
        subs = subs.replace("__ty", v)
        print(subs)

    with open(basefilename + ".cc", "w") as f:
        f.write(ss)


def gen():
    gen_header()
    gen_source()

if __name__ == '__main__':
    gen()
    


