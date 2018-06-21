package smf.core;

import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.Header;

import java.nio.ByteBuffer;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Dispatcher is responsible for managing callbacks based on sessionId (fixme ensure sessionId uniques)
 * Inspired by Dispatcher inside Datastax's Cassandra Java Driver.
 */
public class Dispatcher extends SimpleChannelInboundHandler<RpcResponse> {
    private final static Logger LOG = LogManager.getLogger();
    private final ConcurrentHashMap<Integer, CompletableFuture<ByteBuffer>> pendingRpcCalls = new ConcurrentHashMap<>();
    private SessionIdGenerator sessionIdGenerator;

    public Dispatcher(final SessionIdGenerator sessionIdGenerator) {
        this.sessionIdGenerator = sessionIdGenerator;
    }

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, RpcResponse msg) {
        LOG.debug("[session {}] received to dispatch", msg.getHeader().session());
        final Header header = msg.getHeader();
        final CompletableFuture<ByteBuffer> resultFuture = pendingRpcCalls.remove(header.session());

        if (resultFuture == null) {
            LOG.debug("[session {}] registered handler is null", msg.getHeader().session());
        } else {
            try {
                //FIXME should it be called within event loop ?
                resultFuture.complete(msg.getResponseBody());
            } catch (final Exception ex) {
                resultFuture.completeExceptionally(ex);
            }
        }
        sessionIdGenerator.release(header.session());
    }

    public void assignCallback(final int sessionId, final CompletableFuture<ByteBuffer> resultFuture) {
        pendingRpcCalls.put(sessionId, resultFuture);
    }

}
