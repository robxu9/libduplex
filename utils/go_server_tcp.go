package main

import (
	"log"
	"os"
	"os/signal"

	"github.com/progrium/duplex/poc2/duplex"
)

func main() {
	server := duplex.NewPeer()
	defer server.Shutdown()

	log.Printf("binding to tcp://127.0.0.1:9999")

	if err := server.Bind("tcp://127.0.0.1:9999"); err != nil {
		panic(err)
	}

	ch := make(chan os.Signal, 0)
	signal.Notify(ch, os.Interrupt)

	log.Printf("ctrl-c to quit")

	<-ch
}
