package pkg

import (
	"encoding/json"
	"fmt"
	"testing"

	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

func TestConstraintToFile1(t *testing.T) {

	DataDir = "."

	g := &graph.Graph{
		Name: "test-constraint-graph",
		Vertices: []*graph.Vertex{
			&graph.Vertex{
				Name:       "alpha",
				Properties: map[string]string{"cpu": "16", "endpoint": "yes"},
			},
			&graph.Vertex{
				Name:       "beta",
				Properties: map[string]string{"cpu": "8"},
			},
			&graph.Vertex{
				Name:       "charlie",
				Properties: map[string]string{"cpu": "32", "endpoint": "yes"},
			},
		},
		Edges: []*graph.Edge{
			&graph.Edge{
				Name: "alpha-beta",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name: "alpha",
					},
					&graph.Vertex{
						Name: "beta",
					},
				},
				Properties: map[string]string{"bandwidth": "500", "latency": "5"},
			},
			&graph.Edge{
				Name: "alpha-charlie",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name: "alpha",
					},
					&graph.Vertex{
						Name: "charlie",
					},
				},
				Properties: map[string]string{"bandwidth": "500", "latency": "10"},
			},
			&graph.Edge{
				Name: "beta-charlie",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name: "beta",
					},
					&graph.Vertex{
						Name: "charlie",
					},
				},
				Properties: map[string]string{"bandwidth": "500", "latency": "15"},
			},
		},
	}

	_, m, err := GraphToFile(g)
	if err != nil {
		t.Error(err)
	}

	c := []*proto.Constraint{
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "4",
			Object:   "cpu",
			Vertices: []string{"alpha"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "4",
			Object:   "cpu",
			Vertices: []string{"beta"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "16",
			Object:   "cpu",
			Vertices: []string{"charlie"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "100",
			Object:   "bandwidth",
			Vertices: []string{"alpha", "beta"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "100",
			Object:   "bandwidth",
			Vertices: []string{"alpha", "charlie"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "500",
			Object:   "bandwidth",
			Vertices: []string{"beta", "charlie"},
		},
	}

	_, err = ConstraintsToFile(c, m)
	if err != nil {
		t.Errorf("failed to create vnr file: %v\n", err)
	}

}

func TestConstraintToFile2(t *testing.T) {

	DataDir = "."

	g := &graph.Graph{
		Name: "test-constraint-graph",
		Vertices: []*graph.Vertex{
			&graph.Vertex{
				Name:       "alpha",
				Properties: map[string]string{"cpu": "8", "endpoint": "yes"},
			},
			&graph.Vertex{
				Name:       "beta",
				Properties: map[string]string{"cpu": "2"},
			},
			&graph.Vertex{
				Name:       "charlie",
				Properties: map[string]string{"cpu": "8", "endpoint": "yes"},
			},
		},
		Edges: []*graph.Edge{
			&graph.Edge{
				Name: "alpha-beta",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name: "alpha",
					},
					&graph.Vertex{
						Name: "beta",
					},
				},
				Properties: map[string]string{"bandwidth": "100", "latency": "5"},
			},
			&graph.Edge{
				Name: "beta-charlie",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name: "beta",
					},
					&graph.Vertex{
						Name: "charlie",
					},
				},
				Properties: map[string]string{"bandwidth": "100", "latency": "15"},
			},
		},
	}

	_, m, err := GraphToFile(g)
	if err != nil {
		t.Error(err)
	}

	// assert contents =
	c := []*proto.Constraint{
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "8",
			Object:   "cpu",
			Vertices: []string{"alpha"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "8",
			Object:   "cpu",
			Vertices: []string{"charlie"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "50",
			Object:   "bandwidth",
			Vertices: []string{"alpha", "charlie"},
		},
	}

	_, err = ConstraintsToFile(c, m)
	if err != nil {
		t.Errorf("failed to create vnr file: %v\n", err)
	}

}

func TestGraphToFile(t *testing.T) {

	g := &graph.Graph{
		Name: "test1",
		Vertices: []*graph.Vertex{
			&graph.Vertex{
				Name:       "alpha",
				Value:      "",
				Properties: map[string]string{"cpu": "16", "endpoint": "yes"},
			},
			&graph.Vertex{
				Name:       "beta",
				Value:      "",
				Properties: map[string]string{"cpu": "8"},
			},
			&graph.Vertex{
				Name:       "charlie",
				Value:      "",
				Properties: map[string]string{"cpu": "32", "endpoint": "yes"},
			},
		},
		Edges: []*graph.Edge{
			&graph.Edge{
				Name: "alpha-beta",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name:       "alpha",
						Value:      "",
						Properties: map[string]string{"cpu": "16"},
					},
					&graph.Vertex{
						Name:       "beta",
						Value:      "",
						Properties: map[string]string{"cpu": "8"},
					},
				},
				Properties: map[string]string{"bandwidth": "500", "latency": "5"},
			},
			&graph.Edge{
				Name: "beta-charlie",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name:       "beta",
						Value:      "",
						Properties: map[string]string{"cpu": "8"},
					},
					&graph.Vertex{
						Name:       "charlie",
						Value:      "",
						Properties: map[string]string{"cpu": "32"},
					},
				},
				Properties: map[string]string{"bandwidth": "500", "latency": "15"},
			},
		},
	}

	_, _, err := GraphToFile(g)
	if err != nil {
		t.Errorf("failed to create substrait file: %v\n", err)
	}

	gprime, err := GraphFromFile()
	if err != nil {
		t.Errorf("failed to read graph file: %v\n", err)
	}

	if len(g.Edges) != len(gprime.Edges) {
		t.Errorf("graph in memory different from in file: mismatch edges")
	}

	if len(g.Vertices) != len(g.Vertices) {
		t.Errorf("graph in memory different from in file: mismatch vertices")
	}

}

func TestCBSJsonReader(t *testing.T) {
	var jsonBlob = []byte(`{
  "constraints": [
    {
      "operator": ">",
      "lvalue": "100",
      "object": "cpu",
      "vertices": [
        "alice"
      ]
    },
    {
      "operator": ">",
      "lvalue": "1",
      "object": "cpu",
      "vertices": [
        "charlie"
      ]
    },
    {
      "operator": ">",
      "lvalue": "100",
      "object": "bandwidth",
      "vertices": [
        "alice",
        "charlie"
      ]
    }
  ],
  "graph": {
    "name": "test-blob",
    "vertices": [
      {
        "name": "alice",
        "weight": 10,
        "properties": {
          "cpu": "200"
        }
      },
      {
        "name": "bob",
        "weight": 11,
        "properties": {
          "cpu": "120"
        }
      },
      {
        "name": "charlie",
        "value": "12",
        "properties": {
          "cpu": "2"
        }
      }
    ],
    "edges": [
      {
        "name": "alice-bob",
        "vertices": [
          {
            "name": "alice"
          },
          {
            "name": "bob"
          }
        ],
        "properties": {
          "bandwidth": "200"
        }
      },
      {
        "name": "bob-charlie",
        "vertices": [
          {
            "name": "bob"
          },
          {
            "name": "charlie"
          }
        ],
        "properties": {
          "bandwidth": "200"
        }
      }
    ]
  }
}`)

	var request CBSRequest
	err := json.Unmarshal(jsonBlob, &request)
	if err != nil {
		t.Error(err)
	}

	fmt.Printf("Constraints:\n")
	for _, c := range request.Constraints {
		fmt.Printf("%#v\n", c)
	}
	fmt.Printf("Graph:\n")
	for _, v := range request.Graph.Vertices {
		fmt.Printf("%#v\n", v)
	}
	for _, e := range request.Graph.Edges {
		fmt.Printf("%#v\n", e)
	}

}
