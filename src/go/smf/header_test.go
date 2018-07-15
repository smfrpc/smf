package smf

import (
	cryptorand "crypto/rand"
	"io"
	"math"
	"math/rand"
	. "testing"

	"github.com/cespare/xxhash"
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

func assertEqual(t *T, in, out interface{}) {
	if in != out {
		t.Helper()
		t.Fatalf("expected %v got %v", in, out)
	}
}
