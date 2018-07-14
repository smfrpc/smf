package smf.common.compression;

import com.github.luben.zstd.Zstd;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.CompressionFlags;

import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * Provide simple interface for compression/decompression operations using algorithms supported by SMF.
 */
public class CompressionService {

    private final static Logger LOG = LogManager.getLogger();

    private final Zstd zstd;

    public CompressionService() {
        /**
         * Lack of documentation for this - but I assume thread-safty :D
         */
        this.zstd = new Zstd();
    }

    /**
     * Based on {@param compressionFlags} compress {@param body} using appropriate compression algorithm or
     * do nothing if compression was not requested.
     *
     * @return processed {@param body}, in case of no compression, just received array is returned.
     */
    public byte[] compressBody(byte compressionFlags, byte[] body) {
        switch (compressionFlags) {
            case CompressionFlags.Disabled:
            case CompressionFlags.None:
                return body;
            case CompressionFlags.Zstd:
                return compressUsingZstd(body);
            default:
                throw new UnsupportedOperationException("Compression algorithm is not supported");
        }
    }

    /**
     * @see {@link #compressBody(byte, byte[]) processBody}
     */
    public byte[] compressBody(byte compressionFlags, final ByteBuffer body) {
        final byte[] bodyArray = new byte[body.remaining()];
        body.get(bodyArray);
        return compressBody(compressionFlags, bodyArray);
    }

    private byte[] compressUsingZstd(byte[] body) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("Compressing using ZSTD");
        }

        byte[] dstBuff = new byte[(int) zstd.compressBound(body.length)];
        int bytesWritten = (int) zstd.compress(dstBuff, body, 0);
        byte[] onlyCompressedBytes = Arrays.copyOfRange(dstBuff, 0, bytesWritten);
        return onlyCompressedBytes;
    }

    /**
     * Based on {@param compressionFlags} decompress {@param body} using appropriate compression algorithm or
     * do nothing if decompression was not requested.
     *
     * @return processed {@param body}, in case of no compression, just received array is returned.
     */
    public byte[] decompressBody(byte compressionFlags, byte[] body) {
        switch (compressionFlags) {
            case CompressionFlags.Disabled:
            case CompressionFlags.None:
                return body;
            case CompressionFlags.Zstd:
                return decompressUsingZstd(body);
            default:
                throw new UnsupportedOperationException("Compression algorithm is not supported");
        }
    }

    private byte[] decompressUsingZstd(byte[] body) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("Decompressing using ZSTD");
        }

        long decompressedSize = zstd.decompressedSize(body);
        byte[] decompressedDst = new byte[(int) decompressedSize];
        zstd.decompress(decompressedDst, body);
        return decompressedDst;
    }

}
