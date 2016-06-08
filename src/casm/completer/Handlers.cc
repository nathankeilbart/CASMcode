#ifndef HANDLERS_CC
#define HANDLERS_CC

#include "casm/completer/Handlers.hh"
#include "casm/casm_io/DataFormatter.hh"
#include "casm/clex/Configuration.hh"

namespace CASM {
  namespace Completer {
    typedef ArgHandler::ARG_TYPE ARG_TYPE;


    ARG_TYPE ArgHandler::determine_type(const po::option_description &boost_option) {
      //This string will become something like "<path>", or "arg", or "<path> (=MASTER)"
      std::string raw_boost_format = boost_option.format_parameter();
      //Sometimes boost option has default arguments. We don't want to include that
      std::string argtype_str;
      std::string::size_type space_pos = raw_boost_format.find(' ');

      //Spaces found, probably printing default value as well. Strip it off.
      if(space_pos != std::string::npos) {
        argtype_str = raw_boost_format.substr(0, space_pos);
      }

      //No spaces found, so format_parameter already returned what we wanted
      else {
        argtype_str = raw_boost_format;
      }

      for(auto it = m_argument_table.begin(); it != m_argument_table.end(); ++it) {
        if(it->first == argtype_str) {
          return it->second;
        }
      }

      return ARG_TYPE::VOID;
    }

    std::string ArgHandler::path() {
      return m_argument_table[0].first;
    }

    std::string ArgHandler::command() {
      return m_argument_table[1].first;
    }

    std::string ArgHandler::supercell() {
      return m_argument_table[2].first;
    }

    std::string ArgHandler::query() {
      return m_argument_table[3].first;
    }

    std::string ArgHandler::operation() {
      return m_argument_table[4].first;
    }


    void ArgHandler::void_to_bash(std::vector<std::string> &arguments) {
      return;
    }

    void ArgHandler::path_to_bash(std::vector<std::string> &arguments) {
      arguments.push_back("BASH_COMP_PATH");
      return;
    }

    void ArgHandler::command_to_bash(std::vector<std::string> &arguments) {
      arguments.push_back("BASH_COMP_BIN");
      return;
    }

    //void scelname_to_bash(std::vector<std::string> &arguments)

    void ArgHandler::query_to_bash(std::vector<std::string> &arguments) {
      DataFormatterDictionary<Configuration> dict = make_dictionary<Configuration>();

      for(auto it = dict.begin(); it != dict.cend(); ++it) {
        if(it->type() ==  BaseDatumFormatter<Configuration>::Property) {
          arguments.push_back(it->name());
        }
      }
      return;
    }

    void ArgHandler::operator_to_bash(std::vector<std::string> &arguments) {
      DataFormatterDictionary<Configuration> dict = make_dictionary<Configuration>();

      for(auto it = dict.begin(); it != dict.cend(); ++it) {
        if(it->type() ==  BaseDatumFormatter<Configuration>::Property) {
          arguments.push_back(it->name());
        }
      }
      return;
    }


    /**
     * This construction right here determines what the value_name of the boost options
     * should be named. It is through these strings that bash completion can
     * know which types of completions to suggest.
     */

    const std::vector<std::pair<std::string, ARG_TYPE> > ArgHandler::m_argument_table({
      std::make_pair("<path>", ARG_TYPE::PATH),
      std::make_pair("<command>", ARG_TYPE::COMMAND),
      std::make_pair("<supercell>", ARG_TYPE::SCELNAME),
      std::make_pair("<query>", ARG_TYPE::QUERY),
      std::make_pair("<operation>", ARG_TYPE::OPERATOR)
    });


    //*****************************************************************************************************//

    OptionHandlerBase::OptionHandlerBase(std::string init_tag):
      m_tag(init_tag),
      m_desc(std::string("'casm ") + init_tag + std::string("' usage")) {
    }

    const std::string &OptionHandlerBase::tag() const {
      return m_tag;
    }

    /**
     * Check if there are any program options in the options description. If there aren't, then
     * this is the first time someone is asking for those values, which we set through the
     * initialize routine. If there are values there already, just hand them back.
     */

    const po::options_description &OptionHandlerBase::desc() {
      if(m_desc.options().size() == 0) {
        initialize();
      }

      return m_desc;
    }

    void OptionHandlerBase::add_config_suboption(std::string &selection_str) {
      m_desc.add_options()
      ("config,c", po::value<std::string>(&selection_str)->default_value("MASTER")->value_name(ArgHandler::path()), "config_list files containing configurations for which to collect energies");
      return;
    }

    void OptionHandlerBase::add_help_suboption() {
      m_desc.add_options()
      ("help,h", "Print help message");
      return;
    }

    void OptionHandlerBase::add_general_help_suboption() {
      m_desc.add_options()
      ("help,h", "Print help message");
      return;
    }

    void OptionHandlerBase::add_verbosity_suboption(std::string &verbosity_str) {
      m_desc.add_options()
      ("verbosity", po::value<std::string>(&verbosity_str)->default_value("standard"), "Verbosity of output. Options are 'none', 'quiet', 'standard', 'verbose', 'debug', or an integer 0-100 (0: none, 100: all).");
      return;
    }

    //*****************************************************************************************************//

  }
}

#endif

