#include "src/env_config.h"

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "common/json_parser.h"

using namespace std;

static string ExecutableDir()
{
  char buf[4096];
#if defined(__APPLE__)
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0)
    return {};
#elif defined(__linux__)
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0)
    return {};
  buf[n] = '\0';
#else
  return {};
#endif
  char real[4096];
  if (realpath(buf, real) == nullptr)
    return {};
  string path(real);
  auto slash = path.find_last_of('/');
  if (slash == string::npos)
    return {};
  path.resize(slash);
  return path;
}

static string Dirname(const string & path)
{
  auto slash = path.find_last_of("/\\");
  if (slash == string::npos)
    return ".";
  if (slash == 0)
    return "/";
  return path.substr(0, slash);
}

static string JoinPath(const string & dir, const string & file)
{
  if (file.empty())
    return dir;
  if (file[0] == '/' || (file.size() > 1 && file[1] == ':'))
    return file;
  if (dir.empty() || dir == ".")
    return file;
  if (dir.back() == '/' || dir.back() == '\\')
    return dir + file;
  return dir + "/" + file;
}

static bool FileReadable(const string & path)
{
  return access(path.c_str(), R_OK) == 0;
}

static vector<string> ConfigSearchDirs()
{
  vector<string> dirs;
  dirs.push_back("emulator/configs");
  dirs.push_back("configs");

  string dir = ExecutableDir();
  for (int i = 0; i < 6 && !dir.empty(); ++i) {
    dirs.push_back(dir + "/configs");
    dirs.push_back(dir + "/emulator/configs");
    dirs.push_back(JoinPath(Dirname(dir), "emulator/configs"));
    auto slash = dir.find_last_of('/');
    if (slash == string::npos)
      break;
    dir.resize(slash);
  }
  return dirs;
}

static bool ReadFileText(const string & path, string & text, string & error)
{
  ifstream file(path);
  if (!file.is_open()) {
    error = "cannot read '" + path + "'";
    return false;
  }
  stringstream buffer;
  buffer << file.rdbuf();
  text = buffer.str();
  return true;
}

static string ResolvePath(const string & configDir, const string & path)
{
  if (path.empty())
    return path;
  if (path[0] == '/' || (path.size() > 1 && path[1] == ':'))
    return path;

  string besideConfig = JoinPath(configDir, path);
  if (FileReadable(besideConfig))
    return besideConfig;
  if (FileReadable(path))
    return path;
  return besideConfig;
}

static bool ParseDriveObject(JsonParser & parser, DriveMount & mount, string & error)
{
  if (!parser.Eat('{')) {
    error = "drive entry must be an object";
    return false;
  }

  string type;
  string file;
  string path;
  string dpb;

  if (!parser.Eat('}')) {
    for (;;) {
      string key;
      if (!parser.String(key)) {
        error = parser.m_error;
        return false;
      }
      if (!parser.Eat(':')) {
        error = "expected ':' after drive field '" + key + "'";
        return false;
      }

      if (key == "type" || key == "file" || key == "path" || key == "dpb") {
        string value;
        if (!parser.String(value)) {
          error = parser.m_error;
          return false;
        }
        if (key == "type")
          type = value;
        else if (key == "file")
          file = value;
        else if (key == "path")
          path = value;
        else
          dpb = value;
      }
      else {
        cerr << "warning: ignoring unknown drive field '" << key << "'" << endl;
        if (!parser.SkipValue()) {
          error = parser.m_error;
          return false;
        }
      }

      if (parser.Eat('}'))
        break;
      if (!parser.Eat(',')) {
        error = "expected a comma in drive object";
        return false;
      }
    }
  }

  if (type.empty()) {
    error = "drive entry requires \"type\"";
    return false;
  }

  for (auto & ch : type)
    ch = (char)tolower((unsigned char)ch);

  if (type == "image") {
    if (file.empty()) {
      error = "image drive requires \"file\"";
      return false;
    }
    mount.m_kind = DriveMount::Kind::eImage;
    mount.m_path = file;
    mount.m_dpb = dpb;
    return true;
  }

  if (type == "dir" || type == "directory") {
    if (path.empty()) {
      error = "dir drive requires \"path\"";
      return false;
    }
    mount.m_kind = DriveMount::Kind::eDir;
    mount.m_path = path;
    return true;
  }

  error = "drive type '" + type + "' is not supported (use image or dir)";
  return false;
}

static bool ApplyEnvObject(JsonParser & parser, const string & configPath, Options & options, string & error)
{
  string configDir = Dirname(configPath);

  if (!parser.Eat('{')) {
    error = "environment config must be a JSON object";
    return false;
  }

  if (parser.Eat('}'))
    return true;

  for (;;) {
    string key;
    if (!parser.String(key)) {
      error = parser.m_error;
      return false;
    }
    if (!parser.Eat(':')) {
      error = "expected ':' after '" + key + "'";
      return false;
    }

    if (key == "name") {
      string name;
      if (!parser.String(name)) {
        error = parser.m_error;
        return false;
      }
      // informational; lookup already used the name
      (void)name;
    }
    else if (key == "type") {
      if (!parser.String(options.m_typeName)) {
        error = parser.m_error;
        return false;
      }
    }
    else if (key == "rom") {
      string rom;
      if (!parser.String(rom)) {
        error = parser.m_error;
        return false;
      }
      options.m_romFn = ResolvePath(configDir, rom);
    }
    else if (key == "font") {
      if (!parser.String(options.m_font)) {
        error = parser.m_error;
        return false;
      }
    }
    else if (key == "ram") {
      int ram = 0;
      if (!parser.Number(ram) || ram < 0) {
        error = parser.m_error.empty() ? "invalid ram" : parser.m_error;
        return false;
      }
      options.m_ramSize_k = (unsigned)ram;
    }
    else if (key == "scale" || key == "fontSize" || key == "station") {
      int value = 0;
      if (!parser.Number(value) || value < 0) {
        error = parser.m_error.empty() ? ("invalid " + key) : parser.m_error;
        return false;
      }
      if (key == "scale")
        options.m_videoScale = (unsigned)value;
      else if (key == "fontSize")
        options.m_fontSize = (unsigned)value;
      else
        options.m_starnetStation = value;
    }
    else if (key == "sdl" || key == "turbo" || key == "ei" || key == "gamekb") {
      bool value = false;
      if (!parser.Bool(value)) {
        error = parser.m_error;
        return false;
      }
      if (key == "sdl")
        options.m_useSDL = value;
      else if (key == "turbo")
        options.m_turbo = value;
      else if (key == "ei")
        options.m_withEI = value;
      else
        options.m_gameKb = value;
    }
    else if (key == "drives") {
      if (!parser.Eat('{')) {
        error = "\"drives\" must be an object";
        return false;
      }
      if (!parser.Eat('}')) {
        for (;;) {
          string driveKey;
          if (!parser.String(driveKey)) {
            error = parser.m_error;
            return false;
          }
          if (!parser.Eat(':')) {
            error = "expected ':' after drive '" + driveKey + "'";
            return false;
          }

          // Reject numeric drive keys in configs.
          bool numeric = !driveKey.empty()
                      && std::all_of(driveKey.begin(), driveKey.end(),
                                     [](char ch) { return isdigit((unsigned char)ch); });
          if (numeric) {
            error = "drive key '" + driveKey + "' must be a letter a-p, not a number";
            return false;
          }

          unsigned index = 0;
          if (!ParseDriveLetter(driveKey, index, error))
            return false;

          DriveMount mount;
          if (!ParseDriveObject(parser, mount, error))
            return false;

          mount.m_path = ResolvePath(configDir, mount.m_path);
          if (options.m_drives.count(index)) {
            error = "duplicate drive '" + driveKey + "'";
            return false;
          }
          options.m_drives[index] = mount;

          if (parser.Eat('}'))
            break;
          if (!parser.Eat(',')) {
            error = "expected a comma in drives object";
            return false;
          }
        }
      }
    }
    else {
      cerr << "warning: ignoring unknown config field '" << key << "'" << endl;
      if (!parser.SkipValue()) {
        error = parser.m_error;
        return false;
      }
    }

    if (parser.Eat('}'))
      break;
    if (!parser.Eat(',')) {
      error = "expected a comma in environment config";
      return false;
    }
  }

  return true;
}

static bool PeekConfigName(const string & path, string & name, string & type)
{
  string text;
  string error;
  if (!ReadFileText(path, text, error))
    return false;
  JsonParser parser(text);
  if (!parser.Eat('{'))
    return false;

  name.clear();
  type.clear();
  if (parser.Eat('}'))
    return false;

  for (;;) {
    string key;
    if (!parser.String(key))
      return false;
    if (!parser.Eat(':'))
      return false;
    if (key == "name" || key == "type") {
      string value;
      if (!parser.String(value))
        return false;
      if (key == "name")
        name = value;
      else
        type = value;
    }
    else if (!parser.SkipValue()) {
      return false;
    }
    if (parser.Eat('}'))
      break;
    if (!parser.Eat(','))
      return false;
  }
  return !name.empty() || !type.empty();
}

string FindEnvConfigPath(const string & name, string & error)
{
  if (name.empty()) {
    error = "empty environment name";
    return {};
  }

  // Direct path if the user passed something that looks like a file.
  if (name.find('/') != string::npos || name.find('\\') != string::npos
      || (name.size() > 5 && name.substr(name.size() - 5) == ".json")) {
    if (FileReadable(name))
      return name;
    error = "environment file '" + name + "' not found";
    return {};
  }

  for (const auto & dir : ConfigSearchDirs()) {
    string candidate = JoinPath(dir, name + ".json");
    if (FileReadable(candidate))
      return candidate;
  }

  // Scan for matching "name" field.
  for (const auto & dir : ConfigSearchDirs()) {
    DIR * dp = opendir(dir.c_str());
    if (dp == nullptr)
      continue;
    while (dirent * ent = readdir(dp)) {
      string fn = ent->d_name;
      if (fn.size() < 6 || fn.substr(fn.size() - 5) != ".json")
        continue;
      string path = JoinPath(dir, fn);
      string cfgName;
      string cfgType;
      if (!PeekConfigName(path, cfgName, cfgType))
        continue;
      if (cfgName == name) {
        closedir(dp);
        return path;
      }
    }
    closedir(dp);
  }

  error = "environment '" + name + "' not found (looked in emulator/configs/)";
  return {};
}

bool LoadEnvConfig(const string & name, Options & options, string & error)
{
  string path = FindEnvConfigPath(name, error);
  if (path.empty())
    return false;

  string text;
  if (!ReadFileText(path, text, error))
    return false;

  JsonParser parser(text);
  if (!ApplyEnvObject(parser, path, options, error))
    return false;

  if (options.m_typeName.empty()) {
    error = "environment '" + name + "' does not set \"type\"";
    return false;
  }

  options.m_envConfigPath = path;
  return true;
}

void ListEnvConfigs(vector<EnvConfigInfo> & out)
{
  out.clear();
  vector<string> seen;

  for (const auto & dir : ConfigSearchDirs()) {
    DIR * dp = opendir(dir.c_str());
    if (dp == nullptr)
      continue;
    while (dirent * ent = readdir(dp)) {
      string fn = ent->d_name;
      if (fn.size() < 6 || fn.substr(fn.size() - 5) != ".json")
        continue;
      string path = JoinPath(dir, fn);
      string cfgName;
      string cfgType;
      if (!PeekConfigName(path, cfgName, cfgType))
        continue;
      if (cfgName.empty())
        cfgName = fn.substr(0, fn.size() - 5);
      if (find(seen.begin(), seen.end(), cfgName) != seen.end())
        continue;
      seen.push_back(cfgName);
      EnvConfigInfo info;
      info.m_name = cfgName;
      info.m_type = cfgType;
      info.m_path = path;
      out.push_back(info);
    }
    closedir(dp);
  }

  sort(out.begin(), out.end(),
       [](const EnvConfigInfo & a, const EnvConfigInfo & b) { return a.m_name < b.m_name; });
}
