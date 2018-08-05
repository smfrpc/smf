package smf.common;

import smf.Header;

import java.nio.ByteBuffer;
import java.util.Objects;

public class RpcResponse {
    private final Header header;
    private final ByteBuffer body;

    public RpcResponse(final Header header, final ByteBuffer body) {
        this.header = header;
        this.body = body;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        RpcResponse that = (RpcResponse) o;
        return Objects.equals(header, that.header) &&
                Objects.equals(body, that.body);
    }

    public Header getHeader() {
        return header;
    }

    public ByteBuffer getBody() {
        return body;
    }

    @Override
    public int hashCode() {

        return Objects.hash(header, body);
    }
}
