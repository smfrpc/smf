package smf.server.core;

import io.netty.channel.ChannelHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import smf.Header;
import smf.common.RpcRequest;
import smf.common.RpcResponse;

import java.nio.ByteBuffer;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.Function;

@ChannelHandler.Sharable
public class RequestHandler extends SimpleChannelInboundHandler<RpcRequest> {
    final CopyOnWriteArrayList<RpcService> serviceIdToFunctionHandler = new CopyOnWriteArrayList<>();

    @Override
    protected void channelRead0(final ChannelHandlerContext ctx, final RpcRequest request) {
        final Header header = request.getHeader();
        long meta = header.meta();

        Optional<Function<byte[], byte[]>> requestHandler = serviceIdToFunctionHandler.stream()
                .map(rpcService -> rpcService.getHandler(meta))//
                .filter(Objects::nonNull)//
                .findFirst();

        if (!requestHandler.isPresent()) {
            //TODO handler the case where meta is not registered
        }

        Function<byte[], byte[]> rpcRequestFunction = requestHandler.get();

        final byte[] body = new byte[request.getBody().remaining()];
        request.getBody().get(body);

        byte[] response = rpcRequestFunction.apply(body);

        //kek XD
        final RpcResponse rpcResponse = new RpcResponse(header, ByteBuffer.wrap(response));
        ctx.channel().writeAndFlush(rpcResponse);
    }

    public void registerStorageService(final RpcService rpcService) {
        serviceIdToFunctionHandler.add(rpcService);
    }
}
