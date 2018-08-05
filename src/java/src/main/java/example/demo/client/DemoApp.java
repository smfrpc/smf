package example.demo.client;

import com.google.flatbuffers.FlatBufferBuilder;
import example.demo.Request;
import smf.client.core.SmfClient;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.concurrent.CountDownLatch;

public class DemoApp {

    private final static Logger LOG = LogManager.getLogger();

    public static void main(String... args) throws InterruptedException {

        final SmfClient smfClient = new SmfClient("127.0.0.1", 7000);
        final SmfStorageClient smfStorageClient = new SmfStorageClient(smfClient);

        final CountDownLatch endLatch = new CountDownLatch(1);

        //construct get request.
        final FlatBufferBuilder requestBuilder = new FlatBufferBuilder(0);
        final String currentThreadName = Thread.currentThread().getName();
        int requestPosition = requestBuilder.createString("GET /something/ " + currentThreadName);

        Request.startRequest(requestBuilder);
        Request.addName(requestBuilder, requestPosition);
        final int root = Request.endRequest(requestBuilder);
        requestBuilder.finish(root);

        final byte[] request = requestBuilder.sizedByteArray();

        /**
         * Be careful here - thenAccept will be called from Netty EventLoop !
         */
        smfStorageClient.get(request)
                .thenAccept(response -> {
                    LOG.info("[{}] Got parsed response {}", Thread.currentThread().getName(), response.name());
                    endLatch.countDown();
                })
                .exceptionally(throwable -> {
                    LOG.info("[{}] Got exception {}", Thread.currentThread().getName(), throwable.getCause());
                    endLatch.countDown();
                    return null;
                });

        //await response
        endLatch.await();

        //close client
        smfClient.closeGracefully();
    }
}
