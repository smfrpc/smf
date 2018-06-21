package smf.core;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.ByteToMessageDecoder;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

/**
 * Parse incoming byte-stream into logical [smf.Header + response] pairs.
 * Logic behind is simple and of course not efficient - but is highly probable that it just works.
 *
 * RpcCallDecoder will try to decode each received bytes at once, if this operation fails, it will postpone
 * this operation and try when next chunk arrive.
 */
public class RpcCallDecoder extends ByteToMessageDecoder {
    private final static Logger LOG = LogManager.getLogger();

    @Override
    protected void decode(ChannelHandlerContext ctx, ByteBuf response, List<Object> out) {

        response.markReaderIndex();
        response.markWriterIndex();

        try {
            byte[] hdrbytes = new byte[16];
            response.readBytes(hdrbytes);
            ByteBuffer bb = ByteBuffer.wrap(hdrbytes);
            bb.order(ByteOrder.LITTLE_ENDIAN);
            smf.Header header = new smf.Header();
            header.__init(0, bb);

            final byte[] responseBody = new byte[(int) header.size()];
            response.readBytes(responseBody);

            LOG.debug("[session {}] Decoding response", header.session());

            out.add(new RpcResponse(header, ByteBuffer.wrap(responseBody)));

        } catch (final Exception ex) {
            if (LOG.isDebugEnabled()) {
                LOG.debug("Failed to parse ! postpone ...");

            }
            response.resetReaderIndex();
            response.resetWriterIndex();
        }

    }
}