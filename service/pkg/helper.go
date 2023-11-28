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

/*
	type Vertex struct {
		Name       string
		Value      string
		Properties map[string]string
		Weight     int
	}

	type Edge struct {
		Name       string
		Vertices   []*Vertex
		Properties map[string]string
		Weight     int
	}

	type Graph struct {
		Name     string
		Vertices []*Vertex
		Edges    []*Edge
	}

4 6
0 0 5
0 0 20
0 0 300
0 0 100
0 1 100 30
0 2 100 70
0 3 100 80
1 2 100 75
1 3 100 60
2 3 100 40
*/
func GraphToFile(g *graph.Graph) (string, error) {
	fiName := fmt.Sprintf("%s/%s.sn", DataDir, "graph")

	fi, err := os.OpenFile(fiName, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return "", err
	}
	defer fi.Close()

	// first line: node, edge
	_, err = fi.Write([]byte(fmt.Sprintf("%d %d\n", len(g.Vertices), len(g.Edges))))
	if err != nil {
		return "", err
	}

	nodeMap := make(map[string]int)
	// nodes and cpu
	for t, x := range g.Vertices {
		props := x.Properties
		y, ok := props["cpu"]
		if !ok {
			y = "0"
		}

		_, err = fi.Write([]byte(fmt.Sprintf("0 0 %s\n", y)))
		if err != nil {
			return "", err
		}
		nodeMap[x.Name] = t
	}

	// edges and bandwidth, latency
	for _, e := range g.Edges {
		if len(e.Vertices) != 2 {
			log.Errorf("bad edge, ignoring: %v\n", e)
			continue
		}
		verts := e.Vertices

		x := nodeMap[verts[0].Name]
		y := nodeMap[verts[1].Name]

		props := e.Properties
		bw, ok := props["bandwidth"]
		if !ok {
			bw = "0"
		}
		lat, ok := props["latency"]
		if !ok {
			lat = "0"
		}

		_, err = fi.Write([]byte(fmt.Sprintf("%d %d %s %s\n", x, y, bw, lat)))
		if err != nil {
			return "", err
		}
	}

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
