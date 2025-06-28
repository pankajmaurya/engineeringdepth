package main

import "fmt"
import "math/rand"
import "time"
import "sync"

func requestVote() bool {
	return rand.Float64() < 0.75
}  

func main() {
	rand.Seed(time.Now().UnixNano())

	// Number of yes votes
	count := 0
	// Number of responses received 
	finished := 0

	var mu sync.Mutex

	for i := 0; i < 10; i++ {
		go func() {
			vote := requestVote()
			mu.Lock()
			defer mu.Unlock()
			if vote {
				count++
			}
			finished++
		}()
	}

	for {
		mu.Lock()
		if count >= 5 || finished == 10 {
			break
		}
		mu.Unlock()
		// Busy waiting here, sleeping by how much??
		// time.Sleep(50 * time.Millisecond)
	}

	if count >= 5 {
		fmt.Printf("received 5+ votes, waited for %d resp!\n", finished)
	} else {
		println("lost")
	}
	mu.Unlock()
}
