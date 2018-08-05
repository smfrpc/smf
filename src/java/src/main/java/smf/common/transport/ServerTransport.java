package smf.common.transport;

import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.EventLoopGroup;

import java.util.Objects;

/**
 * Wrapper around key elements required by SmfServer transport layer.
 */
public class ServerTransport {

    private final boolean isNative;
    private final EventLoopGroup bossGroup;
    private final EventLoopGroup workerGroup;
    private final ServerBootstrap serverBootstrap;

    public ServerTransport(final boolean isNative, final EventLoopGroup bossGroup, final EventLoopGroup workerGroup, final ServerBootstrap serverBootstrap) {
        this.isNative = isNative;
        this.bossGroup = bossGroup;
        this.workerGroup = workerGroup;
        this.serverBootstrap = serverBootstrap;
    }

    public boolean isNative() {
        return isNative;
    }

    public EventLoopGroup getBossGroup() {
        return bossGroup;
    }

    public EventLoopGroup getWorkerGroup() {
        return workerGroup;
    }

    public ServerBootstrap getServerBootstrap() {
        return serverBootstrap;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ServerTransport that = (ServerTransport) o;
        return isNative == that.isNative &&
                Objects.equals(bossGroup, that.bossGroup) &&
                Objects.equals(workerGroup, that.workerGroup) &&
                Objects.equals(serverBootstrap, that.serverBootstrap);
    }

    @Override
    public int hashCode() {

        return Objects.hash(isNative, bossGroup, workerGroup, serverBootstrap);
    }

}
