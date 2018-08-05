package smf.common.exceptions;

public class ClosedRpcRequestException extends RuntimeException {
    public ClosedRpcRequestException(final String message) {
        super(message);
    }
}
