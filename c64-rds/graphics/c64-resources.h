__export char header[] =kickasm {{

    .struct Sprite {tile, ext, start, count, skip, size, width, height, zorder, flipv, fliph, bpp, collision, reverse, palettecount, loop}

    // sprite = sprite structure containing parameters
    // tiledata = the list containing the tile data in bytes
    // columns = the amount of columns in the tiledata
    .macro sprite_data(sprite, tiledata, columns) {
        // Header
//        .byte sprite.count, sprite.size, sprite.width, sprite.height, sprite.zorder, sprite.fliph, sprite.flipv, sprite.bpp, sprite.collision, sprite.reverse, sprite.loop,0,0,0,0

        // Sprite
        .print "tiledata size = " + tiledata.size() + ", columns = " + columns
        .var c = 0
        .var line = ""
        .for(var i=0;i<tiledata.size();i++) {
            .eval line = line.string() + toBinaryString(tiledata.get(i),8) + " "
            .eval c++
            .if(c == columns) {
                .eval c = 0
                .print line 
                .eval line = ""
            }
            .byte tiledata.get(i)
        }
    }

    // sprite = sprite structure containing parameters
    // tiledata = the list containing the tile data in bytes
    // columns = the amount of columns in the source tiledata
    // shift = the amount of bits to shift to the right
    .function sprite_shift(sprite, tiledata, columns, shift) {

        // Sprite
        // Create shifted tiledata
        .var tileshift = List()
        .var s = tiledata.size()
        .print "tiledata size = " + s + ", columns = " + columns + ", bit shift to right = " + shift
        .var col = 0
        .var line = ""
        .for(var r = 0; r < s; r += columns) {
            .var rowdata = 0;
            .for(var c = 0; c < columns; c++) {
                .eval rowdata = rowdata * 256 + tiledata.get(r+c)
            }
            .eval rowdata *= 256 
            .eval rowdata = rowdata >> shift;

            // We fill the new shiftdata
            .var rowshift = List()
            .for(var c = 0; c < columns+1; c++) {
                .var rowbyte = mod(rowdata, 256)
                .eval rowdata = floor(rowdata / 256)
                .eval rowshift.add(rowbyte)
            }

            .for(var c = columns; c >= 0; c--) {
                .eval tileshift.add(rowshift.get(c))
            }
        }
        .return tileshift
    }
}};


__export char functions[] = 

kickasm {{

    .function MakeTile(bitmap) {
        .var tiledata = List()
        .var image = bitmap.tile + "_" + bitmap.width + "x" + bitmap.height + "." + bitmap.ext
        .var pic = LoadPicture(image)
        .var xoff = bitmap.width * bitmap.start
        .var yoff = 0
        .for(var p=0;p<bitmap.count;p++) {
            .var hstep = 8 / bitmap.bpp
            .var vstep = 1
            .var hinc = 8 / bitmap.bpp
            .var vinc = 1
//            .print "bitmap = " + p
            .for(var j=0; j<bitmap.height; j+=vstep) {
//                .print "j = " + j
                .for(var i=0+xoff; i<bitmap.width+xoff; i+=hstep) {
//                    .print "i = " + i
                    .for (var y=j; y<j+vstep; y+=vinc) {
//                        .print "y = " + y
                        .for (var x=i; x<i+hstep; x+=hinc) {
//                            .print "x = " + x
                            .var val = 0
                            .for(var v=0; v<hinc; v++) {
                                // Find palette index (add if not known)
                                .var rgb = pic.getPixel(x+v,y)
                                //.print "rgb == " + rgb
                                .if(rgb == 0) {
                                    .eval val = val * 2;
                                } else {
                                    .eval val = val * 2 + 1;
                                }
                            }
                            //.print "val = " + val
                            .eval tiledata.add(val);
                        }
                    }
                }
            }
            .eval xoff += bitmap.width * bitmap.skip
        }
        .return tiledata
    }

}};
