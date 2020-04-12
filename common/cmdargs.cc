#include <iostream>
#include <cstring>
#include <sstream>

#include "misc.h"
#include "cmdargs.h"

using namespace std;

CommandLineArgs::CommandLineArgs()
{
}

bool CommandLineArgs::ProcessArg(bool isLong, const Option & optDef, const std::string & arg_, const std::string & wildcard)
{
  std::string arg(arg_);

  std::string valueNameShort;
  if ((optDef.m_shortName != ' ') && (optDef.m_shortName != 0))  
    valueNameShort = std::string(1, '-') + std::string(1, optDef.m_shortName);

  std::string valueNameLong;
  if (optDef.m_longName != nullptr)
    valueNameLong = std::string(2, '-') + optDef.m_longName;

  std::string optionName = isLong ? valueNameLong : valueNameShort;  

  OptionValue value; 

  value.m_wildcard = wildcard;

  switch ((ArgType)optDef.m_type) {

    // should never occur - end of list
    case ArgType::eEnd:
      cerr << "error: internal error - bad command line arg list" << endl;      
      return false;

    // simple no arg option
    case ArgType::eNone:
      value.m_type    = OptionValue::Type::eBoolean; 
      value.m_boolean = true;
      break;

    // increment the argument count
    case ArgType::eIncrement:
      value.m_type       = OptionValue::Type::eUnsigned; 
      value.m_unsigned++;
      break;

    // string value
    case ArgType::eString:
      value.m_type    = OptionValue::Type::eString; 
      value.m_string = arg;
      break;

    // floating point value value
    case ArgType::eFloat:
      {
        char * ptr;
        double num = strtod(arg.c_str(), &ptr);
        if (ptr == arg.c_str()) {
          cerr << "error: argument to '" << optionName << " is not a floating point value" << endl;
          return false;
        }
        value.m_type  = OptionValue::Type::eFloat; 
        value.m_float = num;
      }
      break;

    // signed integer decimal value
    case ArgType::eInteger:
    case ArgType::eDecimal:
      {
        char * ptr;
        int num = strtol(arg.c_str(), &ptr, 10);
        if (ptr == arg.c_str()) {
          cerr << "error: argument to '" << optionName << " is not a decimal integer" << endl;
          return false;
        }
        value.m_type    = OptionValue::Type::eInteger; 
        value.m_integer = num;
      }
      break;

    // unsigned integer decimal value
    case ArgType::eUnsigned:
      {
        char * ptr;
        int num = strtoul(arg.c_str(), &ptr, 10);
        if (ptr == arg.c_str()) {
          cerr << "error: argument to '" << optionName << " is not an unsigned decimal integer" << endl;
          return false;
        }
        value.m_type    = OptionValue::Type::eUnsigned; 
        value.m_unsigned = num;
      }
      break;

    // unsigned decimal or hex value
    case ArgType::eHexOrDecimal:
      {
        int radix = 10;
        if (arg.substr(0,2) == "0x") {
          radix = 16;
          arg = arg.substr(2);
        }
        char * ptr;
        int num = strtoul(arg.c_str(), &ptr, radix);
        if (ptr == arg.c_str()) {
          cerr << "error: argument to '" << optionName << " is not an unsigned decimal or hex integer" << endl;
          return false;
        }
        value.m_type     = OptionValue::Type::eUnsigned; 
        value.m_unsigned = num;
      }
      break;

    case ArgType::eBoolean:
      {
        value.m_type     = OptionValue::Type::eBoolean; 
        // see if true/false
        for (auto & r : arg) r = tolower(r);
        if ((arg == "false") || (arg == "no") || (arg == "0")) {
          value.m_boolean = false; 
        }
        else if ((arg == "true") || (arg == "yes") || (arg == "1")) {
          value.m_boolean = true;
        } 
        else {
          cerr << "error: argument to '" << optionName << " cannot be converted to a boolean value" << endl;
          return false;
        }
      }
      break;

    default:
      cerr << "error: unknown command line argument type '" << optDef.m_type << "'" << endl;      
      return false;
  }

  if (!valueNameShort.empty()) {
    //cout << "inserting " << valueNameShort << endl;
    m_values.insert(ValueMap::value_type(valueNameShort, value));
  }

  if (!valueNameLong.empty()) {
    //cout << "inserting " << valueNameLong << endl;
    m_values.insert(ValueMap::value_type(valueNameLong, value));
  }

  return true;  
}

int CommandLineArgs::Parse(const Option * options, int argc, char *argv[])
{ 
  m_options = options;
  
  // parse command line arguments
  int optIndex = 1;
  while (optIndex < argc) {

    std::string arg(argv[optIndex]);

    //cout << "parsing argument " << arg << endl;

    size_t len = arg.length();

    // non-option argument terminates options
    if (arg[0] != '-')
      break;

    // solitary "-" terminates parsing
    if (len == 1) {
      ++optIndex;
      break;
    }

    // get remainder of string after leading "-"
    std::string option(arg.substr(1, 1));

    // long options
    if (option[0] == '-') {

      //cerr << "info: parsing long opt " << arg << endl;

      // option "--" terminates parsing
      if (len == 2) {
        optIndex++;
        break;
      }

      // look for matching long option
      std::string wildCard;
      option = arg.substr(2);
      const Option * optDef = m_options;
      while (optDef->m_type != 0) {
        if (optDef->m_longName != nullptr) {
//          cout << "looking for match to '" << optDef->m_longName << "'" << endl;
          size_t len = strlen(optDef->m_longName);
          if ((len > 1) && (optDef->m_longName[len-1] == '*')) {
            len--;
            std::string base(optDef->m_longName, len);
//            cout << "Looking for match to wildcard '" << base << "' and '" << option << "'" << endl;
            if ((option.length() > len) && (option.substr(0, len) == base)) {
//              cout << "match" << endl;
              wildCard = option.substr(len);
              break;
            }
          }
          else if (option == optDef->m_longName)
            break;
        }  
        ++optDef;
      }

      // if no match, give error
      if (optDef->m_type == 0) {
        cerr << "error: no match for option '" << argv[optIndex] << "'" << endl;
        return -1;
      }

      // get argument, if any
      std::string value;
      if ((optDef->m_type == ' ') || (optDef->m_type == '+')) {
        ++optIndex;
      }
      else {
        if (++optIndex >= argc) {
          cerr << "error: command line ends without argument for '" << argv[optIndex] << "'" << endl;
          return -1;
        }
        if (argv[optIndex][0] == '-') {
          cerr << "error: argument for '" << argv[optIndex] << "' looks like another option" << endl;
          return -1;
        }
        value = argv[optIndex++];
      }

      // process
      if (!ProcessArg(true, *optDef, value, wildCard)) {
        return -1;
      }

      //++optIndex;
      continue;
    }

    //cerr << "info: parsing short opt " << arg << endl;

    // look for matching short option(s)
    int i = 1;
    while (i < arg.length()) {
      char opt = arg[i++];

      const Option * optDef = m_options;
      while (optDef->m_type != 0) {
        if ((optDef->m_shortName != ' ') && (opt == optDef->m_shortName))
          break;
        ++optDef;
      }

      // if no match, give error
      if (optDef->m_type == 0) {
        cerr << "error: no match for option '-" << opt << "'" << endl;
        return -1;
      }

      // get argument, if any
      std::string value;
      if ((optDef->m_type == ' ') || (optDef->m_type == '+')) {
        ++optIndex;
      }
      else {

        // allow remainder of option to be value
        // else look for following value
        if (i < arg.length()) {
          value = arg.substr(i);
          i = arg.length();
          ++optIndex;
        }
        else {
          if (++optIndex >= argc) {
            cerr << "error: command line ends without argument for '-" << opt << "'" << endl;
            return -1;
          }
          if (argv[optIndex][0] == '-') {
            cerr << "error: argument for '" << argv[optIndex] << "' looks like another option" << endl;
            return -1;
          }
          value = argv[optIndex++];
        }
      }

      //cout << "long short value is " << value << endl;

      // process
      if (!ProcessArg(false, *optDef, value)) {
        return -1;
      }
    }
  }

  return optIndex;
}

std::string CommandLineArgs::Usage() const
{
  ColumnFormatter::Columns<2> columns;

  const Option * optDef = m_options;
  while (optDef->m_type != 0) {
    {
      std::stringstream strm;
      std::string first;
      if (optDef->m_shortName != ' ') {
        strm << "-" << optDef->m_shortName;
        first = "|";
      }
      if (optDef->m_longName != nullptr) {
        strm << first << "--" << optDef->m_longName;
      }

      switch ((ArgType)optDef->m_type) {
        case ArgType::eNone:
        case ArgType::eIncrement:
          break;
        case ArgType::eString:
          strm << " string";
          break;
        case ArgType::eInteger:
        case ArgType::eDecimal:
          strm << " int";
          break;
        case ArgType::eUnsigned:
          strm << " unsigned";
          break;
        case ArgType::eHexOrDecimal:
          strm << " hex|decimal";
          break;
        case ArgType::eBoolean:
          strm << " bool";
          break;
        case ArgType::eFloat:
          strm << " float";
          break;
        default:
          strm << " ?" << optDef->m_type << "?";  
      }

      columns[0].push_back(strm.str());
    }
    {
      std::stringstream strm;
      if (optDef->m_help != nullptr)
        strm << optDef->m_help;
      columns[1].push_back(strm.str());
    }
    ++optDef;
  }

  std::stringstream strm;
  strm << ColumnFormatter::Print<2>(columns, { "   ", " : " });
  return strm.str();
}

std::string CommandLineArgs::DumpValues() const
{
  std::stringstream strm;
  ColumnFormatter::Columns<3> columns;

  columns[0].push_back("Name");
  columns[0].push_back("----");
  columns[1].push_back("Type");
  columns[1].push_back("----");
  columns[2].push_back("Value");
  columns[2].push_back("----");

  for (const auto & r : m_values) {
    columns[0].push_back(r.first);

    const OptionValue & value = r.second;

    stringstream col2;

    if (!value.m_wildcard.empty()) {
      col2 << "(" << value.m_wildcard << "):";
    }
    switch (value.m_type) {
      case OptionValue::Type::eBoolean:
        columns[1].push_back("boolean");
        col2 << std::string(value.m_boolean ? "true" : "false");
        break;

      case OptionValue::Type::eInteger:
        columns[1].push_back("integer");
        col2 << value.m_integer;
        break;

      case OptionValue::Type::eUnsigned:
        columns[1].push_back("unsigned");
        col2 << value.m_unsigned;
        break;

      case OptionValue::Type::eString:
        columns[1].push_back("string");
        col2 << "'" << value.m_string << "'";
        break;

      case OptionValue::Type::eFloat:
        columns[1].push_back("float");
        col2 << value.m_float;
        break;
    }
    columns[2].push_back(col2.str());
  }

  strm << "Argument values" << endl;
  strm << ColumnFormatter::Print<3>(columns, { "", " ", " " });
  strm << "---------------" << endl;
  return strm.str();
}

bool CommandLineArgs::HasArg(const std::string & opt) const
{
  return m_values.find(opt) != m_values.end();
}


const CommandLineArgs::OptionValue * CommandLineArgs::FindOption(const std::string & opt) const
{
  ValueMap::const_iterator r = m_values.find(opt);
  if (r == m_values.end())
    return nullptr;

  return &r->second;
}

template<>
bool CommandLineArgs::OptionValue::GetValue(std::string & value) const
{
  if (m_type != OptionValue::Type::eString)
    return false;

  value = m_string;
  return true;  
}

template<>
bool CommandLineArgs::OptionValue::GetValue<int>(int & value) const
{
  switch (m_type) {
    case OptionValue::Type::eUnsigned:
      cerr << "warning: value " << m_unsigned << " is not unsigned" << endl;
      value = (int)m_unsigned;
      return true;  
    case OptionValue::Type::eInteger:
      value = m_integer;
      return true;
  }
  return false;
}

template<>
bool CommandLineArgs::OptionValue::GetValue(unsigned & value) const
{
  switch (m_type) {
    case OptionValue::Type::eUnsigned:
      value = m_unsigned;
      return true;  
    case OptionValue::Type::eInteger:
      value = (int)m_unsigned;
      cerr << "warning: value " << m_unsigned << " is not int" << endl;
      return true;
  }
  return false;
}

template<>
bool CommandLineArgs::OptionValue::GetValue(bool & value) const
{
  if (m_type != OptionValue::Type::eBoolean)
    return false;

  value = m_boolean;
  return true;  
}


template<>
bool CommandLineArgs::OptionValue::GetValue(double & value) const
{
  if (m_type != OptionValue::Type::eFloat)
    return false;

  value = m_float;
  return true;  
}