start service:

```
CBSPATH=../cbs DATADIR=./pkg ./service
```

run a curl using a mocked request:

```
curl -X POST -H "Content-Type: application/json" -d @./pkg/mockcbs.request http://localhost:15030/cbs | jq .
```
