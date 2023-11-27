package main

import (
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

type CBSRequest struct {
	Constraints []*proto.Constraint `form:"constraints" json:"constraints" binding:"required"`
	Graph       graph.Graph         `form:"graph" json:"graph" binding:"required"`
}

func runCBS(command string, args []string) error {

	cmd := &exec.Cmd{
		Path:   command,
		Args:   args,
		Stdout: os.Stdout,
		Stderr: os.Stderr,
	}

	log.Infof("Executing command: %s\n", cmd)

	err := cmd.Run()

	log.Infof("Command executed with error: %v\n", err)

	return err
}

func manageCBS(c []*proto.Constraint, g graph.Graph) ([]byte, error) {

	// convert constraints to file
	// vnrFile
	vnrFiName, err := pkg.ConstraintsToFile(c)
	if err != nil {
		return nil, err
	}

	// convert substrait to file
	snFiName, err := pkg.GraphToFile(g)
	if err != nil {
		return nil, err
	}

	// TODO: cbs path modifiable
	err = runCBS(pkg.CBSPath, []string{vnrFiName, snFiName, pkg.DataDir})
	if err != nil {
		return nil, err
	}

	// open outDir json and return as a string
	mapping, err := ioutil.ReadFile(fmt.Sprintf("%s/%s", pkg.DataDir, "out.json"))

	return mapping, err
}

func manageCBSHandler(c *gin.Context) {
	var jsonInput CBSRequest
	if err := c.ShouldBindJSON(&jsonInput); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	constraints := jsonInput.Constraints
	graph := jsonInput.Graph
	result, err := manageCBS(constraints, graph)

	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
	} else {
		c.Data(http.StatusOK, "application/json", result)
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

	flag.Parse()

	hostStr := os.Getenv("CBSHOST")
	if hostStr != "" {
		host = hostStr
	}

	portStr := os.Getenv("CBSPORT")
	if portStr != "" {
		portInt, err := strconv.Atoi(portStr)
		if err != nil {
			log.Warningf("Failed to convert MOCKPORT to int, ignored: %v", portStr)
		} else {
			port = portInt
		}
	}

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
