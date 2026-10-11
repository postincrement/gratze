#ifndef CMD_ARGS_H
#define CMD_ARGS_H

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <sstream>

class CommandLineArgs
{
  public:
    struct Option
    {
      const char   m_shortName;
      const char * m_longName;
      char m_type = 0;
      const char * m_help;
    };

    struct OptionValue
    {
      enum Type {
        eBoolean,
        eInteger,
        eUnsigned,
        eString,
        eFloat
      } m_type;

      template<class Type>
      bool GetValue(Type & value) const;

      std::string m_wildcard;

      // if this was made a union,
      // initializers could not be used
      bool m_boolean = false;
      int m_integer = 0;
      unsigned m_unsigned = 0;
      double m_float = 0.0;
      std::string m_string;
    };

    enum class ArgType {
      eEnd          = 0,
      eNone         = ' ',
      eString       = 's',
      eInteger      = 'i',
      eDecimal      = 'd',
      eUnsigned     = 'u',
      eHexOrDecimal = 'x',
      eBoolean      = 'b',
      eIncrement    = '+',
      eFloat        = 'f'
    };

    typedef std::multimap<std::string, OptionValue> ValueMap;

    CommandLineArgs();

    int Parse(const Option * options, int argc, char *argv[]);

    std::string Usage() const;

    bool HasArg(const std::string & arg) const;

    template<class Type>
    bool GetValue(const std::string & opt, Type & value) const
    {  
      const CommandLineArgs::OptionValue * ptr = FindOption(opt);
      if (ptr == nullptr)
        return false;
      return ptr->GetValue(value);
    } 

    template<class Type>
    bool GetValues(const std::string & opt, std::vector<Type> & values) const
    {
      std::multimap<std::string, Type> entries;
      if (!GetValues(opt, entries))
        return false;
      for (auto & r : entries) {
        values.push_back(r.second);
      }
      return true;
    }

    template<class Type>
    bool GetValues(const std::string & opt, std::map<unsigned, Type> & values, std::string & error) const
    {
      std::multimap<std::string, Type> entries;
      if (!GetValues(opt, entries))
        return false;

      for (auto & r : entries) {
        unsigned val = 0;
        if (!ParseDriveSuffix(r.first, val, error))
          return false;
        if (values.find(val) != values.end()) {
          std::stringstream strm;
          strm << "duplicate drive '" << r.first << "'";
          error = strm.str();
          return false;
        }
        values.insert(typename std::map<unsigned, Type>::value_type(val, r.second));
      }
      return true;
    }

    // Drive wildcard suffixes: "a"-"p" (preferred) or legacy "0"-"15".
    static bool ParseDriveSuffix(const std::string & suffix, unsigned & index, std::string & error);

    template<class Type>
    bool GetValues(const std::string & opt, std::multimap<std::string, Type> & values) const
    {
      int count = m_values.count(opt);
      if (count == 0)
        return true;

      ValueMap::const_iterator r = m_values.find(opt);
      while (count > 0) {
        Type value;
        if (r->second.GetValue(value)) {
          values.insert(typename std::multimap<std::string, Type>::value_type(r->second.m_wildcard, value));
        }
        ++r;
        --count;
      }

      return true;
    }

    std::string DumpValues() const;

  protected:
    bool ProcessArg(bool isLong, const Option & optDef, const std::string & arg_, const std::string & wildcard = "");
    const OptionValue * FindOption(const std::string & opt) const;

    const Option * m_options = nullptr;
    ValueMap m_values;
};


template<>
bool CommandLineArgs::OptionValue::GetValue(std::string & value) const;

template<>
bool CommandLineArgs::OptionValue::GetValue(unsigned & value) const;

template<>
bool CommandLineArgs::OptionValue::GetValue(int & value) const;

template<>
bool CommandLineArgs::OptionValue::GetValue(bool & value) const;

template<>
bool CommandLineArgs::OptionValue::GetValue(double & value) const;

#endif // CMD_ARGS_H