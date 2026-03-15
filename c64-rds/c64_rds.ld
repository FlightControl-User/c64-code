.disk [filename="c64_rds.d64", name="C64 RDS", id="64"] {
    [name="RDS", type="prg", segments="Program"],
    [name="ROAD.BIN", type="prg", noStartAddr, segments="road"],
}
.segmentdef Program [segments="Basic, Code, Data"]
.segmentdef Basic [start=$0801]
.segmentdef Code [start=%P]
.segmentdef Data [startAfter="Code"]
.segmentdef road []

.segment Basic
:BasicUpstart2(main)
.segment Code
.segment Data

.segment road
.import binary "C:/Users/svenv/OneDrive/Documents/GitHub/c64-code/.target/c64-rds/ROAD.BIN"
