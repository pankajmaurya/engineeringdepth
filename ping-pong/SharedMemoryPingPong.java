import java.io.File;
import java.io.RandomAccessFile;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.time.Instant;

public class SharedMemoryPingPong {
    static final String FILE_PATH = "/tmp/pingpong-shm.dat";
    static final int SIZE = 1; // 1 byte for flag

    public static void main(String[] args) throws Exception {
        if (args.length != 1 || (!args[0].equals("ping") && !args[0].equals("pong"))) {
            System.err.println("Usage: java SharedMemoryPingPong <ping|pong>");
            return;
        }

        boolean isPing = args[0].equals("ping");
        System.out.println("Starting " + (isPing ? "PING" : "PONG"));

        // Prepare shared memory file
        File file = new File(FILE_PATH);
        if (isPing && !file.exists()) {
            try (RandomAccessFile raf = new RandomAccessFile(file, "rw")) {
                raf.setLength(SIZE);
                raf.write(0); // init to ping's turn
            }
        }

        try (RandomAccessFile raf = new RandomAccessFile(file, "rw");
             FileChannel channel = raf.getChannel()) {

            MappedByteBuffer buffer = channel.map(FileChannel.MapMode.READ_WRITE, 0, SIZE);

            int rounds = 0;
            long start = System.nanoTime();

            while (true) {
                byte flag = buffer.get(0);

                if (isPing && flag == 0) {
                    buffer.put(0, (byte) 1);
                    rounds++;
                } else if (!isPing && flag == 1) {
                    buffer.put(0, (byte) 0);
                    rounds++;
                }

                // Reporting every 10k
                if (rounds % 10_000 == 0 && rounds > 0) {
                    long now = System.nanoTime();
                    double elapsedSec = (now - start) / 1_000_000_000.0;
                    double latencyUs = (now - start) / 1000.0 / rounds;
                    double throughput = rounds / elapsedSec;
                    System.out.printf("%s: %d rounds, Avg latency: %.2f µs, Throughput: %.2f rounds/sec%n",
                            isPing ? "PING" : "PONG", rounds, latencyUs, throughput);
                }
            }
        }
    }
}
