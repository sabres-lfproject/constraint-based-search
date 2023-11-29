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

	"pulwar.isi.edu/cbs/cbs/service/pkg"
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

	return fiName, err
}

func manageCBS(c []*proto.Constraint, g *graph.Graph) (string, map[string]int, error) {

	// convert substrait to file
	snFiName, m, err := pkg.GraphToFile(g)
	if err != nil {
		//return nil, make(map[string]int), err
		return "", nil, err
	}

	// convert constraints to file
	// vnrFile
	vnrFiName, err := pkg.ConstraintsToFile(c, m)
	if err != nil {
		return "", nil, err
	}

	log.Infof("vnrfile name: %s\n", vnrFiName)
	log.Infof("snfile name: %s\n", snFiName)

	// args[0] is executable
	fiNameOut, err := runCBS(cbsPath, []string{cbsPath, vnrFiName, snFiName, pkg.DataDir})
	if err != nil {
		return "", nil, err
	}

	// open outDir json and return as a string
	//mapping, err := ioutil.ReadFile(fmt.Sprintf("%s/%s", pkg.DataDir, "out.json"))

	return fiNameOut, m, err
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
	var jsonInput pkg.CBSRequest
	if err := c.ShouldBindJSON(&jsonInput); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	constraints := jsonInput.Constraints
	graph := jsonInput.Graph
	outFiName, mapToStr, err := manageCBS(constraints, graph)

	if err != nil {
		contents, err2 := ioutil.ReadFile(outFiName)
		if err2 != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "error2": err2.Error()})
		}

		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "output": contents})
	} else {

		// need to open up the json now
		var content *CBSOutput
		jsonContents, err := ioutil.ReadFile(fmt.Sprintf("%s/%s", pkg.DataDir, "out.json"))
		err = json.Unmarshal(jsonContents, &content)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error(), "output": jsonContents})
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
			for k, v := range mapToStr {
				if e.Source == v {
					src = k
				}
				if e.Destination == v {
					dst = k
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
			for k, v := range mapToStr {
				if n.Node == v {
					node = k
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
