package pkg

import (
	"fmt"
	"os"

	log "github.com/sirupsen/logrus"
	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

var (
	CBSPath        string = "/usr/bin/cbs"
	DefaultCBSHost string = "localhost"
	DefaultCBSPort int    = 15030
	DataDir        string = "./data/"
)

func GraphToFile(g graph.Graph) (string, error) {
	return "", nil
}

func ConstraintsToFile(c []*proto.Constraint) (string, error) {
	fiName := fmt.Sprintf("%s/%s.vnr", DataDir, "req0")

	// TODO because this is iVNE, operator always >

	// first line
	// nodes, edges, 0, 0, 10, 0, 0

	fileCount := 1
	fileMap := make(map[int]string)

	// node constraints
	// x, y, cpu constraint, the line becomes the identifier of the virtual node ID
	nodeMap := make(map[string]int)
	for i, constraint := range c {
		if constraint.Object == "cpu" {
			line := fmt.Sprintf("%d %d %s\n", i, i, constraint.Lvalue)
			if len(constraint.Vertices) == 1 {
				nodeMap[constraint.Vertices[0]] = i
				fileMap[fileCount] = line
				fileCount++
			}
		}
	}

	// edge constraints
	// vNodeID, vNodeID, bandwidth, 0
	for _, constraint := range c {
		if constraint.Object == "bandwidth" {
			if len(constraint.Vertices) == 2 {
				line := fmt.Sprintf("%d %d %s %d\n",
					nodeMap[constraint.Vertices[0]],
					nodeMap[constraint.Vertices[1]],
					constraint.Lvalue, 0,
				)
				fileMap[fileCount] = line
				fileCount++
			}
		}
	}

	nodes := len(nodeMap)
	edges := fileCount - (1 + len(nodeMap))
	fileMap[0] = fmt.Sprintf("%d %d 0 0 10 0 0\n", nodes, edges)

	log.Infof("to write %s contents:\n%v\n", fiName, fileMap)

	fi, err := os.OpenFile(fiName, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return "", err
	}
	defer fi.Close()

	for i := 0; i <= len(fileMap); i++ {
		line := fileMap[i]
		_, err = fi.Write([]byte(line))
		if err != nil {
			return "", err
		}
	}

	return fiName, nil
}
