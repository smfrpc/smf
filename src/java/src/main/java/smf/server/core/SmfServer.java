package smf.server.core;

import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.common.compression.CompressionService;

public class SmfServer {
    private final static Logger LOG = LogManager.getLogger();

    private EventLoopGroup bossGroup;
    private EventLoopGroup workerGroup;
    private volatile Channel channel;
    private final RequestHandler requestHandler;

    public SmfServer(final String host, final int port) throws InterruptedException {
        bossGroup = new NioEventLoopGroup(1);
        workerGroup = new NioEventLoopGroup();

        requestHandler = new RequestHandler();

        final CompressionService compressionService = new CompressionService();

        ServerBootstrap b = new ServerBootstrap();
        b.group(bossGroup, workerGroup)
                .channel(NioServerSocketChannel.class)
                //.handler(new LoggingHandler(LogLevel.INFO))
                .childHandler(new ChannelInitializer() {
                    @Override
                    protected void initChannel(Channel ch) {
                        final ChannelPipeline pipeline = ch.pipeline();
                        //p.addLast("debug", new LoggingHandler(LogLevel.INFO));
                        pipeline.addLast(new RpcResponseEncoder(compressionService));
                        pipeline.addLast(new RpcRequestDecoder(compressionService));
                        pipeline.addLast(requestHandler);
                    }
                });

        LOG.info("Going to listen on {}:{}", host, port);

        channel = b.bind(host, port).sync().channel();
    }

    public void registerStorageService(final RpcService rpcService) {
        requestHandler.registerStorageService(rpcService);
    }

    public void closeGracefully() throws InterruptedException {
        bossGroup.shutdownGracefully().await().sync();
        workerGroup.shutdownGracefully().await().sync();
    }
}