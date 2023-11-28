package pkg

import (
	"testing"

	graph "pulwar.isi.edu/sabres/orchestrator/sabres/network/graph"
	proto "pulwar.isi.edu/sabres/orchestrator/sabres/network/protocol"
)

func TestConstraintToFile(t *testing.T) {

	DataDir = "."

	c := []*proto.Constraint{
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "8",
			Object:   "cpu",
			Vertices: []string{"0"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "4",
			Object:   "cpu",
			Vertices: []string{"1"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "100",
			Object:   "bandwidth",
			Vertices: []string{"0", "1"},
		},
	}

	_, err := ConstraintsToFile(c)
	if err != nil {
		t.Errorf("failed to create vnr file: %v\n", err)
	}

	// assert contents =
	/*
	   2 1 0 0 10 0 0
	   0 0 8
	   1 1 4
	   0 1 100 0
	*/

	c = []*proto.Constraint{
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "8",
			Object:   "cpu",
			Vertices: []string{"0"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "4",
			Object:   "cpu",
			Vertices: []string{"1"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "8",
			Object:   "cpu",
			Vertices: []string{"2"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "100",
			Object:   "bandwidth",
			Vertices: []string{"0", "1"},
		},
		&proto.Constraint{
			Operator: ">",
			Lvalue:   "1000",
			Object:   "bandwidth",
			Vertices: []string{"1", "2"},
		},
	}

	_, err = ConstraintsToFile(c)
	if err != nil {
		t.Errorf("failed to create vnr file: %v\n", err)
	}

	// assert file contents =

	/*
		3 2 0 0 10 0 0
		0 0 8
		1 1 4
		2 2 8
		0 1 100 0
		1 2 1000 0
	*/

}

func TestGraphToFile(t *testing.T) {

	g := &graph.Graph{
		Name: "test1",
		Vertices: []*graph.Vertex{
			&graph.Vertex{
				Name:       "alpha",
				Value:      "",
				Properties: map[string]string{"cpu": "16"},
			},
			&graph.Vertex{
				Name:       "beta",
				Value:      "",
				Properties: map[string]string{"cpu": "2"},
			},
			&graph.Vertex{
				Name:       "charlie",
				Value:      "",
				Properties: map[string]string{"cpu": "32"},
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
						Properties: map[string]string{"cpu": "2"},
					},
				},
				Properties: map[string]string{"bandwidth": "100", "latency": "5"},
			},
			&graph.Edge{
				Name: "beta-charlie",
				Vertices: []*graph.Vertex{
					&graph.Vertex{
						Name:       "beta",
						Value:      "",
						Properties: map[string]string{"cpu": "2"},
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

	_, err := GraphToFile(g)
	if err != nil {
		t.Errorf("failed to create substrait file: %v\n", err)
	}

	/*
		3 2
		0 0 16
		0 0 2
		0 0 32
		0 1 100 5
		1 2 500 15

	*/

}
