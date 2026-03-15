#pragma link("../c64_road.ld")

#ifndef __INTELLISENSE__
__export char roadbin[] = kickasm(resource "road_8x8.png") {{
    .var pic = LoadPicture("road_8x8.png")
    .segmentdef road
    .segment road
    .word $c800
    .for(var p=0; p<59; p++) {
        .for(var y=0; y<8; y++) {
            .var b = 0
            .for(var x=0; x<8; x++) {
                .if(pic.getPixel(p*8 + x, y) == 0) {
                    .eval b = b*2
                } else {
                    .eval b = b*2 + 1
                }
            }
            .byte b
        }
    }
}};
#endif

void main() {
}
