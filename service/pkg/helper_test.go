package pkg

import (
	"testing"

	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

func TestConstraintToFile(t *testing.T) {

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

	/*
		// assert contents =
		c = []*proto.Constraint{
			&proto.Constraint{
				Operator: ">",
				Lvalue:   "8",
				Object:   "cpu",
				Vertices: []string{"", ""},
			},
			&proto.Constraint{
				Operator: ">",
				Lvalue:   "8",
				Object:   "cpu",
				Vertices: []string{"", ""},
			},
			&proto.Constraint{
				Operator: ">",
				Lvalue:   "250",
				Object:   "bandwidth",
				Vertices: []string{"", ""},
			},
		}

		_, err = ConstraintsToFile(c, m)
		if err != nil {
			t.Errorf("failed to create vnr file: %v\n", err)
		}

		// assert file contents =
	*/
}

/*
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
*/
