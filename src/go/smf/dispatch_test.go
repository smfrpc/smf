package smf

import (
	"hash/crc32"
	. "testing"

	"github.com/stretchr/testify/assert"
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

	assert.Equal(t, uint32(212494116), crc32str("SmfStorage"))
	assert.Equal(t, uint32(1719559449), crc32str("Get:smf_gen::demo::Request:smf_gen::demo::Response"))
	assert.Equal(t, 1719559449^212494116, 1792279101)
}

func crc32str(s string) uint32 {
	return crc32.ChecksumIEEE([]byte(s))
}
