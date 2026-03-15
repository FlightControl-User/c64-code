.disk [filename="c64-rds.d64", name="C64 RDS", id="64"] {
	[name="SET", type="prg", segments="Basic, Code, Data"],
	[name="ROAD.BIN", type="prg", noStartAddr, segments="road"],
}
.segmentdef Code [start=%P]
.segmentdef Data [startAfter="Code"]
.segment Basic []
BasicUpstart2(%E)
.segment Code
.segment Data