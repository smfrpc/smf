package smf.common.compression;

import com.github.luben.zstd.Zstd;
import net.jpountz.lz4.LZ4Compressor;
import net.jpountz.lz4.LZ4Factory;
import net.jpountz.lz4.LZ4FastDecompressor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import smf.CompressionFlags;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * Provide simple interface for compression/decompression operations using algorithms supported by SMF.
 */
public class CompressionService {

    private final static Logger LOG = LogManager.getLogger();
    private static final LZ4Factory blazingFastFactory = LZ4Factory.fastestInstance();

    private final Zstd zstd;
    private final LZ4Compressor lz4Compressor;
    private final LZ4FastDecompressor lz4Decompressor;

    public CompressionService() {
        /**
         * Lack of documentation for this - but I assume thread-safty :D
         */
        this.zstd = new Zstd();
        this.lz4Compressor = blazingFastFactory.fastCompressor();
        this.lz4Decompressor = blazingFastFactory.fastDecompressor();
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
            case CompressionFlags.Lz4:
                return compressUsingLz4(body);
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
            case CompressionFlags.Lz4:
                return decompressUsingLz4(body);
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

    private byte[] decompressUsingLz4(byte[] body) {
        final ByteBuffer wrappedBody = ByteBuffer.wrap(body);
        /**
         * I have doubt about this int-long exchangeability (because SMF c++ uses unsigned-int), but, even if
         * we will receive more than max Java's INT, it will not be able to handle such bit array :/
         */
        int originalSize = wrappedBody.order(ByteOrder.LITTLE_ENDIAN).getInt();

        byte[] decompressed = lz4Decompressor.decompress(body, 4, originalSize);
        return decompressed;
    }

    private byte[] compressUsingLz4(byte[] body) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("Compressing using LZ4");
        }

        int maxCompressedLength = lz4Compressor.maxCompressedLength(body.length);
        byte[] compressed = new byte[maxCompressedLength];
        int compressedLength = lz4Compressor.compress(body, 0, body.length, compressed, 0, maxCompressedLength);

        final byte[] finalArray = new byte[compressedLength + 4];

        ByteBuffer wrap = ByteBuffer.wrap(finalArray);
        wrap.order(ByteOrder.LITTLE_ENDIAN);
        wrap.putInt(body.length);

        System.arraycopy(compressed, 0, finalArray, 4, compressedLength);

        return finalArray;
    }

}
