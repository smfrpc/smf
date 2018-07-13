package example.demo.client;

import example.demo.Response;
import smf.client.core.SmfClient;

import java.util.concurrent.CompletableFuture;

public class SmfStorageClient {

    private static long GET_METHOD_META = 212494116 ^ 1719559449;

    private final SmfClient smfClient;

    public SmfStorageClient(final SmfClient smfClient) {
        this.smfClient = smfClient;
    }

    public CompletableFuture<Response> get(final byte[] body) {
        return smfClient.executeAsync(GET_METHOD_META, body)
                .thenApply(Response::getRootAsResponse);
    }
}
