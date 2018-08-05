package smf.client.core;

import com.google.flatbuffers.FlatBufferBuilder;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.MessageToMessageEncoder;
import net.openhft.hashing.LongHashFunction;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import smf.CompressionFlags;
import smf.Header;
import smf.common.compression.CompressionService;

import java.util.List;

public class RpcRequestEncoder extends MessageToMessageEncoder<PreparedRpcRequest> {
    private final static Logger LOG = LogManager.getLogger();
    private final static long MAX_UNSIGNED_INT = (long) (Math.pow(2, 32) - 1);

    private final CompressionService compressionService;

    public RpcRequestEncoder(final CompressionService compressionService) {
        this.compressionService = compressionService;
    }

    @Override
    protected void encode(final ChannelHandlerContext ctx, final PreparedRpcRequest msg, final List<Object> out) {

        if (LOG.isDebugEnabled()) {
            LOG.debug("[session {}] encoding PreparedRpcRequest", msg.getSessionId());
        }

        final RpcRequestOptions rpcRequestOptions = msg.getRpcRequestOptions();

        final byte[] body = compressionService.compressBody(rpcRequestOptions.getCompression(), msg.getBody());

        final long length = body.length;
        final long meta = msg.getMethodMeta();
        final int sessionId = msg.getSessionId();
        final byte compression = rpcRequestOptions.getCompression();
        final byte bitFlags = (byte) 0;

        final long maxUnsignedInt = MAX_UNSIGNED_INT;
        final long checkSum = maxUnsignedInt & LongHashFunction.xx().hashBytes(body);

        final FlatBufferBuilder internalRequest = new FlatBufferBuilder(20);
        int headerPosition = Header.createHeader(internalRequest, compression, bitFlags, sessionId, length, checkSum, meta);
        internalRequest.finish(headerPosition);
        byte[] bytes = internalRequest.sizedByteArray();

        byte[] dest = new byte[16];

        //fixme - I cannot even comment on this ( ͡° ͜ʖ ͡° )つ──☆*:・ﾟ
        System.arraycopy(bytes, 4, dest, 0, 16);

        final ByteBuf byteBuf = ctx.alloc().heapBuffer()
                .writeBytes(dest)
                .writeBytes(body);

        out.add(byteBuf);
    }
}
