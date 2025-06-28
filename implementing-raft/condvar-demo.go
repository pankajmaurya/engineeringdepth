package main

import "fmt"
import "math/rand"
import "time"
import "sync"

func requestVote(id int) bool {
	time.Sleep(10 * time.Millisecond)
	resp := rand.Float64() < 0.75
	fmt.Printf("Request for vote %d -> voting %b\n", id, resp)
	return resp
}  

func main() {
	rand.Seed(time.Now().UnixNano())

	// Number of yes votes
	count := 0
	// Number of responses received 
	finished := 0

	var mu sync.Mutex
	cond := sync.NewCond(&mu)

	for i := 0; i < 10; i++ {
		go func(i int) {
			vote := requestVote(i)
			mu.Lock()
			defer mu.Unlock()
			if vote {
				count++
			}
			finished++
			cond.Broadcast() 
		}(i)
	}

	mu.Lock()
	for count < 5 && finished < 10 {
		cond.Wait()
	}

	if count >= 5 {
		fmt.Printf("received 5+ votes, waited for %d resp!\n", finished)
	} else {
		println("lost")
	}
	mu.Unlock()
}
