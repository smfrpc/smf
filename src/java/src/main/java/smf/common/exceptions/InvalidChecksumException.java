package smf.common.exceptions;

public class InvalidChecksumException extends RuntimeException {
    public InvalidChecksumException(final String message) {
        super(message);
    }
}
