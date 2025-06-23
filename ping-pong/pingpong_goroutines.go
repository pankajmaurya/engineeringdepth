package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
)

const (
	LogInterval = 1000000
)

type PingPongState struct {
	pingReady     int32 // atomic
	pongReady     int32 // atomic
	pingAlive     int32 // atomic
	pongAlive     int32 // atomic
	pingTimestamp int64 // atomic
	pongTimestamp int64 // atomic
	roundCount    int64 // atomic
	resetFlag     int32 // atomic
}

type GoroutineBenchmark struct {
	state   *PingPongState
	running int32 // atomic
	wg      sync.WaitGroup
}

func NewGoroutineBenchmark() *GoroutineBenchmark {
	return &GoroutineBenchmark{
		state:   &PingPongState{},
		running: 1,
	}
}

func (g *GoroutineBenchmark) getTimestampUs() int64 {
	return time.Now().UnixNano() / 1000
}

func (g *GoroutineBenchmark) isRunning() bool {
	return atomic.LoadInt32(&g.running) == 1
}

func (g *GoroutineBenchmark) stop() {
	atomic.StoreInt32(&g.running, 0)
}

func (g *GoroutineBenchmark) pingRoutine() {
	defer g.wg.Done()

	fmt.Println("Starting PING goroutine...")

	var totalLatency int64
	localRoundCount := 0
	startTime := g.getTimestampUs()

	atomic.StoreInt32(&g.state.pingAlive, 1)

	fmt.Println("Waiting for PONG goroutine to be ready...")
	for atomic.LoadInt32(&g.state.pongAlive) == 0 && g.isRunning() {
		time.Sleep(100 * time.Nanosecond)
	}

	if !g.isRunning() {
		return
	}

	fmt.Println("PONG goroutine ready! Starting ping-pong...")

	for g.isRunning() {
		atomic.StoreInt32(&g.state.pingAlive, 1)

		if atomic.LoadInt32(&g.state.pongAlive) == 0 {
			fmt.Println("PONG goroutine died! Exiting...")
			break
		}

		pingTime := g.getTimestampUs()
		atomic.StoreInt64(&g.state.pingTimestamp, pingTime)
		atomic.StoreInt32(&g.state.pongReady, 0)
		atomic.StoreInt32(&g.state.pingReady, 1)

		for atomic.LoadInt32(&g.state.pingReady) == 1 && g.isRunning() {
			if atomic.LoadInt32(&g.state.pongAlive) == 0 {
				fmt.Println("PONG goroutine died during wait! Exiting...")
				return
			}
		}

		if !g.isRunning() {
			break
		}

		if atomic.LoadInt32(&g.state.pongReady) == 1 {
			pongTime := atomic.LoadInt64(&g.state.pongTimestamp)
			latency := pongTime - pingTime
			totalLatency += latency
			localRoundCount++
			atomic.AddInt64(&g.state.roundCount, 1)

			atomic.StoreInt32(&g.state.pongReady, 0)

			if localRoundCount%LogInterval == 0 {
				avgLatency := float64(totalLatency) / LogInterval
				elapsed := g.getTimestampUs() - startTime
				throughput := float64(LogInterval) * 1_000_000.0 / float64(elapsed)

				fmt.Printf("PING: Completed %d rounds, Avg latency: %.2f μs, Throughput: %.2f rounds/sec\n",
					localRoundCount, avgLatency, throughput)

				totalLatency = 0
				startTime = g.getTimestampUs()
			}
		}
	}

	fmt.Printf("PING goroutine shutting down after %d rounds\n", localRoundCount)
	atomic.StoreInt32(&g.state.pingAlive, 0)
}

func (g *GoroutineBenchmark) pongRoutine() {
	defer g.wg.Done()

	fmt.Println("Starting PONG goroutine...")

	localRoundCount := 0

	atomic.StoreInt32(&g.state.pongAlive, 1)

	fmt.Println("Waiting for PING goroutine to be ready...")
	for atomic.LoadInt32(&g.state.pingAlive) == 0 && g.isRunning() {
		time.Sleep(100 * time.Nanosecond)
	}

	if !g.isRunning() {
		return
	}

	fmt.Println("PING goroutine ready! Ready for ping-pong...")

	for g.isRunning() {
		atomic.StoreInt32(&g.state.pongAlive, 1)

		if atomic.LoadInt32(&g.state.pingAlive) == 0 {
			fmt.Println("PING goroutine died! Exiting...")
			break
		}

		for atomic.LoadInt32(&g.state.pingReady) == 0 && g.isRunning() {
			if atomic.LoadInt32(&g.state.pingAlive) == 0 {
				fmt.Println("PING goroutine died during wait! Exiting...")
				return
			}
		}

		if !g.isRunning() {
			break
		}

		if atomic.LoadInt32(&g.state.pingReady) == 1 {
			roundCount := atomic.LoadInt64(&g.state.roundCount)
			if roundCount >= 1_000_000_000 {
				fmt.Println("PONG: FATAL ERROR - Simulated crash at 1 billion pings!")
				os.Exit(1)
			}

			pongTime := g.getTimestampUs()
			atomic.StoreInt64(&g.state.pongTimestamp, pongTime)
			atomic.StoreInt32(&g.state.pingReady, 0)
			atomic.StoreInt32(&g.state.pongReady, 1)
			localRoundCount++

			if localRoundCount%LogInterval == 0 {
				totalRounds := atomic.LoadInt64(&g.state.roundCount)
				fmt.Printf("PONG: Responded to %d pings (Total rounds: %d)\n",
					localRoundCount, totalRounds)
			}
		}
	}

	fmt.Printf("PONG goroutine shutting down after %d responses\n", localRoundCount)
	atomic.StoreInt32(&g.state.pongAlive, 0)
}

func (g *GoroutineBenchmark) Run() {
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigChan
		fmt.Println("\nReceived signal, cleaning up...")
		g.stop()
	}()

	fmt.Println("Starting ping-pong benchmark with goroutines...")

	g.wg.Add(2)
	go g.pingRoutine()
	go g.pongRoutine()

	g.wg.Wait()
	fmt.Println("Benchmark completed")
}

// -------- Channel Benchmark --------

type ChannelBenchmark struct {
	pingChan chan int64
	pongChan chan int64
	running  int32 // atomic
	wg       sync.WaitGroup
}

func NewChannelBenchmark() *ChannelBenchmark {
	return &ChannelBenchmark{
		pingChan: make(chan int64, 1),
		pongChan: make(chan int64, 1),
		running:  1,
	}
}

func (c *ChannelBenchmark) getTimestampUs() int64 {
	return time.Now().UnixNano() / 1000
}

func (c *ChannelBenchmark) isRunning() bool {
	return atomic.LoadInt32(&c.running) == 1
}

func (c *ChannelBenchmark) stop() {
	atomic.StoreInt32(&c.running, 0)
}

func (c *ChannelBenchmark) pingRoutine() {
	defer c.wg.Done()

	fmt.Println("Starting PING goroutine (channel-based)...")

	var totalLatency int64
	localRoundCount := 0
	startTime := c.getTimestampUs()
	var pingTime int64

	for c.isRunning() {
		select {
		case c.pingChan <- func() int64 {
			pingTime = c.getTimestampUs()
			return pingTime
		}():
			select {
			case pongTime := <-c.pongChan:
				latency := pongTime - pingTime
				totalLatency += latency
				localRoundCount++

				if localRoundCount%LogInterval == 0 {
					avgLatency := float64(totalLatency) / LogInterval
					elapsed := c.getTimestampUs() - startTime
					throughput := float64(LogInterval) * 1_000_000.0 / float64(elapsed)

					fmt.Printf("PING (chan): Completed %d rounds, Avg latency: %.2f μs, Throughput: %.2f rounds/sec\n",
						localRoundCount, avgLatency, throughput)

					totalLatency = 0
					startTime = c.getTimestampUs()
				}
			case <-time.After(time.Second):
				fmt.Println("PONG timeout - pong goroutine may be dead")
				continue
			}
		case <-time.After(time.Second):
			fmt.Println("PING timeout - pong goroutine may be dead")
			continue
		}

		if !c.isRunning() {
			break
		}
	}

	fmt.Printf("PING goroutine (channel-based) shutting down after %d rounds\n", localRoundCount)
}

func (c *ChannelBenchmark) pongRoutine() {
	defer c.wg.Done()

	fmt.Println("Starting PONG goroutine (channel-based)...")

	localRoundCount := 0

	for c.isRunning() {
		select {
		case <-c.pingChan:
			if localRoundCount >= 1_000_000_000 {
				fmt.Println("PONG: FATAL ERROR - Simulated crash at 1 billion pings!")
				os.Exit(1)
			}

			pongTime := c.getTimestampUs()
			localRoundCount++

			select {
			case c.pongChan <- pongTime:
			case <-time.After(time.Second):
				fmt.Println("Failed to send pong - ping goroutine may be dead")
				continue
			}

			if localRoundCount%LogInterval == 0 {
				fmt.Printf("PONG (chan): Responded to %d pings\n", localRoundCount)
			}
		case <-time.After(time.Second):
			// no-op
		}
	}

	fmt.Printf("PONG goroutine (channel-based) shutting down after %d responses\n", localRoundCount)
}

func (c *ChannelBenchmark) Run() {
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigChan
		fmt.Println("\nReceived signal, cleaning up...")
		c.stop()
	}()

	fmt.Println("Starting ping-pong benchmark with channels...")

	c.wg.Add(2)
	go c.pingRoutine()
	go c.pongRoutine()

	c.wg.Wait()
	fmt.Println("Channel benchmark completed")
}

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s <atomic|channel>\n", os.Args[0])
		os.Exit(1)
	}

	switch os.Args[1] {
	case "atomic":
		NewGoroutineBenchmark().Run()
	case "channel":
		NewChannelBenchmark().Run()
	default:
		fmt.Println("Invalid mode. Use 'atomic' or 'channel'")
		os.Exit(1)
	}
}
