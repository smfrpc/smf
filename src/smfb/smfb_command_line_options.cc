#include "smfb/smfb_command_line_options.h"
namespace smf {
boost::program_options::options_description smfb_options() {
  namespace po = boost::program_options;
  po::options_description opts("smfb broker options");
  auto o = opts.add_options();
  o("smfb_port", po::value<uint16_t>()->default_value(11201), "rpc port");
  o("rand_home", po::value<bool>()->default_value(false), "randomize $HOME");
  return opts;
}

void validate_options(const boost::program_options::variables_map &vm) {
  // fix rand_home var, etc.
}


void smfb_add_command_line_options(boost::program_options::variables_map &vm,
                                   int argc,
                                   char **argv) {
  namespace po = boost::program_options;
  po::store(po::command_line_parser(argc, argv).options(smfb_options()).run(),
            vm);
  po::notify(vm);
  validate_options(vm);
}


} // end namespace smf
