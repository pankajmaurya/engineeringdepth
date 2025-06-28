package main

import "fmt"
import "sync"
import "time"

var done bool
var mu sync.Mutex

func main() {
	time.Sleep(1 * time.Second)
	println("started")
	go periodic(1)
	go periodic(2)
	time.Sleep(5 * time.Second)

	mu.Lock()
	done = true
	mu.Unlock()
	println("cancelled")
	time.Sleep(3 * time.Second)
}

func periodic(id int) {
	for {
		fmt.Printf("%d tick\n", id)
		time.Sleep(1 * time.Second)
		mu.Lock()
		if done {
			fmt.Printf("%d About to return\n", id)
			// Without this the above line shows up only for 1 periodic
			mu.Unlock()
			return
		}
		mu.Unlock()
	}
}
