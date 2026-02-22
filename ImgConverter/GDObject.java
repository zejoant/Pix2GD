package ImgConverter;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

class GDObject {
    int x, y, width, height;
    int color, zLayer, zOrder;
    int linkID;

    public GDObject(int x, int y, int width, int height, int color, int zLayer, int zOrder, int linkID) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.color = color;
        this.zLayer = zLayer;
        this.zOrder = zOrder;
        this.linkID = linkID;
    }
    
    //@Override
    public String toString(float scale, int startColorID, int startZOrder) {
        final DecimalFormat DF = new DecimalFormat("0.########", DecimalFormatSymbols.getInstance(Locale.US));

        float centerX = 180.0f + (x*scale + ((width*scale) / 2.0f))*7.5f;
        float centerY = 570.0f - (y*scale + ((height*scale) / 2.0f))*7.5f;

        String obj = String.format(
            Locale.US,
            "1,917,155,1,2,%s,24,%d,3,%s,25,%d,128,%s,129,%s,21,%d,20,%d",
            DF.format(centerX),
            zLayer,
            DF.format(centerY),
            zOrder,
            DF.format(width*scale),
            DF.format(height*scale),
            startColorID+color,
            zOrder-startZOrder
        );
        if(linkID != -1){
            obj = obj + ",108," + linkID;
        }
        return obj + ";";
    }
}
