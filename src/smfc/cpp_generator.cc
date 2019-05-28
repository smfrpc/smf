// Copyright 2019 SMF Authors
//

// vars,  Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "cpp_generator.h"

#include <map>
#include <sstream>

#include <glog/logging.h>

#include "smf_printer.h"

namespace smf_gen {
namespace utils {

static std::string
file_name_identifier(const std::string &filename) {
  std::string result;
  for (unsigned i = 0; i < filename.size(); i++) {
    char c = filename[i];
    if (isalnum(c)) {
      result.push_back(toupper(c));
    } else if (c == '_') {
      result.push_back(c);
    } else {
      static char hex[] = "0123456789abcdef";
      result.push_back('_');
      result.push_back(hex[(c >> 4) & 0xf]);
      result.push_back(hex[c & 0xf]);
    }
  }
  return result;
}

inline bool
is_camel_case(const std::string &s) {
  return std::find_if(s.begin(), s.end(), ::isupper) != s.end();
}

inline std::string
lower_vec(const std::vector<std::string> &vec) {
  std::stringstream ss;
  for (auto &x : vec) {
    ss << x;
  }
  auto retval = ss.str();
  for (auto i = 0u; i < retval.length(); ++i) {
    if (std::isalnum(retval[i])) { retval[i] = std::tolower(retval[i]); }
  }
  return retval;
}

inline std::string
proper_postfix_token(std::string s, std::string postfix) {
  CHECK(!s.empty() || !postfix.empty()) << "Can't compute postfix token";
  if (is_camel_case(s)) {
    s[0] = std::toupper(s[0]);
    postfix[0] = std::toupper(postfix[0]);
    return s + postfix;
  }
  return lower_vec({s, std::string("_"), postfix});
}

inline std::string
proper_prefix_token(std::string prefix, std::string s) {
  CHECK(!s.empty() || !prefix.empty()) << "Can't compute prefix token";
  if (is_camel_case(s)) {
    s[0] = toupper(s[0]);
    prefix[0] = toupper(prefix[0]);
    return prefix + s;
  }
  return lower_vec({prefix, std::string("_"), s});
}

static void
print_header_service_ctor_dtor(smf_printer &printer,
                               const smf_service *service) {
  VLOG(1) << "print_header_service_ctor_dtor for service: " << service->name();
  std::map<std::string, std::string> vars;

  vars["Service"] = service->name();
  printer.print(vars, "virtual ~$Service$() = default;\n");
  printer.print(vars, "$Service$() : handles_{{\n");
  printer.indent();

  for (int32_t i = 0, max = service->methods().size(); i < max; ++i) {
    auto &method = service->methods()[i];
    vars["MethodName"] = method->name();
    vars["RawMethodName"] = proper_prefix_token("raw", method->name());
    vars["InType"] = method->input_type_name();
    vars["OutType"] = method->output_type_name();
    vars["MethodId"] = std::to_string(method->method_id());
    printer.print("smf::rpc_service_method_handle(\n");
    printer.indent();
    printer.print("[this](smf::rpc_recv_context c) -> "
                  "seastar::future<smf::rpc_envelope> {\n");
    printer.indent();
    printer.print("// Session accounting\n"
                  "auto session_id = c.session();\n");
    printer.print(vars, "return $RawMethodName$(std::move(c)).then("
                        "[session_id](smf::rpc_envelope e) {\n");
    printer.indent();
    printer.print(
      "e.letter.header.mutate_session(session_id);\n"
      "return seastar::make_ready_future<smf::rpc_envelope>(std::move(e));\n");
    printer.outdent();
    if (i < max - 1) {
      printer.print("});}),\n");
    } else {
      printer.print("});})\n");
    }
    printer.outdent();
    printer.outdent();
  }
  printer.outdent();
  printer.print("}} {}\n");
}

static void
print_header_service_handle_request_id(smf_printer &printer,
                                       const smf_service *service) {
  printer.print("virtual smf::rpc_service_method_handle *\n"
                "method_for_request_id(uint32_t idx) override final {\n");
  printer.indent();
  printer.print("switch(idx){\n");
  printer.indent();

  for (auto i = 0u; i < service->methods().size(); ++i) {
    std::map<std::string, std::string> vars;
    auto &method = service->methods()[i];
    vars["ServiceID"] = std::to_string(method->service_id());
    vars["MethodId"] = std::to_string(method->method_id());
    vars["VectorIdx"] = std::to_string(i);
    printer.print(
      vars,
      "case ($ServiceID$ ^ $MethodId$): return &handles_[$VectorIdx$];\n");
  }
  printer.print("default: return nullptr;\n");
  printer.outdent();
  printer.print("}\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_header_service_handles(smf_printer &printer, const smf_service *service) {
  std::map<std::string, std::string> vars;
  vars["ServiceHandleSize"] = std::to_string(service->methods().size());
  printer.print(vars,
                "virtual const std::array<smf::rpc_service_method_handle, "
                "$ServiceHandleSize$> &\n"
                "methods() final {\n");
  printer.indent();
  printer.print("return handles_;\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_header_service_method(smf_printer &printer, const smf_method *method) {
  VLOG(1) << "print_header_service_method: " << method->name();

  std::map<std::string, std::string> vars;
  vars["RawMethodName"] = proper_prefix_token("raw", method->name());
  vars["MethodName"] = method->name();
  vars["MethodId"] = std::to_string(method->method_id());
  vars["InType"] = method->input_type_name();
  vars["OutType"] = method->output_type_name();
  printer.print(vars, "inline virtual\n"
                      "seastar::future<smf::rpc_typed_envelope<$OutType$>>\n");
  printer.print(
    vars, "$MethodName$(smf::rpc_recv_typed_context<$InType$> &&rec) {\n");
  printer.indent();
  printer.print(
    vars, "using out_type = $OutType$;\n"
          "using env_t = smf::rpc_typed_envelope<out_type>;\n"
          "env_t data;\n"
          "// User should override this method.\n"
          "//\n"
          "// Helpful for clients to set the status.\n"
          "// Typically follows HTTP style. Not imposed by smf whatsoever.\n"
          "// i.e. 501 == Method not implemented\n"
          "data.envelope.set_status(501);\n"
          "return seastar::make_ready_future<env_t>(std::move(data));\n");
  printer.outdent();
  printer.print("}\n");

  // RAW

  printer.print(vars, "inline virtual\n"
                      "seastar::future<smf::rpc_envelope>\n");
  printer.print(vars, "$RawMethodName$(smf::rpc_recv_context &&c) {\n");
  printer.indent();
  printer.print(
    vars,
    "using inner_t = $InType$;\n"
    "using input_t = smf::rpc_recv_typed_context<inner_t>;\n"
    "using mid_t = smf::rpc_typed_envelope<$OutType$>;\n"
    "return $MethodName$(input_t(std::move(c))).then([this](mid_t x) {\n");
  printer.indent();
  printer.print("return "
                "seastar::make_ready_future<smf::rpc_envelope>(x.serialize_"
                "data());\n");
  printer.outdent();
  printer.print("});\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_header_service(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_header_service: " << service->name();
  std::map<std::string, std::string> vars{};
  vars["Service"] = service->name();
  vars["ServiceID"] = std::to_string(service->service_id());
  vars["ServiceHandleSize"] = std::to_string(service->methods().size());

  printer.print(vars, "class $Service$: public smf::rpc_service {\n");
  printer.print(" private:\n");
  printer.print(vars, "  std::array<smf::rpc_service_method_handle, "
                      "$ServiceHandleSize$> handles_;\n");
  printer.print(" public:\n");
  printer.indent();
  print_header_service_ctor_dtor(printer, service);

  // print the overrides for smf
  printer.print("virtual const char *\n"
                "service_name() const override final {\n");
  printer.indent();
  printer.print(vars, "return \"$Service$\";\n");
  printer.outdent();
  printer.print("}\n");
  printer.print("virtual uint32_t\n"
                "service_id() const override final {\n");
  printer.indent();
  printer.print(vars, "return $ServiceID$;\n");
  printer.outdent();
  printer.print("}\n");

  // print the overrides for smf
  printer.print(
    "virtual std::ostream &\nprint(std::ostream &o) const override final {\n");
  printer.indent();
  printer.print("return o << *this;\n");
  printer.outdent();
  printer.print("}\n");

  print_header_service_handles(printer, service);
  print_header_service_handle_request_id(printer, service);

  for (auto &method : service->methods()) {
    print_header_service_method(printer, method.get());
  }

  printer.outdent();
  printer.print(vars, "}; // end of service: $Service$\n");
}

static void
print_header_client_method(smf_printer &printer, const smf_method *method) {
  std::map<std::string, std::string> vars;
  vars["RawMethodName"] = proper_prefix_token("raw", method->name());
  vars["MethodName"] = method->name();
  vars["MethodID"] = std::to_string(method->method_id());
  vars["ServiceID"] = std::to_string(method->service_id());
  vars["ServiceName"] = method->service_name();
  vars["InType"] = method->input_type_name();
  vars["OutType"] = method->output_type_name();

  // untyped
  printer.print(vars,
                "inline virtual\n"
                "seastar::future<smf::rpc_recv_typed_context<$OutType$>>\n"
                "$MethodName$(smf::rpc_typed_envelope<$InType$> x) {\n");
  printer.print(vars, "  return $MethodName$(x.serialize_data());\n");
  printer.print("}\n");

  // untyped
  printer.print(vars,
                "inline virtual\n"
                "seastar::future<smf::rpc_recv_typed_context<$OutType$>>\n"
                "$MethodName$(smf::rpc_envelope e) {\n");
  printer.indent();
  printer.print(vars, "/// ServiceID: $ServiceID$\n"
                      "/// MethodID:  $MethodID$\n"
                      "e.set_request_id($ServiceID$ ^ $MethodID$);\n"
                      "return send<$OutType$>(std::move(e));\n");
  printer.outdent();
  printer.print("}\n");
}

static void
print_header_client(smf_printer &printer, const smf_service *service) {
  // print the client rpc code
  VLOG(1) << "print_header_client for service: " << service->name();
  std::map<std::string, std::string> vars{};
  vars["ClientName"] = proper_postfix_token(service->name(), "client");
  vars["ServiceID"] = std::to_string(service->service_id());

  printer.print(vars, "class $ClientName$: ");
  printer.print("public smf::rpc_client {\n");
  printer.indent();
  printer.print("public:\n");
  printer.indent();
  // print ctor
  printer.print(vars, "explicit $ClientName$(seastar::ipv4_addr "
                      "server_addr)\n:smf::rpc_client(std::move("
                      "server_addr)) {}\n");
  // print ctor2
  printer.print(vars, "explicit $ClientName$(smf::rpc_client_opts o)"
                      "\n:smf::rpc_client(std::move(o)) {}\n");
  // move ctor
  printer.print(vars, "$ClientName$($ClientName$ &&o) = default;\n");

  // print ctor2
  printer.print(vars, "virtual ~$ClientName$() = default;\n");

  // name
  printer.print("virtual const char *name() const final {\n");
  printer.indent();
  printer.print(vars, "return \"$ClientName$\";\n");
  printer.outdent();
  printer.print("}\n");

  for (auto &method : service->methods()) {
    print_header_client_method(printer, method.get());
  }

  printer.outdent();
  printer.print(vars, "}; // end of rpc client: $ClientName$\n");
  printer.outdent();
}
static void
print_std_ostream_services(smf_printer &printer, const smf_service *service) {
  VLOG(1) << "print_std_ostream_services: " << service->name();
  std::map<std::string, std::string> vars{};
  {
    std::vector<std::string> tmp(
      service->raw_service()->defined_namespace->components.begin(),
      service->raw_service()->defined_namespace->components.end());
    tmp.push_back(service->name());
    vars["FQDNService"] = boost::algorithm::join(tmp, "::");
  }
  vars["ServiceID"] = std::to_string(service->service_id());
  vars["ServiceHandleSize"] = std::to_string(service->methods().size());

  printer.print("namespace std {\n");
  printer.indent();
  printer.print(vars, "inline ostream& operator<<(ostream &o, const "
                      "$FQDNService$ &s) {\n");
  printer.indent();
  printer.print(
    vars, "o << \"$FQDNService$={total_handles=$ServiceHandleSize$,\";\n");
  for (auto i = 0; i < service->methods().size(); ++i) {
    auto &method = service->methods()[i];
    if (i != 0) { printer.print("o << \", \";\n"); }
    vars["MethodName"] = method->name();
    vars["InType"] = method->input_type_name();
    vars["OutType"] = method->output_type_name();
    vars["MethodId"] = std::to_string(method->method_id());
    printer.print(vars, "o << \"method={id=$MethodId$, "
                        "fn=$MethodName$($InType$):$OutType$}\";\n");
  }
  printer.print("return o << \"}\";\n");
  printer.outdent();
  printer.print("}\n");

  // namespace outdent
  printer.outdent();
  printer.print("} // namespace std\n");
}
}  // namespace utils
}  // namespace smf_gen

namespace smf_gen {

using namespace smf_gen::utils;  // NOLINT

void
cpp_generator::generate_header_prologue() {
  VLOG(1) << "get_header_prologue";
  std::map<std::string, std::string> vars;

  vars["filename"] = input_filename;
  vars["filename_identifier"] =
    file_name_identifier(input_filename_without_ext());
  vars["filename_base"] = input_filename_without_ext();
  vars["message_header_ext"] = message_header_ext();

  printer_.print("// Generated by smfc.\n");
  printer_.print("// Any local changes WILL BE LOST.\n");
  printer_.print(vars, "// source: $filename$\n");
  for (auto &p : included_files()) {
    if (p.first == input_filename) { continue; }
    vars["header"] = p.first;
    printer_.print(vars, "// transitive fbs: $header$\n");
  }
  printer_.print("#pragma once\n");
  printer_.print(vars, "#ifndef SMF_$filename_identifier$_INCLUDED\n");
  printer_.print(vars, "#define SMF_$filename_identifier$_INCLUDED\n\n");
}

void
cpp_generator::generate_header_prologue_forward_decl_external() {
  for (auto &srv : services()) {
    for (auto &srv : services()) {
      print_std_ostream_services(printer_, srv.get());
    }
  }
  printer_.print("\n");
}

void
cpp_generator::generate_header_prologue_forward_decl() {
  for (auto &srv : services()) {
    std::map<std::string, std::string> vars;
    vars["Service"] = srv->name();
    vars["ClientName"] = proper_postfix_token(srv->name(), "client");
    printer_.print(vars, "class $Service$;\n");
    printer_.print(vars, "class $ClientName$;\n");
  }
  printer_.print("\n");
}

void
cpp_generator::generate_header_prologue_namespace() {
  if (!package().empty()) {
    std::vector<std::string> parts = package_parts();
    std::map<std::string, std::string> vars;
    for (auto part = parts.begin(); part != parts.end(); part++) {
      vars["part"] = *part;
      printer_.print(vars, "namespace $part$ {\n");
    }
    printer_.print("\n");
  }
}

void
cpp_generator::generate_header_prologue_includes() {
  VLOG(1) << "get_header_includes";
  std::map<std::string, std::string> vars;
  static const std::vector<std::string> headers = {
        "ostream", "seastar/core/sstring.hh",  
        "smf/std-compat.h", "smf/rpc_service.h",
        "smf/rpc_client.h", "smf/rpc_recv_typed_context.h",
        "smf/rpc_typed_envelope.h", "smf/log.h" };

  for (auto &hdr : headers) {
    vars["header"] = hdr;
    printer_.print(vars, "#include <$header$>\n");
  }
  printer_.print("\n");
  vars["filename"] = input_filename;
  vars["filename_identifier"] =
    file_name_identifier(input_filename_without_ext());
  vars["filename_base"] = input_filename_without_ext();
  vars["message_header_ext"] = message_header_ext();
  printer_.print(vars, "#include \"$filename_base$$message_header_ext$\"\n\n");
}
void
cpp_generator::generate_header_epilogue_namespace() {
  VLOG(1) << "get_header_epilogue_namespace";
  std::map<std::string, std::string> vars;

  vars["filename"] = input_filename;
  vars["filename_identifier"] =
    file_name_identifier(input_filename_without_ext());

  if (!package().empty()) {
    std::vector<std::string> parts = package_parts();

    for (auto part = parts.rbegin(); part != parts.rend(); part++) {
      vars["part"] = *part;
      printer_.print(vars, "}  // namespace $part$\n");
    }
    printer_.print(vars, "\n");
  }
  printer_.print(vars, "\n");
}

void
cpp_generator::generate_header_epilogue() {
  VLOG(1) << "get_header_epilogue";
  std::map<std::string, std::string> vars;

  vars["filename"] = input_filename;
  vars["filename_identifier"] =
    file_name_identifier(input_filename_without_ext());

  printer_.print("\n\n");
  printer_.print(vars, "#endif  // SMF_$filename_identifier$_INCLUDED\n");
}

void
cpp_generator::generate_header_services() {
  VLOG(1) << "get_header_services";
  VLOG(1) << "Generating (" << services().size() << ") services";
  for (auto &srv : services()) {
    print_header_service(printer_, srv.get());
    printer_.print("\n");
    print_header_client(printer_, srv.get());
    printer_.print("\n");
  }
}

}  // namespace smf_gen
