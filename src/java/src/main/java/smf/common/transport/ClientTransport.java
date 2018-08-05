package smf.common.transport;

import io.netty.bootstrap.Bootstrap;
import io.netty.channel.EventLoopGroup;

import java.util.Objects;

/**
 * Wrapper around key elements required by SmfClient transport layer.
 */
public class ClientTransport {
    private final boolean isNative;
    private final EventLoopGroup group;
    private final Bootstrap bootstrap;

    public ClientTransport(final boolean isNative, final EventLoopGroup group, final Bootstrap bootstrap) {
        this.isNative = isNative;
        this.group = group;
        this.bootstrap = bootstrap;
    }

    public boolean isNative() {
        return isNative;
    }

    public EventLoopGroup getGroup() {
        return group;
    }

    public Bootstrap getBootstrap() {
        return bootstrap;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ClientTransport that = (ClientTransport) o;
        return isNative == that.isNative &&
                Objects.equals(group, that.group) &&
                Objects.equals(bootstrap, that.bootstrap);
    }

    @Override
    public int hashCode() {

        return Objects.hash(isNative, group, bootstrap);
    }

}
