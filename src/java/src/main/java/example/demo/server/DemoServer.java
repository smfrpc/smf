package example.demo.server;

import com.google.flatbuffers.FlatBufferBuilder;
import example.demo.Request;
import example.demo.Response;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.server.core.RpcService;
import smf.server.core.SmfServer;

import java.util.function.Function;

public class DemoServer {
    private final static Logger LOG = LogManager.getLogger();

    public static void main(final String... args) throws InterruptedException {
        final SmfServer smfServer = new SmfServer("127.0.0.1", 7000);

        LOG.debug("Listening for requests ! comon !");

        final StorageService storageService = new StorageService();
        smfServer.registerStorageService(storageService);
    }
}

/**
 * First view of how RpcService can be implemented.
 */
class StorageService implements RpcService {
    private final static Logger LOG = LogManager.getLogger();

    private static long GET_METHOD_META = 212494116 ^ 1719559449;

    private final Function<byte[], byte[]> RESPONSE_HANDLER = (request) -> {
        final FlatBufferBuilder responseBuilder = new FlatBufferBuilder(0);
        final String currentThreadName = Thread.currentThread().getName();
        LOG.info("[{}] Handling incoming request ! plain : {}", currentThreadName, new String(request));
        int responsePosition = responseBuilder.createString("RESPONSE FROM JAVA-SERVER! ");
        Response.startResponse(responseBuilder);
        Response.addName(responseBuilder, responsePosition);
        final int root = Request.endRequest(responseBuilder);
        responseBuilder.finish(root);
        final byte[] response = responseBuilder.sizedByteArray();
        return response;
    };

    @Override
    public String getServiceName() {
        return "StorageService";
    }

    @Override
    public long getServiceId() {
        return 212494116;
    }

    @Override
    public Function<byte[], byte[]> getHandler(long id) {
        if (id == GET_METHOD_META) {
            return RESPONSE_HANDLER;
        }

        return null;
    }
}