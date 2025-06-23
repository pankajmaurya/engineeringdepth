package main

import (
	"encoding/binary"
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"time"
	"unsafe"

	"golang.org/x/exp/mmap"
)

const (
	ShmFile     = "/tmp/pingpong_shm"
	ShmSize     = 1024
	LogInterval = 1000
)

// SharedState represents the shared memory structure
// Must match the C struct layout for compatibility
type SharedState struct {
	PingReady     int32 // 4 bytes
	PongReady     int32 // 4 bytes
	PingAlive     int32 // 4 bytes
	PongAlive     int32 // 4 bytes
	PingTimestamp int64 // 8 bytes
	PongTimestamp int64 // 8 bytes
	RoundCount    int32 // 4 bytes
	ResetFlag     int32 // 4 bytes
}

type PingPongBenchmark struct {
	mmapReader *mmap.ReaderAt
	sharedMem  []byte
	running    bool
	isPing     bool
}

func NewPingPongBenchmark(isPing bool) (*PingPongBenchmark, error) {
	// Create shared memory file if it doesn't exist
	if _, err := os.Stat(ShmFile); os.IsNotExist(err) {
		file, err := os.Create(ShmFile)
		if err != nil {
			return nil, fmt.Errorf("failed to create shared memory file: %v", err)
		}
		
		// Initialize file with zeros
		if err := file.Truncate(ShmSize); err != nil {
			file.Close()
			return nil, fmt.Errorf("failed to truncate file: %v", err)
		}
		file.Close()
	}

	// Open the memory-mapped file
	reader, err := mmap.Open(ShmFile)
	if err != nil {
		return nil, fmt.Errorf("failed to open mmap file: %v", err)
	}

	// Create a byte slice to work with the memory
	sharedMem := make([]byte, int(unsafe.Sizeof(SharedState{})))
	
	return &PingPongBenchmark{
		mmapReader: reader,
		sharedMem:  sharedMem,
		running:    true,
		isPing:     isPing,
	}, nil
}

func (p *PingPongBenchmark) Close() {
	if p.mmapReader != nil {
		p.mmapReader.Close()
	}
}

func (p *PingPongBenchmark) getTimestampUs() int64 {
	return time.Now().UnixNano() / 1000
}

func (p *PingPongBenchmark) readSharedState() *SharedState {
	// Read from mmap into our buffer
	n, err := p.mmapReader.ReadAt(p.sharedMem, 0)
	if err != nil || n < len(p.sharedMem) {
		return nil
	}
	
	// Convert bytes to struct
	state := &SharedState{}
	offset := 0
	
	state.PingReady = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	offset += 4
	state.PongReady = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	offset += 4
	state.PingAlive = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	offset += 4
	state.PongAlive = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	offset += 4
	state.PingTimestamp = int64(binary.LittleEndian.Uint64(p.sharedMem[offset:]))
	offset += 8
	state.PongTimestamp = int64(binary.LittleEndian.Uint64(p.sharedMem[offset:]))
	offset += 8
	state.RoundCount = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	offset += 4
	state.ResetFlag = int32(binary.LittleEndian.Uint32(p.sharedMem[offset:]))
	
	return state
}

func (p *PingPongBenchmark) writeSharedState(state *SharedState) error {
	// Convert struct to bytes
	offset := 0
	
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.PingReady))
	offset += 4
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.PongReady))
	offset += 4
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.PingAlive))
	offset += 4
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.PongAlive))
	offset += 4
	binary.LittleEndian.PutUint64(p.sharedMem[offset:], uint64(state.PingTimestamp))
	offset += 8
	binary.LittleEndian.PutUint64(p.sharedMem[offset:], uint64(state.PongTimestamp))
	offset += 8
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.RoundCount))
	offset += 4
	binary.LittleEndian.PutUint32(p.sharedMem[offset:], uint32(state.ResetFlag))
	
	// Write to file (this is a simplified approach - in production you'd want proper file writing)
	file, err := os.OpenFile(ShmFile, os.O_WRONLY, 0666)
	if err != nil {
		return err
	}
	defer file.Close()
	
	_, err = file.WriteAt(p.sharedMem, 0)
	return err
}

func (p *PingPongBenchmark) updateField(offset int, value interface{}) error {
	// Read current state
	state := p.readSharedState()
	if state == nil {
		return fmt.Errorf("failed to read shared state")
	}
	
	// Update specific field
	switch offset {
	case 0: // PingReady
		state.PingReady = value.(int32)
	case 1: // PongReady
		state.PongReady = value.(int32)
	case 2: // PingAlive
		state.PingAlive = value.(int32)
	case 3: // PongAlive
		state.PongAlive = value.(int32)
	case 4: // PingTimestamp
		state.PingTimestamp = value.(int64)
	case 5: // PongTimestamp
		state.PongTimestamp = value.(int64)
	case 6: // RoundCount
		state.RoundCount = value.(int32)
	case 7: // ResetFlag
		state.ResetFlag = value.(int32)
	}
	
	return p.writeSharedState(state)
}

func (p *PingPongBenchmark) pingServer() {
	fmt.Println("Starting PING server...")
	
	var totalLatency int64
	localRoundCount := 0
	startTime := p.getTimestampUs()
	
	// Initialize as ping server
	state := &SharedState{
		PingAlive:  1,
		PingReady:  0,
		PongReady:  0,
		RoundCount: 0,
	}
	p.writeSharedState(state)
	
	for p.running {
		currentState := p.readSharedState()
		if currentState == nil {
			continue
		}
		
		// Update heartbeat
		p.updateField(2, int32(1)) // PingAlive
		
		// Wait for pong server to be alive
		if currentState.PongAlive == 0 {
			fmt.Println("Waiting for PONG server to join...")
			for {
				currentState = p.readSharedState()
				if currentState == nil || !p.running {
					break
				}
				if currentState.PongAlive != 0 {
					break
				}
				p.updateField(2, int32(1)) // Keep updating heartbeat
				time.Sleep(time.Millisecond)
			}
			if !p.running {
				break
			}
			fmt.Println("PONG server joined! Starting ping-pong...")
		}
		
		// Send PING
		pingTime := p.getTimestampUs()
		p.updateField(4, pingTime)    // PingTimestamp
		p.updateField(0, int32(1))    // PingReady
		p.updateField(1, int32(0))    // PongReady
		
		// Wait for PONG response
		for {
			currentState = p.readSharedState()
			if currentState == nil || !p.running {
				break
			}
			if currentState.PingReady == 0 {
				break
			}
			if currentState.PongAlive == 0 {
				fmt.Println("PONG server died! Waiting for it to restart...")
				break
			}
			p.updateField(2, int32(1)) // Update heartbeat
		}
		
		// Check if pong server died
		currentState = p.readSharedState()
		if currentState != nil && currentState.PongAlive == 0 {
			continue
		}
		
		if !p.running {
			break
		}
		
		// PONG received, calculate latency
		if currentState != nil && currentState.PongReady == 1 {
			latency := currentState.PongTimestamp - pingTime
			totalLatency += latency
			localRoundCount++
			p.updateField(6, currentState.RoundCount+1) // RoundCount
			
			// Reset pong_ready for next round
			p.updateField(1, int32(0)) // PongReady
			
			// Log every 1000 rounds
			if localRoundCount%LogInterval == 0 {
				avgLatency := float64(totalLatency) / LogInterval
				elapsed := p.getTimestampUs() - startTime
				throughput := float64(LogInterval) * 1000000.0 / float64(elapsed)
				
				fmt.Printf("PING: Completed %d rounds, Avg latency: %.2f μs, Throughput: %.2f rounds/sec\n",
					localRoundCount, avgLatency, throughput)
				
				totalLatency = 0
				startTime = p.getTimestampUs()
			}
		}
	}
	
	fmt.Printf("PING server shutting down after %d rounds\n", localRoundCount)
}

func (p *PingPongBenchmark) pongServer() {
	fmt.Println("Starting PONG server...")
	
	localRoundCount := 0
	
	// Mark pong server as alive
	p.updateField(3, int32(1)) // PongAlive
	
	for p.running {
		currentState := p.readSharedState()
		if currentState == nil {
			continue
		}
		
		// Update heartbeat
		p.updateField(3, int32(1)) // PongAlive
		
		// Wait for ping server to be alive
		if currentState.PingAlive == 0 {
			fmt.Println("Waiting for PING server to join...")
			for {
				currentState = p.readSharedState()
				if currentState == nil || !p.running {
					break
				}
				if currentState.PingAlive != 0 {
					break
				}
				p.updateField(3, int32(1)) // Keep updating heartbeat
				time.Sleep(time.Millisecond)
			}
			if !p.running {
				break
			}
			fmt.Println("PING server joined! Ready for ping-pong...")
		}
		
		// Wait for PING
		for {
			currentState = p.readSharedState()
			if currentState == nil || !p.running {
				break
			}
			if currentState.PingReady != 0 {
				break
			}
			if currentState.PingAlive == 0 {
				fmt.Println("PING server died! Waiting for it to restart...")
				break
			}
			p.updateField(3, int32(1)) // Update heartbeat
		}
		
		// Check if ping server died
		currentState = p.readSharedState()
		if currentState != nil && currentState.PingAlive == 0 {
			continue
		}
		
		if !p.running {
			break
		}
		
		// PING received, send PONG
		if currentState != nil && currentState.PingReady == 1 {
			// SIMULATE BUG: Crash when ping count reaches 1 billion
			if currentState.RoundCount >= 1000000000 {
				fmt.Println("PONG: FATAL ERROR - Simulated crash at 1 billion pings!")
				fmt.Println("PONG: Process crashing due to simulated bug...")
				os.Exit(1)
			}
			
			pongTime := p.getTimestampUs()
			p.updateField(5, pongTime)    // PongTimestamp
			p.updateField(0, int32(0))    // PingReady (clear)
			p.updateField(1, int32(1))    // PongReady (set)
			localRoundCount++
			
			// Log every 1000 rounds
			if localRoundCount%LogInterval == 0 {
				fmt.Printf("PONG: Responded to %d pings (Total rounds: %d)\n",
					localRoundCount, currentState.RoundCount)
			}
		}
	}
	
	fmt.Printf("PONG server shutting down after %d responses\n", localRoundCount)
}

func (p *PingPongBenchmark) Run() {
	// Set up signal handlers
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	
	go func() {
		<-sigChan
		fmt.Println("\nReceived signal, cleaning up...")
		p.running = false
	}()
	
	fmt.Printf("Connected to shared memory file, running in %s mode\n", map[bool]string{true: "ping", false: "pong"}[p.isPing])
	
	// Run appropriate server
	if p.isPing {
		p.pingServer()
	} else {
		p.pongServer()
	}
	
	// Cleanup
	if p.isPing {
		p.updateField(2, int32(0)) // PingAlive
	} else {
		p.updateField(3, int32(0)) // PongAlive
	}
}

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s <ping|pong>\n", os.Args[0])
		fmt.Println("  ping - Run as PING server (initiates ping-pong)")
		fmt.Println("  pong - Run as PONG server (responds to pings)")
		os.Exit(1)
	}
	
	mode := os.Args[1]
	isPing := mode == "ping"
	isPong := mode == "pong"
	
	if !isPing && !isPong {
		fmt.Println("Invalid mode. Use 'ping' or 'pong'")
		os.Exit(1)
	}
	
	benchmark, err := NewPingPongBenchmark(isPing)
	if err != nil {
		fmt.Printf("Failed to create benchmark: %v\n", err)
		os.Exit(1)
	}
	defer benchmark.Close()
	
	// Clean up shared memory file on exit
	defer func() {
		if err := os.Remove(ShmFile); err != nil {
			fmt.Printf("Note: Could not remove shared memory file: %v\n", err)
		} else {
			fmt.Println("Shared memory file cleaned up")
		}
	}()
	
	benchmark.Run()
}
