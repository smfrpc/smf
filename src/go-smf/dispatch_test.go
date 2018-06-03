package smf

import (
	"hash/crc32"
	. "testing"
)

func TestDispatch(t *T) {
	// Source: https://senior7515.github.io/smf/rpc/
	//
	//  auto fqn = fully_qualified_name;
	//
	//  service_id = hash( fqn(service_name) )
	//  method_id  = hash(  ∀ fqn(x) in input_args_types,
	//                      ∀ fqn(x) in output_args_types,
	//                      fqn(method_name),
	//                      separator = “:”)
	//
	//  rpc_dispatch_id = service_id ^ method_id;

	/// RequestID: 212494116 ^ 1719559449
	/// ServiceID: 212494116 == crc32("SmfStorage")
	/// MethodID:  1719559449 == crc32("Get:smf_gen::demo::Request:smf_gen::demo::Response")

	if 1719559449^212494116 != 1792279101 {
		t.Fatal("Failed at maths")
	}
	if res := crc32str("SmfStorage"); res != uint32(212494116) {
		t.Fatalf("Failed to compute crc32 for SmfStorage got %d", res)
	}
	if res := crc32str("Get:smf_gen::demo::Request:smf_gen::demo::Response"); res != uint32(1719559449) {
		t.Fatalf("Failed to compute crc32 for Get:smf_gen::demo::Request:smf_gen::demo::Response got %d", res)
	}
}

func crc32str(s string) uint32 {
	return crc32.ChecksumIEEE([]byte(s))
}
