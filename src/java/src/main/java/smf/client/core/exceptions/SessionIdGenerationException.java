package smf.client.core.exceptions;

/**
 * Represents problem with sessionId generation.
 * Most likely that this exception is thrown because of high contention during session generation.
 */
public class SessionIdGenerationException extends RuntimeException {
    public SessionIdGenerationException(final String message) {
        super(message);
    }
}
