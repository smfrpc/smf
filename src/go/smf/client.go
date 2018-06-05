package smf

import (
	"bufio"
	"net"
)

// Client - SMF Client connection.
type Client struct {
	conn  net.Conn
	bufio *bufio.Writer
	sess  uint16
}

// Dial - Dials network address and returns SMF RPC client.
func Dial(network, addr string) (client *Client, err error) {
	conn, err := net.Dial(network, addr)
	if err != nil {
		return
	}
	return NewClient(conn), nil
}

// NewClient - Creates new client from connection.
func NewClient(conn net.Conn) *Client {
	writer := bufio.NewWriter(conn)
	return &Client{
		conn:  conn,
		bufio: writer,
	}
}

// SendRecv - Sends request and receives a response.
func (client *Client) SendRecv(req []byte, meta uint32) (resp []byte, err error) {
	err = client.Send(req, meta)
	if err != nil {
		return
	}
	_, resp, err = client.Recv()
	return
}

// Send - Sends request with header to a connection.
func (client *Client) Send(req []byte, meta uint32) (err error) {
	client.sess++ // TODO(crackcomm): atomic or else
	err = WritePayload(client.bufio, client.sess, req, meta)
	if err != nil {
		return
	}
	return client.bufio.Flush()
}

// Recv - Receives response with header.
func (client *Client) Recv() (*Header, []byte, error) {
	return ReceivePayload(client.conn)
}

// Close - Closes client connetion.
func (client *Client) Close() error {
	return client.conn.Close()
}
