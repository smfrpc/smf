package smf

import (
	"fmt"
	"math"

	"github.com/cespare/xxhash"

	flatbuffers "github.com/google/flatbuffers/go"
)

// NewHeader - Constructs Header struct from bytes.
func NewHeader(buf []byte) (hdr *Header) {
	hdr = new(Header)
	hdr.Init(buf, 0)
	return
}

// BuildHeader - Builds smf RPC request/response header.
func BuildHeader(session uint16, body []byte, meta uint32) []byte {
	builder := flatbuffers.NewBuilder(32) // [1]
	res := CreateHeader(builder,
		0,                                         // compression int8,
		0,                                         // bitflags int8,
		session,                                   // session uint16,
		uint32(len(body)),                         // size uint32,
		uint32(math.MaxUint32&xxhash.Sum64(body)), // checksum uint32,
		meta, //	meta uint32
	)
	builder.Finish(res)
	// TODO(crackcomm): builder prepends 4 bytes
	// the header is the last 16 bytes of message
	// so I did [^1] 32 bytes allocation anyway
	// I have no idea why it does that
	return builder.FinishedBytes()[4:]
}

// String - Formats header as debug string.
func (hdr *Header) String() string {
	return fmt.Sprintf("[ compression=%d, bitflags=%d, session=%d, size=%d, checksum=%d, meta=%d ]",
		hdr.Compression(),
		hdr.Bitflags(),
		hdr.Session(),
		hdr.Size(),
		hdr.Checksum(),
		hdr.Meta())
}
