package example.demo;

import com.google.flatbuffers.FlatBufferBuilder;
import smf.core.SmfClient;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.CyclicBarrier;

public class DemoApp {

    private final static Logger LOG = LogManager.getLogger();

    public static void main(String... args) throws InterruptedException {

        final SmfClient smfClient = new SmfClient("127.0.0.1", 7000);

        final SmfStorageClient smfStorageClient = new SmfStorageClient(smfClient);

        //lets schedule 1000 concurrent requests
        final int concurrentConCount = 1000;
        final CountDownLatch endLatch = new CountDownLatch(concurrentConCount);
        final CyclicBarrier cyclicBarrier = new CyclicBarrier(50);

        for (int i = 0; i < concurrentConCount; i++) {
            new Thread(() -> {

                try {
                    cyclicBarrier.await();
                } catch (Exception ex) {
                    LOG.error("Error occurred {}", ex);
                }

                //construct get request.
                final FlatBufferBuilder requestBuilder = new FlatBufferBuilder(0);
                final String currentThreadName = Thread.currentThread().getName();
                int requestPosition = requestBuilder.createString("GET /something/ " + currentThreadName);

                example.demo.Request.startRequest(requestBuilder);
                example.demo.Request.addName(requestBuilder, requestPosition);
                final int root = example.demo.Request.endRequest(requestBuilder);
                requestBuilder.finish(root);

                final byte[] request = requestBuilder.sizedByteArray();

                /**
                 * Be careful here - thenAccept will be called from Netty EventLoop !
                 */
                smfStorageClient.get(request)
                        .thenAccept(response -> {
                            LOG.info("[{}] Got parsed response {}", Thread.currentThread().getName(), response.name());
                            endLatch.countDown();
                        });

            }).start();

        }

        //await response
        endLatch.await();

        //close client
        smfClient.closeGracefully();
    }
}
