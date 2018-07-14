package smf.client.core;

import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioSocketChannel;
import io.netty.handler.logging.LogLevel;
import io.netty.handler.logging.LoggingHandler;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.common.compression.CompressionService;

import java.nio.ByteBuffer;
import java.util.concurrent.*;

public class SmfClient {
    private final static Logger LOG = LogManager.getLogger();

    private EventLoopGroup group;
    private final Bootstrap bootstrap;
    private final Dispatcher dispatcher;
    private volatile Channel channel;
    private final SessionIdGenerator sessionIdGenerator;

    public SmfClient(final String host, final int port) throws InterruptedException {
        sessionIdGenerator = new SessionIdGenerator();
        group = new NioEventLoopGroup(1);
        dispatcher = new Dispatcher(sessionIdGenerator);

        final CompressionService compressionService = new CompressionService();

        final RpcRequestEncoder rpcRequestEncoder = new RpcRequestEncoder(compressionService);
        final RpcResponseDecoder rpcResponseDecoder = new RpcResponseDecoder(compressionService);

        bootstrap = new Bootstrap();
        bootstrap.group(group)
                .channel(NioSocketChannel.class)
                .option(ChannelOption.TCP_NODELAY, true)
                .handler(new ChannelInitializer<SocketChannel>() {
                    @Override
                    protected void initChannel(final SocketChannel ch) {
                        ChannelPipeline p = ch.pipeline();
                        //paranoid debug
//                        p.addLast("debug", new LoggingHandler(LogLevel.INFO));
                        p.addLast(rpcRequestEncoder);
                        p.addLast(rpcResponseDecoder);
                        p.addLast(dispatcher);
                    }
                });

        LOG.info("Going to connect to {} on port {}", host, port);
        ChannelFuture connect = bootstrap.connect(host, port);

        //ヽ( ͠°෴ °)ﾉ
        connect.addListener(result -> channel = connect.channel());
        //fixme not best solution - but most important is to have working client
        connect.sync().await();
    }

    /**
     * schedule RPC call and assign callback invocation to feature result of scheduled request.
     *
     * @param methodMeta
     * @param body
     * @return CompletableFuture representing result of RPC request.
     */
    public CompletableFuture<ByteBuffer> executeAsync(long methodMeta, byte[] body) {
        final CompletableFuture<ByteBuffer> resultFuture = new CompletableFuture<>();
        int sessionId = sessionIdGenerator.next();
        LOG.info("Constructing RPC call for sessionId {}", sessionId);
        final PreparedRpcRequest preparedRpcRequest = new PreparedRpcRequest(sessionId, methodMeta, body, resultFuture);
        dispatcher.assignCallback(sessionId, preparedRpcRequest.getResultFuture());
        //TODO channel.isWritable() has to be checked before.
        channel.writeAndFlush(preparedRpcRequest);
        return resultFuture;
    }

    public void closeGracefully() throws InterruptedException {
        group.shutdownGracefully().await().sync();
    }

}
