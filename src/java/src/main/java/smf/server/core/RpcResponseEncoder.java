package smf.server.core;

import com.google.flatbuffers.FlatBufferBuilder;
import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.MessageToMessageEncoder;
import net.openhft.hashing.LongHashFunction;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.CompressionFlags;
import smf.Header;
import smf.common.RpcRequest;
import smf.common.RpcResponse;
import smf.common.compression.CompressionService;

import java.util.List;

public class RpcResponseEncoder extends MessageToMessageEncoder<RpcResponse> {
    private final static Logger LOG = LogManager.getLogger();
    private final static long MAX_UNSIGNED_INT = (long) (Math.pow(2, 32) - 1);

    private final CompressionService compressionService;

    public RpcResponseEncoder(final CompressionService compressionService) {
        this.compressionService = compressionService;
    }

    @Override
    protected void encode(final ChannelHandlerContext ctx, final RpcResponse response, final List<Object> out) {

        final Header header = response.getHeader();

        if (LOG.isDebugEnabled()) {
            LOG.debug("[session {}] encoding RpcResponse", header.session());
        }

        final byte[] body = compressionService.compressBody(header.compression(), response.getBody());

        final long length = body.length;
        final long meta = header.meta();
        final int sessionId = header.session();
        final byte compression = header.compression();
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
