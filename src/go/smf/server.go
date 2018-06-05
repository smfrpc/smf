package smf

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"log"
	"net"
)

// Server - SMF RPC server.
type Server struct {
	services []Service
}

// RegisterService - Registers service on a server.
func (server *Server) RegisterService(service Service) {
	server.services = append(server.services, service)
}

// MethodHandle - Returns method handle for request ID.
// The handle is nil if the request ID is not recognized.
func (server *Server) MethodHandle(id uint32) (handle RawHandle) {
	for _, service := range server.services {
		handle = service.MethodHandle(id)
		if handle != nil {
			return
		}
	}
	return
}

// HandleConnection - Handles accepted connection.
func (server *Server) HandleConnection(conn net.Conn) error {
	writer := bufio.NewWriter(conn)

	defer func() {
		if err := writer.Flush(); err != nil {
			log.Printf("Connection flush error: %v", err)
		}
		if err := conn.Close(); err != nil {
			log.Printf("Connection close error: %v", err)
		}
	}()

	for {
		header, req, err := ReceivePayload(conn)
		if err != nil {
			return err
		}

		handle := server.MethodHandle(header.Meta())
		if handle == nil {
			return fmt.Errorf("Method ID %d not found", header.Meta())
		}

		resp, err := handle(context.TODO(), req) // TODO: context
		if err != nil {
			// TODO: application errors
			return err
		}

		err = WritePayload(writer, header.Session(), resp, 200)
		if err != nil {
			return err
		}

		err = writer.Flush()
		if err != nil {
			return err
		}
	}
}

// Serve - Starts accepting connections on the listener and serving.
func (server *Server) Serve(ln net.Listener) (err error) {
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Printf("Connection accept error: %v", err)
			continue
		}
		go func() {
			if err := server.HandleConnection(conn); err != nil && err != io.EOF {
				log.Printf("Connection error: %v", err)
			}
		}()
	}
}

// ListenAndServe - Starts listening on given address and serves connections.
func (server *Server) ListenAndServe(network, address string) (err error) {
	ln, err := net.Listen(network, address)
	if err != nil {
		return
	}
	return server.Serve(ln)
}
