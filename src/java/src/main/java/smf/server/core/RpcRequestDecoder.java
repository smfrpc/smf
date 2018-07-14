package smf.server.core;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.ByteToMessageDecoder;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.common.RpcRequest;
import smf.common.compression.CompressionService;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

public class RpcRequestDecoder extends ByteToMessageDecoder {
    private final static Logger LOG = LogManager.getLogger();

    private final CompressionService compressionService;

    public RpcRequestDecoder(final CompressionService compressionService) {
        this.compressionService = compressionService;
    }

    @Override
    protected void decode(final ChannelHandlerContext ctx, final ByteBuf request, final List<Object> out) {
        /**
         * TODO FIXME merge it with encoder/decoder stuff from client.core - remove duplication
         */
        request.markReaderIndex();
        request.markWriterIndex();

        try {
            byte[] hdrbytes = new byte[16];
            request.readBytes(hdrbytes);
            ByteBuffer bb = ByteBuffer.wrap(hdrbytes);
            bb.order(ByteOrder.LITTLE_ENDIAN);
            smf.Header header = new smf.Header();
            header.__init(0, bb);

            final byte[] requestBody = new byte[(int) header.size()];
            request.readBytes(requestBody);

            //decompress if-needed
//            byte[] decompressBody = compressionService.decompressBody(header.compression(), requestBody);

            //FIXME wait to be unblock by SMF core end-to-end testing


            if (LOG.isDebugEnabled()) {
                LOG.debug("[session {}] Decoding response", header.session());
            }

            /**
             * header indicates size of received body, it will be different than body passed further
             * because decompression process.
             */
            out.add(new RpcRequest(header, ByteBuffer.wrap(requestBody)));

        } catch (final Exception ex) {
            if (LOG.isDebugEnabled()) {
                LOG.debug("Failed to parse ! postpone ...");

            }
            request.resetReaderIndex();
            request.resetWriterIndex();
        }

    }
}
