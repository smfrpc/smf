package smf.core;

import com.google.flatbuffers.FlatBufferBuilder;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.MessageToMessageEncoder;
import net.openhft.hashing.LongHashFunction;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import smf.Header;

import java.util.List;

public class RpcCallEncoder extends MessageToMessageEncoder<RpcRequest> {
    private final static Logger LOG = LogManager.getLogger();
    private final static long MAX_UNSIGNED_INT = (long) (Math.pow(2, 32) - 1);

    @Override
    protected void encode(final ChannelHandlerContext ctx, final RpcRequest msg, final List<Object> out) {

        LOG.info("[session {}] encoding RpcRequest", msg.getSessionId());

        final byte[] body = msg.getBody();
        final long length = body.length;
        final long meta = msg.getMethodMeta();
        final int sessionId = msg.getSessionId();
        final byte compression = (byte) 0;
        final byte bitFlags = (byte) 0;

        final long maxUnsignedInt = MAX_UNSIGNED_INT;
        final long checkSum = maxUnsignedInt & LongHashFunction.xx().hashBytes(body);

        final FlatBufferBuilder internalRequest = new FlatBufferBuilder(20);
        int headerPosition = Header.createHeader(internalRequest, compression, bitFlags, sessionId, length, checkSum, meta);
        internalRequest.finish(headerPosition);
        byte[] bytes = internalRequest.sizedByteArray();

        byte[] dest = new byte[16];

        //fixme - I cannot even comment on this (｡◕‿‿◕｡)
        System.arraycopy(bytes, 4, dest, 0, 16);

        final ByteBuf byteBuf = ctx.alloc().heapBuffer()
                .writeBytes(dest)
                .writeBytes(body);

        out.add(byteBuf);
    }
}
