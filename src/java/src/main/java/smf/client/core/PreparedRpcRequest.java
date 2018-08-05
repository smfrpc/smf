package smf.client.core;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;

/**
 * This is not called RpcRequest because it :
 * 1. doesn't contain full Header
 * 2. contain stuff not related to requests itself.
 */
public class PreparedRpcRequest {
    private final int sessionId;
    private final long methodMeta;
    private final byte[] body;
    private final CompletableFuture<ByteBuffer> resultFuture;
    private final RpcRequestOptions rpcRequestOptions;

    public PreparedRpcRequest(final int sessionId, long methodMeta, byte[] body, final CompletableFuture<ByteBuffer> resultFuture, final RpcRequestOptions rpcRequestOptions) {
        this.sessionId = sessionId;
        this.methodMeta = methodMeta;
        this.body = body;
        this.resultFuture = resultFuture;
        this.rpcRequestOptions = rpcRequestOptions;
    }

    public int getSessionId() {
        return sessionId;
    }

    public long getMethodMeta() {
        return methodMeta;
    }

    public byte[] getBody() {
        return body;
    }

    public RpcRequestOptions getRpcRequestOptions() {
        return rpcRequestOptions;
    }

    public CompletableFuture<ByteBuffer> getResultFuture() {
        return resultFuture;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        PreparedRpcRequest that = (PreparedRpcRequest) o;
        return sessionId == that.sessionId &&
                methodMeta == that.methodMeta &&
                Arrays.equals(body, that.body) &&
                Objects.equals(resultFuture, that.resultFuture) &&
                Objects.equals(rpcRequestOptions, that.rpcRequestOptions);
    }

    @Override
    public int hashCode() {

        int result = Objects.hash(sessionId, methodMeta, resultFuture, rpcRequestOptions);
        result = 31 * result + Arrays.hashCode(body);
        return result;
    }
}
