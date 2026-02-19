package ImgConverter;

import javax.imageio.ImageIO;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

class GDObject {
    int x, y, width, height;
    int color, zLayer, zOrder;

    public GDObject(int x, int y, int width, int height, int color, int zLayer, int zOrder) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.color = color;
        this.zLayer = zLayer;
        this.zOrder = zOrder;
    }
    
    //@Override
    public String toString(float scale, int startColorID, int startZOrder) {
        final DecimalFormat DF = new DecimalFormat("0.########", DecimalFormatSymbols.getInstance(Locale.US));

        float centerX = 180.0f + (x*scale + ((width*scale) / 2.0f))*7.5f;
        float centerY = 570.0f - (y*scale + ((height*scale) / 2.0f))*7.5f;

        return String.format(
            Locale.US,
            "1,917,155,1,2,%s,24,%d,3,%s,25,%d,128,%s,129,%s,21,%d,20,%d;",
            DF.format(centerX),
            zLayer,
            DF.format(centerY),
            zOrder,
            DF.format(width*scale),
            DF.format(height*scale),
            startColorID+color,
            zOrder-startZOrder
        );
    }
}

public class Pix2GD {
    static List<Color> uniqueColors = new ArrayList<>();

    public static List<GDObject> convertToGDObjectsSingleColor(BufferedImage image, int zLayer, int startingZOrder) {
        List<GDObject> gdObjects = new ArrayList<>();
        int width  = image.getWidth();
        int height = image.getHeight();
        boolean[][] processed = new boolean[height][width];

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (processed[y][x]) continue;

                // Read base pixel
                int base = image.getRGB(x, y);
                int baseAlpha = (base >>> 24) & 0xFF;
                int baseRGB   = base & 0xFFFFFF;

                // Skip transparent
                if (baseAlpha == 0) {
                    processed[y][x] = true;
                    continue;
                }

                int rectWidth  = 1;
                int rectHeight = 1;

                boolean canExpandRight = true;
                boolean canExpandLeft  = true;
                boolean canExpandDown  = true;
                boolean canExpandUp    = true;

                //Expand Right (only necessary overlap)
                while (canExpandRight && x + rectWidth < width) {
                    int nx = x + rectWidth;
                    for (int i = 0; i < rectHeight; i++) {
                        int ny = y + i;
                        int p = image.getRGB(nx, ny);
                        int alpha = (p >>> 24) & 0xFF;
                        int rgb   = p & 0xFFFFFF;

                        if (alpha == 0 || rgb != baseRGB) {
                            canExpandRight = false;
                            break;
                        }
                    }
                    if (canExpandRight) {
                        rectWidth++;
                    }
                }

                //Expand Down
                while (canExpandDown && y + rectHeight < height) {
                    int ny = y + rectHeight;
                    for (int i = 0; i < rectWidth; i++) {
                        int nx = x + i;
                        int p = image.getRGB(nx, ny);
                        int alpha = (p >>> 24) & 0xFF;
                        int rgb   = p & 0xFFFFFF;

                        if (alpha == 0 || rgb != baseRGB) {
                            canExpandDown = false;
                            break;
                        }
                    }
                    if (canExpandDown) {
                        rectHeight++;
                    }
                }

                //Expand Left
                while (canExpandLeft && x > 0) {
                    int nx = x - 1;
                    for (int i = 0; i < rectHeight; i++) {
                        int ny = y + i;
                        int p = image.getRGB(nx, ny);
                        int alpha = (p >>> 24) & 0xFF;
                        int rgb   = p & 0xFFFFFF;

                        if (alpha == 0 || rgb != baseRGB) {
                            canExpandLeft = false;
                            break;
                        }
                    }
                    if (canExpandLeft) {
                        rectWidth++;
                        x--;
                    }
                }

                //Expand Up
                while (canExpandUp && y > 0) {
                    int ny = y - 1;
                    for (int i = 0; i < rectWidth; i++) {
                        int nx = x + i;
                        int p = image.getRGB(nx, ny);
                        int alpha = (p >>> 24) & 0xFF;
                        int rgb   = p & 0xFFFFFF;

                        if (alpha == 0 || rgb != baseRGB) {
                            canExpandUp = false;
                            break;
                        }
                    }
                    if (canExpandUp) {
                        rectHeight++;
                        y--;
                    }
                }

                //Mark Processed
                for (int dy = 0; dy < rectHeight; dy++) {
                    for (int dx = 0; dx < rectWidth; dx++) {
                        processed[y + dy][x + dx] = true;
                    }
                }

                // Convert RGB back to Color
                Color col = new Color(baseRGB);
                if (!uniqueColors.contains(col)) {
                    uniqueColors.add(col);
                }
                
                //Create Object
                int colorId = uniqueColors.indexOf(col);
                gdObjects.add(new GDObject(x, y, rectWidth, rectHeight, colorId, zLayer, startingZOrder));
            }
        }
        return gdObjects;
    }

    public static List<GDObject> convertToGDObjectsTiled(BufferedImage image, int zLayer, int startingZOrder, int tileWidth, int tileHeight) {
        List<GDObject> gdObjects = new ArrayList<>();
        int imgWidth  = image.getWidth();
        int imgHeight = image.getHeight();
        boolean[][] processed = new boolean[imgHeight][imgWidth];

        if(tileHeight == 0) tileHeight = imgHeight;
        if(tileWidth == 0) tileWidth = imgWidth;

        for(int tx = 0; tx < (float)imgWidth/(float)tileWidth; tx++){
            for(int ty = 0; ty < (float)imgHeight/(float)tileHeight; ty++){
                int tWidth = tx*tileWidth+tileWidth;
                int tHeight = ty*tileHeight+tileHeight;
                for (int y = ty*tileHeight; y < tHeight && y < imgHeight; y++) {
                    for (int x = tx*tileWidth; x < tWidth && x < imgWidth; x++) {
                        if (processed[y][x]) continue;

                        // Read base pixel
                        int base = image.getRGB(x, y);
                        int baseAlpha = (base >>> 24) & 0xFF;
                        int baseRGB   = base & 0xFFFFFF;

                        // Skip transparent
                        if (baseAlpha == 0) {
                            processed[y][x] = true;
                            continue;
                        }

                        int rectWidth  = 1;
                        int rectHeight = 1;

                        boolean canExpandRight = true;
                        boolean canExpandLeft  = true;
                        boolean canExpandDown  = true;
                        boolean canExpandUp    = true;

                        int rightCount = 0;
                        boolean rightCheck;

                        //Expand Right (only necessary overlap)
                        while (canExpandRight && x + rectWidth < imgWidth && x + rectWidth < tWidth) {
                            rightCheck = false;
                            int nx = x + rectWidth;
                            for (int i = 0; i < rectHeight; i++) {
                                int ny = y + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandRight = false;
                                    break;
                                }
                                else if (!processed[ny][nx]) {
                                    rightCheck = true;
                                }
                            }
                            if (canExpandRight) {
                                rectWidth++;
                                if (rightCheck) rightCount = 0;
                                else rightCount++;
                            }
                        }
                        rectWidth -= rightCount;

                        //Expand Down
                        while (canExpandDown && y + rectHeight < imgHeight && y + rectHeight < tHeight) {
                            int ny = y + rectHeight;
                            for (int i = 0; i < rectWidth; i++) {
                                int nx = x + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandDown = false;
                                    break;
                                }
                            }
                            if (canExpandDown) {
                                rectHeight++;
                            }
                        }

                        //Expand Left
                        while (canExpandLeft && x > tx*tileWidth) {
                            int nx = x - 1;
                            for (int i = 0; i < rectHeight; i++) {
                                int ny = y + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandLeft = false;
                                    break;
                                }
                            }
                            if (canExpandLeft) {
                                rectWidth++;
                                x--;
                            }
                        }

                        //Expand Up
                        while (canExpandUp && y > ty*tileHeight) {
                            int ny = y - 1;
                            for (int i = 0; i < rectWidth; i++) {
                                int nx = x + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || rgb != baseRGB) {
                                    canExpandUp = false;
                                    break;
                                }
                            }
                            if (canExpandUp) {
                                rectHeight++;
                                y--;
                            }
                        }

                        //Mark Processed
                        for (int dy = 0; dy < rectHeight; dy++) {
                            for (int dx = 0; dx < rectWidth; dx++) {
                                int p = image.getRGB(x + dx, y + dy);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;
                                if (alpha != 0 && rgb == baseRGB) {
                                    processed[y + dy][x + dx] = true;
                                }
                            }
                        }

                        // Convert RGB back to Color
                        Color col = new Color(baseRGB);
                        if (!uniqueColors.contains(col)) {
                            uniqueColors.add(col);
                        }

                        int colorId = uniqueColors.indexOf(col);
                        
                        //Create Object
                        gdObjects.add(new GDObject(x, y, rectWidth, rectHeight, colorId, zLayer, startingZOrder));
                    }
                }
            }
        }
        return gdObjects;
    }

    static public int[][] getCoverage(List<GDObject> objects, int width, int height){
        int[][] coverage = new int[width][height];

        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            for(int y = obj.y; y < obj.y+obj.height; y++){
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    //if the object has a different color, its visible on top, save its index
                    if(coverage[x][y] == 0 || objects.get(Math.abs(coverage[x][y])-1).color != obj.color){
                        coverage[x][y] = i+1;
                    }
                    else{ //if the same color behind, then its negative to indicate this
                        coverage[x][y] = -(i+1);
                    }
                }
            }
        }
        return coverage;
    }

    static List<GDObject> removeRedundantObjects(List<GDObject> objects, int[][] cov) {
        List<GDObject> nonRedundant = new ArrayList<>();

        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            boolean visible = false;

            for(int y = obj.y; y < obj.y+obj.height; y++){
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    if(Math.abs(cov[x][y])-1 == i && cov[x][y] > 0){
                        nonRedundant.add(obj);
                        visible = true;
                        break;
                    }
                }
                if(visible) break;
            }

        }
        return nonRedundant;
    }

    static void assignZOrder(List<GDObject> objects) {
        for (int i = 0; i < objects.size(); i++) {
            GDObject curr = objects.get(i);

            int maxZ = curr.zOrder; // start with current zOrder

            for (int j = 0; j < i; j++) {
                GDObject prev = objects.get(j);

                //same color can be on the same z-order
                if(prev.color == curr.color){
                    continue;
                }

                // Check if prev overlaps curr
                boolean overlapX = curr.x < prev.x + prev.width && curr.x + curr.width > prev.x;
                boolean overlapY = curr.y < prev.y + prev.height && curr.y + curr.height > prev.y;
                if (overlapX && overlapY) {
                    maxZ = Math.max(maxZ, prev.zOrder + 1);
                    if(maxZ == 0){
                        maxZ++;
                    }
                }
            }

            curr.zOrder = maxZ;
        }
    }
    
    static void shrinkObjects(List<GDObject> objects, BufferedImage image){
        //int[][] cov = getCoverage(objects, image.getWidth(), image.getHeight());

        //create coverage list
        @SuppressWarnings("unchecked")
        List<Integer>[][] cov = (List<Integer>[][]) new ArrayList[image.getWidth()][image.getHeight()];
        for (int x = 0; x < image.getWidth(); x++) {
            for (int y = 0; y < image.getHeight(); y++) {
                cov[x][y] = new ArrayList<>();
            }
        }

        //fill coverage list
        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            for(int y = obj.y; y < obj.y+obj.height; y++){
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    cov[x][y].add(i);
                }
            }
        }
        
        //shrink objects
        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            boolean canShrink = true;

            //shrink top
            while(canShrink && obj.height > 1){
                int y = obj.y;
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    int size = cov[x][y].size();
                    int ind = cov[x][y].get(size-1);
                    if(ind == i && (size == 1 || objects.get(cov[x][y].get(size-2)).color != obj.color)){
                        canShrink = false;
                        break;
                    }
                }
                if(canShrink){
                    obj.y++;
                    obj.height--;
                    for(int x = obj.x; x < obj.x+obj.width; x++){
                        cov[x][y].remove(cov[x][y].indexOf(i));
                    }
                }
            }

            //shrink bottom
            canShrink = true;
            while(canShrink && obj.height > 1){
                int y = obj.y+obj.height-1;
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    int size = cov[x][y].size();
                    int ind = cov[x][y].get(size-1);
                    if(ind == i && (size == 1 || objects.get(cov[x][y].get(size-2)).color != obj.color)){
                        canShrink = false;
                        break;
                    }
                }
                if(canShrink){
                    obj.height--;
                    for(int x = obj.x; x < obj.x+obj.width; x++){
                        cov[x][y].remove(cov[x][y].indexOf(i));
                    }
                }
            }

            //shrink left
            canShrink = true;
            while(canShrink && obj.width > 1){
                int x = obj.x;
                for(int y = obj.y; y < obj.y+obj.height; y++){
                    int size = cov[x][y].size();
                    int ind = cov[x][y].get(size-1);
                    if(ind == i && (size == 1 || objects.get(cov[x][y].get(size-2)).color != obj.color)){
                        canShrink = false;
                        break;
                    }
                }
                if(canShrink){
                    obj.x++;
                    obj.width--;
                    for(int y = obj.y; y < obj.y+obj.height; y++){
                        cov[x][y].remove(cov[x][y].indexOf(i));
                    }
                }
            }

            //shrink right
            canShrink = true;
            while(canShrink && obj.width > 1){
                int x = obj.x+obj.width-1;
                for(int y = obj.y; y < obj.y+obj.height; y++){
                    int size = cov[x][y].size();
                    int ind = cov[x][y].get(size-1);
                    if(ind == i && (size == 1 || objects.get(cov[x][y].get(size-2)).color != obj.color)){
                        canShrink = false;
                        break;
                    }
                }
                if(canShrink){
                    obj.width--;
                    for(int y = obj.y; y < obj.y+obj.height; y++){
                        cov[x][y].remove(cov[x][y].indexOf(i));
                    }
                }
            }
        }
    }
    
    static public String colorToTrigger(int id, Color color){
        final DecimalFormat DF = new DecimalFormat("0.########", DecimalFormatSymbols.getInstance(Locale.US));
        float[] rgb = color.getRGBColorComponents(null);

        return String.format(
            Locale.US,
            "1,899,2,0,3,%s,155,1,36,1,7,%d,8,%d,9,%d,10,0,35,1,23,%d;",
            DF.format(id*15),
            (int)(rgb[0]*255),
            (int)(rgb[1]*255),
            (int)(rgb[2]*255),
            id
        );
    }

    public String[] run(String p, float s, int c, int l, int o, int tileWidth, int tileHeight) throws IOException{
        BufferedImage image = ImageIO.read(new File(p));

        long startTime = System.currentTimeMillis();
        
        List<GDObject> gdObjects = convertToGDObjectsTiled(image, l, o, tileWidth, tileHeight);
        int[][] coverage = getCoverage(gdObjects, image.getWidth(), image.getHeight());
        gdObjects = removeRedundantObjects(gdObjects, coverage);
        shrinkObjects(gdObjects, image);
        assignZOrder(gdObjects);

        long endTime = System.currentTimeMillis();
        float measuredTime = (endTime - startTime) / 1000.0f;

        
        StringBuilder sb = new StringBuilder("");
        
        for (GDObject obj : gdObjects) {
            sb.append(obj.toString(s, c, o));
        }
        
        for(int i = 0; i < uniqueColors.size(); i++){
            sb.append(colorToTrigger(i+c, uniqueColors.get(i)));
        }

        uniqueColors.clear(); //empty it in case you run again

        String finishedLevelString = sb.toString();
        String[] arr = {String.valueOf(gdObjects.size()), finishedLevelString, Float.toString(measuredTime)};
        return arr;
    }
}
