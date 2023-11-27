package pkg

import (
	"testing"

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
