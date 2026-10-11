#ifndef GRATZE_JSON_PARSER_H_
#define GRATZE_JSON_PARSER_H_

#include <cctype>
#include <string>
#include <vector>

// Minimal JSON reader used by DPB defs and emulator environment configs.
// Supports objects, arrays, strings, numbers, booleans, and null.
// String escapes are not supported.
struct JsonParser
{
  const std::string & m_text;
  size_t m_index = 0;
  std::string m_error;

  explicit JsonParser(const std::string & text)
    : m_text(text)
  { }

  bool Fail(const std::string & message)
  {
    m_error = message;
    return false;
  }

  void Skip()
  {
    while (m_index < m_text.size() && isspace((unsigned char)m_text[m_index]))
      ++m_index;
  }

  bool Eat(char ch)
  {
    Skip();
    if (m_index < m_text.size() && m_text[m_index] == ch) {
      ++m_index;
      return true;
    }
    return false;
  }

  bool Peek(char ch)
  {
    Skip();
    return m_index < m_text.size() && m_text[m_index] == ch;
  }

  char PeekChar()
  {
    Skip();
    if (m_index >= m_text.size())
      return 0;
    return m_text[m_index];
  }

  bool String(std::string & out)
  {
    if (!Eat('"'))
      return Fail("expected a string");
    out.clear();
    while (m_index < m_text.size() && m_text[m_index] != '"') {
      if (m_text[m_index] == '\\')
        return Fail("string escapes are not supported");
      out.push_back(m_text[m_index++]);
    }
    if (m_index >= m_text.size())
      return Fail("unterminated string");
    ++m_index;
    return true;
  }

  bool Number(int & out)
  {
    Skip();
    size_t start = m_index;
    if (m_index < m_text.size() && m_text[m_index] == '-')
      ++m_index;
    if (m_index >= m_text.size() || !isdigit((unsigned char)m_text[m_index]))
      return Fail("expected a number");
    while (m_index < m_text.size() && isdigit((unsigned char)m_text[m_index]))
      ++m_index;
    try {
      out = std::stoi(m_text.substr(start, m_index - start));
    }
    catch (...) {
      return Fail("number is out of range");
    }
    return true;
  }

  bool Bool(bool & out)
  {
    Skip();
    if (m_text.compare(m_index, 4, "true") == 0) {
      m_index += 4;
      out = true;
      return true;
    }
    if (m_text.compare(m_index, 5, "false") == 0) {
      m_index += 5;
      out = false;
      return true;
    }
    return Fail("expected a boolean");
  }

  bool Null()
  {
    Skip();
    if (m_text.compare(m_index, 4, "null") == 0) {
      m_index += 4;
      return true;
    }
    return Fail("expected null");
  }

  bool Array(std::vector<int> & out)
  {
    if (!Eat('['))
      return Fail("expected an array");
    out.clear();
    if (Eat(']'))
      return true;
    for (;;) {
      int value = 0;
      if (!Number(value))
        return false;
      out.push_back(value);
      if (Eat(']'))
        return true;
      if (!Eat(','))
        return Fail("expected a comma in array");
    }
  }

  // Skip any single JSON value (object, array, string, number, bool, null).
  bool SkipValue()
  {
    char ch = PeekChar();
    if (ch == '{')
      return SkipObject();
    if (ch == '[')
      return SkipArray();
    if (ch == '"') {
      std::string unused;
      return String(unused);
    }
    if (ch == 't' || ch == 'f') {
      bool unused = false;
      return Bool(unused);
    }
    if (ch == 'n')
      return Null();
    if (ch == '-' || isdigit((unsigned char)ch)) {
      int unused = 0;
      return Number(unused);
    }
    return Fail("expected a JSON value");
  }

  bool SkipObject()
  {
    if (!Eat('{'))
      return Fail("expected an object");
    if (Eat('}'))
      return true;
    for (;;) {
      std::string key;
      if (!String(key))
        return false;
      if (!Eat(':'))
        return Fail("expected ':' after object key");
      if (!SkipValue())
        return false;
      if (Eat('}'))
        return true;
      if (!Eat(','))
        return Fail("expected a comma in object");
    }
  }

  bool SkipArray()
  {
    if (!Eat('['))
      return Fail("expected an array");
    if (Eat(']'))
      return true;
    for (;;) {
      if (!SkipValue())
        return false;
      if (Eat(']'))
        return true;
      if (!Eat(','))
        return Fail("expected a comma in array");
    }
  }
};

#endif // GRATZE_JSON_PARSER_H_
