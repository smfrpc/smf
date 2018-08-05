package smf.common.transport;

import io.netty.bootstrap.Bootstrap;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.*;
import io.netty.channel.epoll.Epoll;
import io.netty.channel.epoll.EpollEventLoopGroup;
import io.netty.channel.epoll.EpollServerSocketChannel;
import io.netty.channel.epoll.EpollSocketChannel;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import io.netty.channel.socket.nio.NioSocketChannel;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Factory responsible for creating transport layer for Netty bootstrap.
 * If possible native transport will be preferred.
 * <p>
 * TODO allow to pass Options (socket, threads count, etc.)
 */
public class BootstrapFactory {
    private final static Logger LOG = LogManager.getLogger();

    public static ClientTransport getClientBootstrap() {
        LOG.info("Going to create client bootstrap.");

        final Bootstrap bootstrap = new Bootstrap();

        EventLoopGroup group;
        Class<? extends Channel> channelClass;
        boolean isNative;
        if (Epoll.isAvailable()) {
            LOG.info("Native epoll transport is available.");

            channelClass = EpollSocketChannel.class;
            group = new EpollEventLoopGroup(1);
            isNative = true;
        } else {
            LOG.info("Default transport layer used.");

            channelClass = NioSocketChannel.class;
            group = new NioEventLoopGroup(1);
            isNative = false;
        }

        bootstrap.group(group)
                .channel(channelClass)
                .option(ChannelOption.TCP_NODELAY, true);

        return new ClientTransport(isNative, group, bootstrap);
    }

    public static ServerTransport getServerBootstrap() {
        LOG.info("Going to create server bootstrap.");

        final ServerBootstrap serverBootstrap = new ServerBootstrap();

        EventLoopGroup bossGroup;
        EventLoopGroup workerGroup;

        Class<? extends ServerChannel> channelClass;
        boolean isNative;
        if (Epoll.isAvailable()) {
            LOG.info("Native epoll transport is available.");

            channelClass = EpollServerSocketChannel.class;
            bossGroup = new EpollEventLoopGroup(1);
            workerGroup = new EpollEventLoopGroup();
            isNative = true;
        } else {
            LOG.info("Default transport layer used.");

            channelClass = NioServerSocketChannel.class;
            bossGroup = new NioEventLoopGroup(1);
            workerGroup = new NioEventLoopGroup();
            isNative = false;
        }

        serverBootstrap.group(bossGroup, workerGroup)
                .channel(channelClass);

        return new ServerTransport(isNative, bossGroup, workerGroup, serverBootstrap);
    }
}
