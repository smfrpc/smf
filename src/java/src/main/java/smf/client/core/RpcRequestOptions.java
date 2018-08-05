package smf.client.core;

import java.util.Objects;

/**
 * FIXME/TODO make fluent builder
 */
public class RpcRequestOptions {
    private final byte compression;

    public RpcRequestOptions(final byte compression) {
        this.compression = compression;
    }

    public byte getCompression() {
        return compression;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        RpcRequestOptions that = (RpcRequestOptions) o;
        return compression == that.compression;
    }

    @Override
    public int hashCode() {

        return Objects.hash(compression);
    }

}
