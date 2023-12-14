package pkg

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	log "github.com/sirupsen/logrus"
	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

var (
	DefaultCBSHost string = "localhost"
	DefaultCBSPort int    = 15030
	DataDir        string = "./data/"
)

type CBSRequest struct {
	Constraints []*proto.Constraint `form:"constraints" json:"constraints" binding:"required"`
	Graph       *graph.Graph        `form:"graph" json:"graph" binding:"required"`
}

type FileContents struct {
	Name     string
	Endpoint int
	Number   int
}

type CBSEdge struct {
	Source      int     `yaml:"src" json:"src" binding:"required"`
	Destination int     `yaml:"dst" json:"dst" binding:"required"`
	Cost        float32 `yaml:"cost" json:"cost" binding:"required"`
}

type CBSNode struct {
	Node     int     `yaml:"node" json:"node" binding:"required"`
	CPU      float32 `yaml:"cpu" json:"cpu" binding:"required"`
	Leftover float32 `yaml:"leftover" json:"leftover" binding:"required"`
}

type CBSOutput struct {
	Nodes []*CBSNode `yaml:"nodes" json:"nodes" binding:"required"`
	Edges []*CBSEdge `yaml:"edges" json:"edges" binding:"required"`
}

type JsonCBSOut struct {
	Nodes []map[string]string `yaml:"nodes" json:"nodes" binding:"required"`
	Edges []map[string]string `yaml:"edges" json:"edges" binding:"required"`
}

/*
type CBSNode struct {
	Cpu      string `json:"cpu,omitempty"`
	Leftover string `json:"leftover,omitempty"`
	Node     string `json:"node,omitempty"`
}

type CBSEdge struct {
	Cost        string `json:"cost,omitempty"`
	Source      string `json:"src,omitempty"`
	Destination string `json:"dst,omitempty"`
}

type CBSResponse struct {
	Nodes []*CBSNode
	Edges []*CBSEdge
}
*/

func GraphToFile(g *graph.Graph) (string, []*FileContents, error) {
	fiName := fmt.Sprintf("%s/%s.sn", DataDir, "graph")

	fi, err := os.Create(fiName)
	if err != nil {
		return "", nil, err
	}
	defer fi.Close()

	// first line: node, edge
	line := fmt.Sprintf("%d %d\n", len(g.Vertices), len(g.Edges))
	log.Infof("line: %s\n", line)

	_, err = fi.Write([]byte(line))
	if err != nil {
		return "", nil, err
	}

	// track node name to where it gets written in file
	m := make([]*FileContents, 0)
	nodeMap := make(map[string]int)
	coord := 1

	for t, x := range g.Vertices {
		fc := &FileContents{Name: x.Name, Number: t}
		props := x.Properties
		y, ok := props["cpu"]
		if !ok {
			y = "0"
		}

		// TODO: because of how cbs is written, we need
		// to check prop field for a start and end and write
		// that into the graph, even though that may be
		// a constraint- so TODO is just wait for the next
		// code iteration
		line := ""
		_, ok = props["endpoint"]
		if ok {
			line = fmt.Sprintf("%d %d %s\n", coord, coord, y)
			fc.Endpoint = coord
			coord++
		} else {
			line = fmt.Sprintf("0 0 %s\n", y)
			fc.Endpoint = 0
		}
		log.Infof("line: %s\n", line)

		_, err = fi.Write([]byte(line))
		if err != nil {
			return "", nil, err
		}
		nodeMap[x.Name] = t
		m = append(m, fc)
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
		bw, ok := props["bw"]
		if !ok {
			bw = "0"
		}
		lat, ok := props["lat"]
		if !ok {
			lat = "0"
		}

		line := fmt.Sprintf("%d %d %s %s\n", x, y, bw, lat)
		log.Infof("line: %s\n", line)
		_, err = fi.Write([]byte(line))
		if err != nil {
			return "", nil, err
		}
	}

	return fiName, m, nil
}

func GraphFromFile() (*graph.Graph, error) {
	fiName := fmt.Sprintf("%s/%s.sn", DataDir, "graph")

	fiContents, err := os.ReadFile(fiName)
	if err != nil {
		return nil, err
	}
	contents := strings.Split(string(fiContents), "\n")

	log.Infof("graphToFile contents: %#v", contents)

	line1 := strings.Split(string(contents[0]), " ")
	log.Infof("line 0: %s\n", line1)
	nodeStr := line1[0]
	nodes, err := strconv.Atoi(nodeStr)
	if err != nil {
		return nil, err
	}

	/*
		vertStr := line1[1]
		verts, err := strconv.Atoi(vertStr)
		if err != nil {
			return nil, err
		}
	*/

	g := &graph.Graph{}

	for i := 1; i <= nodes; i++ {
		nodeLine := strings.Split(string(contents[i]), " ")
		log.Infof("vertex line %d: %s\n", i, nodeLine)

		if len(nodeLine) != 3 {
			return nil, fmt.Errorf("input graph file has bad node line: %s", nodeLine)
		}

		var endpoint bool = false
		if nodeLine[0] != "0" {
			endpoint = true
		}

		cpu := nodeLine[2]

		n := &graph.Vertex{
			Name: fmt.Sprintf("%d", i),
			Properties: map[string]string{
				"cpu": cpu,
			},
		}

		if endpoint {
			n.Properties["endpoint"] = "true"
		}

		err := g.AddVertexObj(n)
		if err != nil {
			return nil, err
		}

	}

	for i := nodes + 1; i < len(contents)-1; i++ {
		edgeLine := strings.Split(string(contents[i]), " ")
		log.Infof("edge line %d: %s\n", i, edgeLine)

		if len(edgeLine) != 4 {
			return nil, fmt.Errorf("input graph file has bad edge line: %s", edgeLine)
		}

		v0, err := strconv.Atoi(edgeLine[0])
		if err != nil {
			return nil, err
		}

		v1, err := strconv.Atoi(edgeLine[1])
		if err != nil {
			return nil, err
		}

		v := &graph.Vertex{Name: fmt.Sprintf("%d", v0+1)}

		f, n0 := g.FindVertex(v)
		if !f {
			return nil, fmt.Errorf("could not find vertex: %s in graph", edgeLine[0])
		}

		v = &graph.Vertex{Name: fmt.Sprintf("%d", v1+1)}

		f, n1 := g.FindVertex(v)
		if !f {
			return nil, fmt.Errorf("could not find vertex: %s in graph", edgeLine[1])
		}

		_, err = g.AddEdge(n0, n1, map[string]string{"bandwidth": edgeLine[2], "latency": edgeLine[3]})
		if err != nil {
			return nil, err
		}
	}

	log.Infof("after reading file: %v\n", g)

	return g, nil
}

// we need graph so we can understand the underlay to reference
func ConstraintsToFile(c []*proto.Constraint, graphFileMap []*FileContents) (string, error) {
	fiName := fmt.Sprintf("%s/%s.vnr", DataDir, "req0")

	// TODO because this is iVNE, operator always >

	// first line
	// nodes, edges, 0, 0, 10, 0, 0

	fileCount := 1
	fileMap := make(map[int]string)

	// TODO: the constraint must understand the underlying graph topology for naming nodes
	nodeMap := make(map[string]int)

	// node constraints
	// x, y, cpu constraint, the line becomes the identifier of the virtual node ID
	for i, constraint := range c {
		if constraint.Object == "cpu" {
			if len(constraint.Vertices) == 1 {
				name := constraint.Vertices[0]
				// nln should not be 0, if it is its a bad but legal constraint?
				line := fmt.Sprintf("0 0 %s\n", constraint.Lvalue)
				for _, x := range graphFileMap {
					if x.Name == name {
						if x.Endpoint > 0 {
							line = fmt.Sprintf("%d %d %s\n", x.Endpoint, x.Endpoint, constraint.Lvalue)
						}
						break
					}
				}
				//log.Infof("%s, %v", name, graphFileMap)
				fileMap[fileCount] = line
				nodeMap[constraint.Vertices[0]] = i
				fileCount++
			}
		}
	}

	// edge constraints
	// vNodeID, vNodeID, bandwidth, 0
	for _, constraint := range c {
		if constraint.Object == "bandwidth" {
			if len(constraint.Vertices) == 2 {

				x, ok := nodeMap[constraint.Vertices[0]]
				if !ok {
					return "", fmt.Errorf("edge vertex missing: %s", constraint.Vertices[0])
				}
				y, ok := nodeMap[constraint.Vertices[1]]
				if !ok {
					return "", fmt.Errorf("edge vertex missing: %s", constraint.Vertices[1])

				}

				line := fmt.Sprintf("%d %d %s %d\n",
					x,
					y,
					constraint.Lvalue,
					0,
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

	fi, err := os.Create(fiName)
	if err != nil {
		return "", err
	}
	defer fi.Close()

	for i := 0; i < len(fileMap); i++ {
		line := fileMap[i]
		log.Infof("line %d: %s\n", i, line)
		_, err = fi.Write([]byte(line))
		if err != nil {
			return "", err
		}
	}

	return fiName, nil
}
