package smf

import (
	"context"
)

// RawHandle - Raw smf RPC method handle.
type RawHandle func(context.Context, []byte) ([]byte, error)
