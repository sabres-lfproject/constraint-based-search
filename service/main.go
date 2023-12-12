package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"strconv"

	"github.com/gin-gonic/gin"
	log "github.com/sirupsen/logrus"

	"pulwar.isi.edu/sabres/cbs/cbs/service/pkg"
	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

var (
	cbsPath string
)

func runCBS(command string, args []string) (string, error) {

	// open the out file for writing
	fiName := fmt.Sprintf("%s/cbs_output.txt", pkg.DataDir)
	outfile, err := os.Create(fiName)
	if err != nil {
		panic(err)
	}
	defer outfile.Close()

	bufWrite := bufio.NewWriter(outfile)

	cmd := &exec.Cmd{
		Path:   command,
		Args:   args,
		Stdout: bufWrite,
		Stderr: bufWrite,
	}

	log.Infof("Executing command: %s\n", cmd)

	err = cmd.Run()

	log.Infof("Command executed with error: %v\n", err)

	if err != nil {
		runFi, err2 := os.ReadFile(fiName)
		if err2 != nil {
			return fiName, err
		}
		log.Infof("Output of command: %s", string(runFi))
	}

	return fiName, err
}

func manageCBS(c []*proto.Constraint, g *graph.Graph) (string, []*pkg.FileContents, error) {

	for _, xxx := range c {
		log.Infof("manageCBS: constraints [%v] %s %s\n", xxx.Vertices, xxx.Object, xxx.Lvalue)
	}

	for _, zzz := range g.Vertices {
		if zzz.Properties == nil {
			log.Warnf("bad map passed in for vertex: %v\n", zzz)
			continue
		}
		log.Infof("manageCBS: vertex %s %s (*%s*)\n", zzz.Name, zzz.Properties["cpu"], zzz.Properties["endpoint"])
	}

	for _, yyy := range g.Edges {
		if len(yyy.Vertices) < 1 {
			log.Warnf("bad edge passed in: %v\n", yyy)
			continue
		}
		if yyy.Properties == nil {
			log.Warnf("bad map passed in for edge: %v\n", yyy)
			continue
		}
		log.Infof("manageCBS: edge %s->%s %s %s\n", yyy.Vertices[0].Name, yyy.Vertices[1].Name, yyy.Properties["bw"], yyy.Properties["selector"])
	}

	// convert substrait to file
	snFiName, graphMap, err := pkg.GraphToFile(g)
	if err != nil {
		//return nil, make(map[string]int), err
		return "", nil, err
	}

	log.Infof("Post GraphToFile map: %#v\n", graphMap)

	// convert constraints to file
	// vnrFile
	vnrFiName, err := pkg.ConstraintsToFile(c, graphMap)
	if err != nil {
		return "", nil, err
	}

	log.Infof("vnrfile name: %s\n", vnrFiName)
	log.Infof("snfile name: %s\n", snFiName)

	// args[0] is executable
	fiNameOut, err := runCBS(cbsPath, []string{cbsPath, vnrFiName, snFiName, pkg.DataDir})
	if err != nil {
		return fiNameOut, nil, err
	}

	// open outDir json and return as a string
	//mapping, err := ioutil.ReadFile(fmt.Sprintf("%s/%s", pkg.DataDir, "out.json"))

	return fiNameOut, graphMap, err
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

func manageCBSHandler(c *gin.Context) {

	log.Infof("in manageCBSHandler\n")

	var jsonInput pkg.CBSRequest
	if err := c.ShouldBindJSON(&jsonInput); err != nil {
		log.Infof("bad CBS Request\n")
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	constraints := jsonInput.Constraints
	graph := jsonInput.Graph

	log.Infof("json input: %v\n", jsonInput)

	outFiName, mapToStr, err := manageCBS(constraints, graph)

	if err != nil {
		contents, err2 := ioutil.ReadFile(outFiName)
		if err2 != nil {
			errStr := fmt.Sprintf("file: %s, error: %v", outFiName, err2.Error())
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "error2": errStr})
			return
		}

		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "output": contents})
		return
	} else {

		log.Infof("map in<->out: %#v\n", mapToStr)

		// need to open up the json now
		var content *CBSOutput
		jsonContents, err := ioutil.ReadFile(fmt.Sprintf("%s/%s", pkg.DataDir, "out.json"))
		err = json.Unmarshal(jsonContents, &content)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "output": jsonContents})
			return
		}

		jsonOut := &JsonCBSOut{
			Edges: make([]map[string]string, 0),
			Nodes: make([]map[string]string, 0),
		}
		// correlate back the vertex/edge names back to string format
		// assumes my code doesnt put in duplicates
		for _, e := range content.Edges {
			src := ""
			dst := ""
			cost := fmt.Sprintf("%f", e.Cost)
			for _, obj := range mapToStr {
				log.Infof("(%d->%d) [%s][%d][%d]\n", e.Source, e.Destination, obj.Name, obj.Number, obj.Endpoint)
				if e.Source == obj.Number {
					log.Infof("src set to: %s", obj.Name)
					src = obj.Name
				}
				if e.Destination == obj.Number {
					dst = obj.Name
					log.Infof("dst set to: %s", obj.Name)
				}
			}
			jsonOut.Edges = append(jsonOut.Edges, map[string]string{
				"src":  src,
				"dst":  dst,
				"cost": cost,
			})
		}
		for _, n := range content.Nodes {
			node := ""
			cpu := fmt.Sprintf("%f", n.CPU)
			leftover := fmt.Sprintf("%f", n.Leftover)
			for _, obj := range mapToStr {
				log.Infof("(%d) [%s] [%d]\n", n.Node, obj.Name, obj.Number)
				if n.Node == obj.Number {
					node = obj.Name
					log.Infof("node set to: %s", obj.Name)
					break
				}
			}

			jsonOut.Nodes = append(jsonOut.Nodes, map[string]string{
				"node":     node,
				"cpu":      cpu,
				"leftover": leftover,
			})
		}

		// Send back the data now with strings
		data, err := json.Marshal(jsonOut)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "output": jsonOut})
			return
		}

		c.Data(http.StatusOK, "application/json", data)
	}

}

func main() {

	var debug bool
	var port int
	var host string

	flag.StringVar(&host, "host", pkg.DefaultCBSHost, "set the cbs host value")
	flag.IntVar(&port, "port", pkg.DefaultCBSPort, "set the cbs port value")
	flag.BoolVar(&debug, "debug", false, "enable extra debug logging")
	flag.StringVar(&pkg.DataDir, "dir", "./data", "directory with mock data")
	flag.StringVar(&cbsPath, "path", "/usr/bin/cbs", "set the path to find cbs binary")

	flag.Parse()

	cbsPathStr := os.Getenv("CBSPATH")
	if cbsPathStr != "" {
		cbsPath = cbsPathStr
	}

	log.Infof("cbs binary path: %s\n", cbsPath)

	hostStr := os.Getenv("CBSHOST")
	if hostStr != "" {
		host = hostStr
	}

	log.Infof("cbs host: %s\n", host)

	portStr := os.Getenv("CBSPORT")
	if portStr != "" {
		portInt, err := strconv.Atoi(portStr)
		if err != nil {
			log.Warningf("Failed to convert MOCKPORT to int, ignored: %v", portStr)
		} else {
			port = portInt
		}
	}

	log.Infof("cbs port: %d\n", port)

	debugStr := os.Getenv("DEBUG")
	if debugStr != "" {
		debugInt, err := strconv.ParseBool(debugStr)
		if err != nil {
			log.Warningf("Failed to convert DEBUG to bool, ignored: %v", debugStr)
		} else {
			debug = debugInt
		}
	}

	datadirStr := os.Getenv("DATADIR")
	if datadirStr != "" {
		pkg.DataDir = datadirStr
	}

	log.Infof("data directory: %s\n", pkg.DataDir)

	r := gin.Default()

	if debug {
		log.SetLevel(log.DebugLevel)
		gin.SetMode(gin.DebugMode)
	} else {
		log.SetLevel(log.InfoLevel)
		gin.SetMode(gin.ReleaseMode)
	}

	r.GET("/ping", func(c *gin.Context) {
		c.Data(http.StatusOK, "application/json", []byte("pong"))
	})

	r.POST("/cbs", manageCBSHandler)

	r.PUT("/cbs", manageCBSHandler)

	mockAddr := fmt.Sprintf("localhost:%d", port)
	log.Infof("starting cbs wrapper on: %s\n", mockAddr)

	r.Run(mockAddr)
}
