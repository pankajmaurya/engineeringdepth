package main

import (
	"fmt"
	"sync"
	"time"
)

func main() {
	c := make(chan bool, 1)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		time.Sleep(1 * time.Second)
		// Receive from channel
		<- c
		fmt.Printf("Received from channel in goroutine\n")
		wg.Done()
	}()
	start := time.Now()
	// Send true to channel
	c <- true
	fmt.Printf("send finished in %v\n", time.Since(start))
	wg.Wait()
}
