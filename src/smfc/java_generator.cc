#include "java_generator.h"

#include <glog/logging.h>
#include <boost/algorithm/string/join.hpp>

namespace smf_gen {
static void
print_server_method(smf_printer &printer, const smf_method *method) {
  std::map<std::string, std::string> vars;
  vars["ClassName"] = method->service_name();
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["MethodID"] = std::to_string(method->method_id());
  vars["RawMethodName"] = "Raw" + (method->name());
  vars["MethodName"] = (method->name());

  printer.print(vars, "func (s *$ServiceName$) $RawMethodName$(ctx "
                      "context.Context, req []byte) ([]byte, error) {\n");
  printer.indent();
  printer.print(vars, "return s.$InterfaceName$.$MethodName$(ctx, "
                      "demo.GetRootAsRequest(req, 0))\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_server_jump_table(smf_printer &printer, const smf_service *service) {
  std::map<std::string, std::string> vars;
  vars["InterfaceName"] = (service->name());
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["ServiceID"] = std::to_string(service->service_id());

  printer.print(
    vars, "func (s *$ServiceName$) MethodHandle(id uint32) smf.RawHandle {\n");
  printer.indent();


  printer.print("switch id {\n");
  for (auto &m : service->methods()) {
    vars["MethodID"] = std::to_string(m->method_id());
    vars["RawMethodName"] = "Raw" + (m->name());
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
  printer.print("}\n");
}
static void
print_server(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_server";
  std::map<std::string, std::string> vars;

  vars["ClassName"] = service->name();
  vars["ServiceName"] = vars["InterfaceName"] + "Service";
  vars["ServiceID"] = std::to_string(service->service_id());
  printer.print("\n// Server\n");

  // gen the interface for service
  printer.print(vars, "type $InterfaceName$ interface {\n");
  printer.indent();
  for (auto &m : service->methods()) {
    vars["MethodName"] = (m->name());
    printer.print(
      vars, "$MethodName$(context.Context, *smf.Request) ([]byte, error)\n");
  }
  printer.outdent();
  printer.print("}\n");

  // gen the service
  printer.print(vars, "type $ServiceName$ struct {\n");
  printer.indent();
  printer.print(vars, "$InterfaceName$\n");
  printer.outdent();
  printer.print("}\n\n");

  // gen ctor
  printer.print(
    vars, "func New$ServiceName$(s $InterfaceName$) *$ServiceName$ {\n");
  printer.indent();
  printer.print(vars, "return &$ServiceName${$InterfaceName$: s}\n");
  printer.outdent();
  printer.print("}\n");

  // gen service name
  printer.print(vars, "func (s *$ServiceName$) ServiceName() string {\n");
  printer.indent();
  printer.print(vars, "return \"$InterfaceName$\"\n");
  printer.outdent();
  printer.print("}\n");

  // gen service id
  printer.print(vars, "func (s *$ServiceName$) ServiceID() uint32 {\n");
  printer.indent();
  printer.print(vars, "return $ServiceID$\n");
  printer.outdent();
  printer.print("}\n");


  // gen jump table
  print_server_jump_table(printer, service);

  // gen all methods
  for (auto &m : service->methods()) { print_server_method(printer, m.get()); }
}


static void
print_client_method(smf_printer &printer, const smf_method *method) {
  std::map<std::string, std::string> vars;
  vars["MethodID"] = std::to_string(method->method_id());
  vars["ServiceID"] = std::to_string(method->service_id());
  vars["RawMethodName"] = "raw" + method->name();
  vars["MethodName"] = method->name();
  vars["OutputType"] = method->output_type_name(language::java);

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
  printer.print(vars, "return smf.GetRootAsResponse(res, 0), nil\n");
  printer.outdent();
  printer.print("}\n");

  // raw
  printer.print(vars, "func (s *$ClientName$) $RawMethodName$(ctx "
                      "context.Context, req []byte) ([]byte, error) {\n");
  printer.indent();
  printer.print(
    vars, "return s.Client.SendRecv(req, $ServiceID$^$MethodID$)\n");
  printer.outdent();
  printer.print("}\n");
}


static void
print_client(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_client";
  std::map<std::string, std::string> vars;

  vars["ClassName"] = service->name();
  vars["ClientName"] = vars["ClassName"] + "Client";
  vars["ServiceID"] = std::to_string(service->service_id());

  printer.print("\n// Client\n");
  // gen the client
  printer.print(vars, "static public class $ClientName$ {\n");
  printer.indent();
  // gen ctor
  printer.print(vars, "public $ClientName$(final SmfClient c) {\n");
  printer.indent();
  printer.print("this.client_ = c;\n");
  printer.outdent();
  printer.print("}\n");

  // gen all methods
  for (auto &m : service->methods()) { print_client_method(printer, m.get()); }

  printer.outdent();
  printer.print("\n\n");
  printer.indent();
  printer.print(vars, "private final SmfClient client_;\n");
  printer.outdent();
  printer.print("}\n");
}

void
java_generator::generate_header_prologue() {
  VLOG(1) << "header_prologue";
  std::map<std::string, std::string> vars;
  vars["filename"] = input_filename;
  vars["OuterClass"] = "SMF";

  printer_.print("// Generated by smfc.\n");
  printer_.print("// Any local changes WILL BE LOST.\n");
  printer_.print(vars, "// source: $filename$\n");
  printer_.print("import smf.core.SmfClient;\n");
  printer_.print("import java.util.concurrent.CompletableFuture;\n");
  printer_.print(vars, "class $OuterClass$ {\n");
  printer_.indent();
}

void
java_generator::generate_header_services() {
  VLOG(1) << "Generating (" << services().size() << ") services";
  // server code
  // for (auto &srv : services()) { print_server(printer_, srv.get()); }

  // client code
  for (auto &srv : services()) { print_client(printer_, srv.get()); }
}

void
java_generator::generate_header_epilogue() {
  VLOG(1) << "header_epilogue";
  printer_.print("} // SMF outer class \n");
  printer_.outdent();
}
}  // namespace smf_gen
