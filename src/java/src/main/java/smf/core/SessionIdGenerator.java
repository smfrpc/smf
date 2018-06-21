package smf.core;

import smf.core.exceptions.SessionIdGenerationException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Generate sessionId with max to {@param USHRT_MAX} because of SMF sessionsId max value restrictions.
 * SessionIdGenerator is Thread Safe and can be used from multiple threads - the only one restriction is that,
 * it should be issued once per SMF-Client to prevent session collision inside single JVM.
 * Performance : this is separate issue that will be fixed in https://github.com/KowalczykBartek/smf-java/issues/3
 */
public class SessionIdGenerator {
    private Boolean TRUE = new Boolean(true);
    private static final byte MAX_RETRIES = 5;

    private final static Logger LOG = LogManager.getLogger();
    private static final int USHRT_MAX_PLUS_ONE = 65536;

    private final ConcurrentHashMap<Integer, Boolean> pendingSessions = new ConcurrentHashMap<>();
    private final AtomicInteger sessionIdGen = new AtomicInteger(0);

    /**
     * Generate new sessionId not used by any pending request. Method is thread safe and doesn't block.
     * Internally sessionId is assigned in "compare and set" manner, and if this assignment will
     * fail {@param MAX_RETRIES} times, SessionIdGenerationException exception will be thrown.
     *
     * @return sessionId that can be used by RPC request.
     * @throws SessionIdGenerationException if method cannot generate sessionId because of thread contention.
     */
    public int next() {
        int iteration = 0;

        while (true) {

            /**
             * Even if sessionIdGen will turn over and two threads will get the same number, putIfAbsent is atomic,
             * and one of those threads will fail.
             */
            int sessionIdAttempt = sessionIdGen.incrementAndGet() % USHRT_MAX_PLUS_ONE;

            if (pendingSessions.putIfAbsent(sessionIdAttempt, TRUE) == null) {

                if (LOG.isDebugEnabled()) {
                    LOG.debug("Generated sessionId : {} ", sessionIdAttempt);
                }

                return sessionIdAttempt;
            } else {
                if (LOG.isDebugEnabled()) {
                    LOG.debug("SessionID MISS ! " +
                            "Other thread generated exact sessionId ( {}) and was faster to assign it", sessionIdAttempt);
                }
            }

            if (iteration > MAX_RETRIES) {
                throw new SessionIdGenerationException("Cannot generate sessionId because of " +
                        "height contention with other threads ! " + MAX_RETRIES + " performed");
            }

            ++iteration;
        }

    }

    public void release(final int sessionId) {
        pendingSessions.remove(sessionId);
    }

}
