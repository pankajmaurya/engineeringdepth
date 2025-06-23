import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.*;
import java.util.concurrent.Exchanger;

public class PingPongBenchmark {

    static final int REPORT_INTERVAL = 1_000_000;
    static final int TOTAL_ITERATIONS = 1_000_000_000;

    interface Strategy {
        void start() throws Exception;
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            System.err.println("Usage: java PingPongBenchmark <strategy>");
            System.err.println("Available strategies: atomic, volatile, semaphore, condition, exchanger");
            return;
        }

        String strategy = args[0];
        Strategy impl;
        switch (strategy) {
            case "atomic": impl = new AtomicStrategy(); break;
            case "volatile": impl = new VolatileStrategy(); break;
            case "semaphore": impl = new SemaphoreStrategy(); break;
            case "condition": impl = new ConditionStrategy(); break;
            case "exchanger": impl = new ExchangerStrategy(); break;
            default:
                System.err.println("Unknown strategy: " + strategy);
                return;
        }

        System.out.println("Strategy: " + strategy);
        impl.start();
    }

    // ----------- Strategy 1: Atomic -------------
    static class AtomicStrategy implements Strategy {
        public void start() {
            AtomicBoolean pingTurn = new AtomicBoolean(true);
            Thread pong = new Thread(() -> {
                while (true) {
                    if (!pingTurn.get()) {
                        pingTurn.set(true);
                    }
                }
            });

            long startTime = System.nanoTime();
            pong.start();

            for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                while (!pingTurn.get()) {
                    Thread.onSpinWait();
                }
                pingTurn.set(false);

                if (i % REPORT_INTERVAL == 0) {
                    report(i, startTime);
                }
            }
            pong.stop(); // forcefully stop pong thread
        }
    }

    // ----------- Strategy 2: Volatile -------------
    static class VolatileStrategy implements Strategy {
        volatile boolean pingTurn = true;

        public void start() {
            Thread pong = new Thread(() -> {
                while (true) {
                    if (!pingTurn) {
                        pingTurn = true;
                    }
                }
            });

            long startTime = System.nanoTime();
            pong.start();

            for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                while (!pingTurn) {
                    Thread.onSpinWait();
                }
                pingTurn = false;

                if (i % REPORT_INTERVAL == 0) {
                    report(i, startTime);
                }
            }
            pong.stop();
        }
    }

    // ----------- Strategy 3: Semaphore -------------
    static class SemaphoreStrategy implements Strategy {
        public void start() {
            Semaphore pingSem = new Semaphore(1);
            Semaphore pongSem = new Semaphore(0);

            Thread pong = new Thread(() -> {
                try {
                    while (true) {
                        pongSem.acquire();
                        pingSem.release();
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            });

            pong.start();
            long startTime = System.nanoTime();

            for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                try {
                    pingSem.acquire();
                    pongSem.release();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                if (i % REPORT_INTERVAL == 0) {
                    report(i, startTime);
                }
            }
            pong.stop();
        }
    }

    // ----------- Strategy 4: Condition Variable / Lock -------------
    static class ConditionStrategy implements Strategy {
        final Lock lock = new ReentrantLock();
        final Condition cond = lock.newCondition();
        volatile boolean pingTurn = true;
        volatile boolean running = true;

        public void start() throws InterruptedException {
            Thread pong = new Thread(() -> {
                while (running) {
                    lock.lock();
                    try {
                        while (pingTurn) {
                            cond.await();
                        }
                        pingTurn = true;
                        cond.signal();
                    } catch (InterruptedException e) {
                        break; // graceful exit
                    } finally {
                        lock.unlock();
                    }
                }
            });

            pong.start();
            long startTime = System.nanoTime();

            for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                lock.lock();
                try {
                    while (!pingTurn) {
                        cond.await();
                    }
                    pingTurn = false;
                    cond.signal();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } finally {
                    lock.unlock();
                }

                if (i % REPORT_INTERVAL == 0) {
                    report(i, startTime);
                }
            }

            // Shut down pong thread cleanly
            running = false;
            pong.interrupt();
            pong.join();
        }
    }
    /*
        static class ConditionStrategy implements Strategy {
            final Lock lock = new ReentrantLock();
            final Condition cond = lock.newCondition();
            boolean pingTurn = true;

            public void start() {
                Thread pong = new Thread(() -> {
                    while (true) {
                        lock.lock();
                        try {
                            while (pingTurn)
                                cond.awaitUninterruptibly();
                            pingTurn = true;
                            cond.signal();
                        } finally {
                            lock.unlock();
                        }
                    }
                });

                pong.start();
                long startTime = System.nanoTime();

                for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                    lock.lock();
                    try {
                        while (!pingTurn)
                            cond.awaitUninterruptibly();
                        pingTurn = false;
                        cond.signal();
                    } finally {
                        lock.unlock();
                    }

                    if (i % REPORT_INTERVAL == 0) {
                        report(i, startTime);
                    }
                }
                pong.stop();
            }
        }
    */
    // ----------- Strategy 5: Exchanger -------------
    static class ExchangerStrategy implements Strategy {
        public void start() {
            Exchanger<String> exchanger = new Exchanger<>();

            Thread pong = new Thread(() -> {
                while (true) {
                    try {
                        exchanger.exchange("pong");
                    } catch (InterruptedException e) {
                        return;
                    }
                }
            });

            pong.start();
            long startTime = System.nanoTime();

            for (int i = 1; i <= TOTAL_ITERATIONS; i++) {
                try {
                    exchanger.exchange("ping");
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                if (i % REPORT_INTERVAL == 0) {
                    report(i, startTime);
                }
            }
            pong.stop();
        }
    }

    // ----------- Helper: Report function -------------
    static void report(int iterations, long startNano) {
        long now = System.nanoTime();
        double elapsedSec = (now - startNano) / 1_000_000_000.0;
        double avgLatencyUs = (now - startNano) / 1000.0 / iterations;
        double throughput = iterations / elapsedSec;
        System.out.printf("PING: Completed %d rounds, Avg latency: %.2f μs, Throughput: %.2f rounds/sec%n",
                iterations, avgLatencyUs, throughput);
    }
}
