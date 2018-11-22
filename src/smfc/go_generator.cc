// Copyright 2018 SMF Authors
//

#include "go_generator.h"

#include <memory>
#include <string>

#include <glog/logging.h>

namespace smf_gen {

// helpers

static inline std::string
go_public_name(std::string s) {
  s[0] = std::toupper(s[0]);
  return s;
}

static void
print_server_method(smf_printer &printer, const smf_method *method) {
  std::map<std::string, std::string> vars;
  vars["InterfaceName"] = go_public_name(method->service_name());
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["MethodID"] = std::to_string(method->method_id());
  vars["RawMethodName"] = "Raw" + go_public_name(method->name());
  vars["MethodName"] = go_public_name(method->name());
  vars["InType"] = method->input_type_name(language::go);

  printer.print(vars, "func (s *$ServiceName$) $RawMethodName$(ctx "
                      "context.Context, req []byte) ([]byte, error) {\n");
  printer.indent();
  printer.print(vars, "return s.$InterfaceName$.$MethodName$(ctx, "
                      "GetRootAs$InType$(req, 0))\n");
  printer.outdent();
  printer.print("}\n\n");
}

static void
print_server_jump_table(smf_printer &printer, const smf_service *service) {
  std::map<std::string, std::string> vars;
  vars["InterfaceName"] = go_public_name(service->name());
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["ServiceID"] = std::to_string(service->service_id());

  printer.print(
    vars, "func (s *$ServiceName$) MethodHandle(id uint32) smf.RawHandle {\n");
  printer.indent();

  printer.print("switch id {\n");
  for (auto &m : service->methods()) {
    vars["MethodID"] = std::to_string(m->method_id());
    vars["RawMethodName"] = "Raw" + go_public_name(m->name());
    printer.print(vars, "case $ServiceID$ ^ $MethodID$:\n");
    printer.indent();
    printer.print(vars, "return s.$RawMethodName$\n");
    printer.outdent();
  }
  printer.print("default:\n");
  printer.indent();
  printer.print("return nil\n");
  printer.outdent();
  printer.print("}\n");

  printer.outdent();
  printer.print("}\n\n");
}
static void
print_server(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_server";
  std::map<std::string, std::string> vars;

  vars["InterfaceName"] = go_public_name(service->name());
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["ServiceID"] = std::to_string(service->service_id());
  printer.print(vars,
                "\n// $InterfaceName$ - $InterfaceName$ Server interface\n");

  // gen the interface for service
  printer.print(vars, "type $InterfaceName$ interface {\n");
  printer.indent();
  for (auto &m : service->methods()) {
    vars["MethodName"] = go_public_name(m->name());
    vars["InType"] = m->input_type_name(language::go);
    printer.print(vars,
                  "$MethodName$(context.Context, *$InType$) ([]byte, error)\n");
  }
  printer.outdent();
  printer.print("}\n\n");

  // gen the service
  printer.print(vars, "type $ServiceName$ struct {\n");
  printer.indent();
  printer.print(vars, "$InterfaceName$\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen ctor
  printer.print(vars,
                "func New$ServiceName$(s $InterfaceName$) *$ServiceName$ {\n");
  printer.indent();
  printer.print(vars, "return &$ServiceName${$InterfaceName$: s}\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen service name
  printer.print(vars, "func (s *$ServiceName$) ServiceName() string {\n");
  printer.indent();
  printer.print(vars, "return \"$InterfaceName$\"\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen service id
  printer.print(vars, "func (s *$ServiceName$) ServiceID() uint32 {\n");
  printer.indent();
  printer.print(vars, "return $ServiceID$\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen jump table
  print_server_jump_table(printer, service);

  // gen all methods
  for (auto &m : service->methods()) {
    print_server_method(printer, m.get());
  }
}

static void
print_client_method(smf_printer &printer, const smf_method *method) {
  std::map<std::string, std::string> vars;
  vars["InterfaceName"] = go_public_name(method->service_name());
  vars["ClientName"] = vars["InterfaceName"] + "Client";
  vars["MethodID"] = std::to_string(method->method_id());
  vars["ServiceID"] = std::to_string(method->service_id());
  vars["RawMethodName"] = "Raw" + go_public_name(method->name());
  vars["MethodName"] = go_public_name(method->name());
  vars["OutputType"] = method->output_type_name(language::go);

  // high level
  printer.print(vars,
                "func (s *$ClientName$) $MethodName$(ctx "
                "context.Context, req []byte) (*$OutputType$, error) {\n");
  printer.indent();
  printer.print(vars, "res, err := s.$RawMethodName$(ctx, req)\n"
                      "if err != nil {\n");
  printer.indent();
  printer.print("return nil, err\n");
  printer.outdent();
  printer.print("}\n");
  printer.print(vars, "return GetRootAs$OutputType$(res, 0), nil\n");
  printer.outdent();
  printer.print("}\n\n");

  // raw
  printer.print(vars, "func (s *$ClientName$) $RawMethodName$(ctx "
                      "context.Context, req []byte) ([]byte, error) {\n");
  printer.indent();
  printer.print(vars,
                "return s.Client.SendRecv(req, $ServiceID$^$MethodID$)\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_client(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_client";
  std::map<std::string, std::string> vars;

  vars["InterfaceName"] = go_public_name(service->name());
  vars["ClientName"] = vars["InterfaceName"] + "Client";
  vars["ServiceID"] = std::to_string(service->service_id());

  printer.print("// Client\n");
  // gen the client
  printer.print(vars, "type $ClientName$ struct {\n");
  printer.indent();
  printer.print(vars, "*smf.Client\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen ctor
  printer.print(vars, "func New$ClientName$(c *smf.Client) *$ClientName$ {\n");
  printer.indent();
  printer.print(vars, "return &$ClientName${Client: c}\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen all methods
  for (auto &m : service->methods()) {
    print_client_method(printer, m.get());
  }
}

// members

void
go_generator::generate_header_prologue() {
  VLOG(1) << "generate_header_prologue";
  std::map<std::string, std::string> vars;
  vars["filename"] = input_filename;
  vars["package"] = package_parts().back();
  printer_.print("// Generated by smfc.\n");
  printer_.print("// Any local changes WILL BE LOST.\n");
  printer_.print(vars, "// source: $filename$\n");
  printer_.print(vars, "package $package$\n\n");
}

void
go_generator::generate_header_includes() {
  VLOG(1) << "get_header_includes";
  std::map<std::string, std::string> vars;
  printer_.print("import (\n");
  printer_.indent();
  printer_.print("\"context\"\n"
                 "\"github.com/smfrpc/smf-go/src/smf\"\n");
  printer_.outdent();
  printer_.print(")\n");
}

void
go_generator::generate_header_services() {
  VLOG(1) << "Generating (" << services().size() << ") services";
  // server code
  for (auto &srv : services()) {
    print_server(printer_, srv.get());
  }

  // client code
  for (auto &srv : services()) {
    print_client(printer_, srv.get());
  }
}

}  // namespace smf_gen
