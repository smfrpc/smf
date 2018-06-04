package smf

// Service - Service interface.
type Service interface {
	// ServiceName - Returns service name.
	ServiceName() string

	// ServiceID - Returns service ID.
	ServiceID() uint32

	// MethodHandle - Returns method handle for request ID.
	// The handle is nil if the request ID is not recognized.
	MethodHandle(id uint32) RawHandle
}
