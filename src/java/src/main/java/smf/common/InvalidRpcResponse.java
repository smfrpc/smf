package smf.common;

import smf.Header;

/**
 * Instance of {@class smf.client.core.InvalidRpcResponse} allows to propagate parsing exception downstream to
 * next handlers.
 */
public class InvalidRpcResponse extends RpcResponse {
    final Throwable cause;
    public InvalidRpcResponse(final Header header, final Throwable cause) {
        super(header, null);
        this.cause = cause;
    }

    public Throwable getCause()
    {
        return cause;
    }
}
