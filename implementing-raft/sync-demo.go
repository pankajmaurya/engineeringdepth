package main

import "sync"

func main() {
	var a string
	a = "Foo"
	var wg sync.WaitGroup
	wg.Add(1)
	// A closure in Go
	go func() {
		a = "Hello world"
		wg.Done()
	}()
	println(a)
	wg.Wait()
	println(a)
}
