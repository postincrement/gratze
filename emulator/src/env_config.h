#ifndef ENV_CONFIG_H_
#define ENV_CONFIG_H_

#include <string>
#include <vector>

#include "src/options.h"

struct EnvConfigInfo
{
  std::string m_name;
  std::string m_type;
  std::string m_path;
};

// Resolve <name> to a config file path. Empty on failure (error set).
std::string FindEnvConfigPath(const std::string & name, std::string & error);

// Load a named environment into options (type, drives, optional flags).
// Does not clear CLI-derived fields already set; merges into options.
bool LoadEnvConfig(const std::string & name, Options & options, std::string & error);

// List all discoverable environment configs.
void ListEnvConfigs(std::vector<EnvConfigInfo> & out);

#endif // ENV_CONFIG_H_
