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
    public String toString(float scale, int startColorID) {
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
            zOrder
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

    public static List<GDObject> convertToGDObjects(BufferedImage image, int zLayer, int startingZOrder) {
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

                int rightCount = 0;
                boolean rightCheck;

                //Expand Right (only necessary overlap)
                while (canExpandRight && x + rectWidth < width) {
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
                while (canExpandDown && y + rectHeight < height) {
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
                while (canExpandLeft && x > 0) {
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
        return gdObjects;
    }

    static List<GDObject> removeRedundantObjects(List<GDObject> objects, int[][][] cov) {
        List<GDObject> nonRedundant = new ArrayList<>();

        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            //boolean[][] hidden = new boolean[obj.width][obj.height];
            boolean visible = false;

            for(int y = obj.y; y < obj.y+obj.height; y++){
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    if(cov[x][y][obj.color] == 1){
                        //hidden[x-obj.x][y-obj.y] = true;
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
                }
            }

            curr.zOrder = maxZ;
        }
    }

    static void shrinkObjects(List<GDObject> objects, BufferedImage image) {
        for (GDObject obj : objects) {
            int minX = obj.x;
            int minY = obj.y;
            int maxX = obj.x + obj.width - 1;
            int maxY = obj.y + obj.height - 1;

            boolean shrink = true;

            // Shrink from top
            while (minY <= maxY && shrink) {
                for (int x = minX; x <= maxX; x++) {
                    int p = image.getRGB(x, minY);
                    int alpha = (p >>> 24) & 0xFF;
                    int rgb   = p & 0xFFFFFF;
                    if (alpha != 0 && rgb == (new Color(uniqueColors.get(obj.color).getRGB()).getRGB() & 0xFFFFFF)) {
                        shrink = false;
                        break;
                    }
                }
                if (shrink) minY++;
                else break;
            }

            shrink = true;
            // Shrink from bottom
            while (maxY >= minY && shrink) {
                for (int x = minX; x <= maxX; x++) {
                    int p = image.getRGB(x, maxY);
                    int alpha = (p >>> 24) & 0xFF;
                    int rgb   = p & 0xFFFFFF;
                    if (alpha != 0 && rgb == (new Color(uniqueColors.get(obj.color).getRGB()).getRGB() & 0xFFFFFF)) {
                        shrink = false;
                        break;
                    }
                }
                if (shrink) maxY--;
                else break;
            }

            shrink = true;
            // Shrink from left
            while (minX <= maxX && shrink) {
                for (int y = minY; y <= maxY; y++) {
                    int p = image.getRGB(minX, y);
                    int alpha = (p >>> 24) & 0xFF;
                    int rgb   = p & 0xFFFFFF;
                    if (alpha != 0 && rgb == (new Color(uniqueColors.get(obj.color).getRGB()).getRGB() & 0xFFFFFF)) {
                        shrink = false;
                        break;
                    }
                }
                if (shrink) minX++;
                else break;
            }

            shrink = true;
            // Shrink from right
            while (maxX >= minX && shrink) {
                for (int y = minY; y <= maxY; y++) {
                    int p = image.getRGB(maxX, y);
                    int alpha = (p >>> 24) & 0xFF;
                    int rgb   = p & 0xFFFFFF;
                    if (alpha != 0 && rgb == (new Color(uniqueColors.get(obj.color).getRGB()).getRGB() & 0xFFFFFF)) {
                        shrink = false;
                        break;
                    }
                }
                if (shrink) maxX--;
                else break;
            }

            // Apply new dimensions
            obj.x = minX;
            obj.y = minY;
            obj.width  = maxX - minX + 1;
            obj.height = maxY - minY + 1;
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

    static public int[][][] getCoverage(List<GDObject> objects, int width, int height){
        int[][][] coverage = new int[width][height][uniqueColors.size()];

        for(int i = 0; i < objects.size(); i++){
            GDObject obj = objects.get(i);
            for(int y = obj.y; y < obj.y+obj.height; y++){
                for(int x = obj.x; x < obj.x+obj.width; x++){
                    coverage[x][y][obj.color] += 1;
                }
            }
        }

        return coverage;
    }

    public String[] run(String p, float s, int c, int l, int o) throws IOException{
        BufferedImage image = ImageIO.read(new File(p));

        long startTime = System.currentTimeMillis();
        
        List<GDObject> gdObjects = convertToGDObjects(image, l, o);
        int[][][] coverage = getCoverage(gdObjects, image.getWidth(), image.getHeight());
        gdObjects = removeRedundantObjects(gdObjects, coverage);
        shrinkObjects(gdObjects, image);
        assignZOrder(gdObjects);

        long endTime = System.currentTimeMillis();
        float measuredTime = (endTime - startTime) / 1000.0f;
        //System.out.println("time taken: " + measuredTime);

        
        StringBuilder sb = new StringBuilder("");
        
        for (GDObject obj : gdObjects) {
            sb.append(obj.toString(s, c));
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
