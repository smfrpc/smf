package smf

import (
	"io"
)

// ReceiveHeader - Reads smf RPC header from connection reader.
func ReceiveHeader(conn io.Reader) (hdr *Header, err error) {
	buf := make([]byte, 16)
	_, err = io.ReadFull(conn, buf)
	if err != nil {
		return
	}
	hdr = new(Header)
	hdr.Init(buf, 0)
	return
}

// ReceivePayload - Reads request header and body.
func ReceivePayload(conn io.Reader) (hdr *Header, req []byte, err error) {
	hdr, err = ReceiveHeader(conn)
	if err != nil {
		return
	}
	req = make([]byte, hdr.Size())
	_, err = io.ReadFull(conn, req)
	return
}

// WritePayload - Writes payload to io.Writer with header.
func WritePayload(w io.Writer, session uint16, body []byte, meta uint32) (err error) {
	head := BuildHeader(session, body, meta)
	if _, err := w.Write(head); err != nil {
		return err
	}
	if _, err := w.Write(body); err != nil {
		return err
	}
	return nil
}
