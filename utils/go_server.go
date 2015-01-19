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

	log.Printf("binding to /tmp/duplex_server")

	if err := server.Bind("unix:///tmp/duplex_server"); err != nil {
		panic(err)
	}

	ch := make(chan os.Signal, 0)
	signal.Notify(ch, os.Interrupt)

	log.Printf("ctrl-c to quit")

	<-ch
}
