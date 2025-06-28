package main

import (
	"time"
	"fmt"
)

func main() {
	c := make(chan bool)
	go func() {
		time.Sleep(1 * time.Second)
		// Receive from channel
		<- c
	}()
	start := time.Now()
	// Send true to channel
	c <- true
	fmt.Printf("send finished in %v\n", time.Since(start))
}
