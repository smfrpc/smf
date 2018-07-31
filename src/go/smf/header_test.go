package smf

import (
	cryptorand "crypto/rand"
	"io"
	"math"
	"math/rand"
	. "testing"

	"github.com/cespare/xxhash"
	flatbuffers "github.com/google/flatbuffers/go"
)

func TestBuildHeader(t *T) {
	for index := 0; index < 10; index++ {
		meta := uint32(rand.Intn(65535))
		sess := uint16(rand.Intn(65535))
		body := make([]byte, rand.Intn(200))
		io.ReadFull(cryptorand.Reader, body)
		hbody := BuildHeader(sess, body, meta)
		assertEqual(t, 16, len(hbody))
		hdr := NewHeader(hbody)
		assertEqual(t, meta, hdr.Meta())
		assertEqual(t, sess, hdr.Session())
		assertEqual(t, uint32(len(body)), hdr.Size())
		checksum := uint32(math.MaxUint32 & xxhash.Sum64(body))
		assertEqual(t, checksum, hdr.Checksum())
	}
}

func BenchmarkBuildHeader(b *B) {
	meta := uint32(rand.Intn(65535))
	sess := uint16(rand.Intn(65535))
	body := make([]byte, rand.Intn(200))
	checksum := math.MaxUint32 & xxhash.Sum64(body)
	io.ReadFull(cryptorand.Reader, body)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		builder := flatbuffers.NewBuilder(32) // [1]
		res := CreateHeader(builder,
			0,                 // compression int8,
			0,                 // bitflags int8,
			sess,              // session uint16,
			uint32(len(body)), // size uint32,
			uint32(checksum),  // checksum uint32,
			meta,              //	meta uint32
		)
		builder.Finish(res)
		// TODO(crackcomm): builder prepends 4 bytes
		// the header is the last 16 bytes of message
		// so I did [^1] 32 bytes allocation anyway
		// I have no idea why it does that
		// _ = builder.FinishedBytes()
	}
}

func assertEqual(t *T, in, out interface{}) {
	if in != out {
		t.Helper()
		t.Fatalf("expected %v got %v", in, out)
	}
}
