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
    public String toString(float scale) {
        final DecimalFormat DF = new DecimalFormat("0.########", DecimalFormatSymbols.getInstance(Locale.US));

        float centerX = 180.0f + (x*scale + ((width*scale) / 2.0f))*7.5f;
        float centerY = 570.0f - (y*scale + ((height*scale) / 2.0f))*7.5f;

        return String.format(
            Locale.US,
            "1,917,155,1,2,%s,24,%d,3,%s,25,%d,128,%s,129,%s,21,%d;",
            DF.format(centerX),
            zLayer,
            DF.format(centerY),
            zOrder,
            DF.format(width*scale),
            DF.format(height*scale),
            color
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
                int colorId = uniqueColors.indexOf(col) + 1;
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
                
                //Create Object
                int colorId = uniqueColors.indexOf(col) + 1;
                gdObjects.add(new GDObject(x, y, rectWidth, rectHeight, colorId, zLayer, startingZOrder));
            }
        }
        return gdObjects;
    }

    static List<GDObject> removeHiddenObjects(List<GDObject> objects, int imgWidth, int imgHeight) {
        int[][] cover = new int[imgHeight][imgWidth];
        for (GDObject obj : objects) {
            for (int y = obj.y; y < obj.y + obj.height; y++) {
                for (int x = obj.x; x < obj.x + obj.width; x++) {
                    cover[y][x]++;
                }
            }
        }

        List<GDObject> result = new ArrayList<>();

        for (GDObject obj : objects) {
            boolean fullyCovered = true;
            for (int y = obj.y; y < obj.y + obj.height && fullyCovered; y++) {
                for (int x = obj.x; x < obj.x + obj.width; x++) {
                    // If only this object covers it -> visible
                    if (cover[y][x] <= 1) {
                        fullyCovered = false;
                        break;
                    }
                }
            }
            if (!fullyCovered) {
                result.add(obj);
            }
        }
        return result;
    }

    static int[][] buildOwnerMap(List<GDObject> objects, int w, int h) {

        int[][] owner = new int[h][w];

        // -1 = no owner
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                owner[y][x] = -1;
            }
        }

        // Top to bottom
        for (int i = objects.size() - 1; i >= 0; i--) {

            GDObject obj = objects.get(i);

            for (int y = obj.y; y < obj.y + obj.height; y++) {
                for (int x = obj.x; x < obj.x + obj.width; x++) {

                    if (owner[y][x] == -1) {
                        owner[y][x] = i;
                    }
                }
            }
        }

        return owner;
    }

    static void shrinkToFit(List<GDObject> objects, int[][] owner, int w, int h) {
        int n = objects.size();

        int[] minX = new int[n];
        int[] minY = new int[n];
        int[] maxX = new int[n];
        int[] maxY = new int[n];

        // Init
        for (int i = 0; i < n; i++) {
            minX[i] = Integer.MAX_VALUE;
            minY[i] = Integer.MAX_VALUE;
            maxX[i] = -1;
            maxY[i] = -1;
        }

        // Scan pixels
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int id = owner[y][x];
                if (id == -1) continue;
                minX[id] = Math.min(minX[id], x);
                minY[id] = Math.min(minY[id], y);
                maxX[id] = Math.max(maxX[id], x);
                maxY[id] = Math.max(maxY[id], y);
            }
        }

        // Apply shrink
        for (int i = 0; i < n; i++) {
            // Object ended up owning nothing
            if (maxX[i] < minX[i]) {
                objects.remove(i);
                i--;
                n--;
                continue;
            }

            GDObject obj = objects.get(i);
            obj.x = minX[i];
            obj.y = minY[i];
            obj.width  = maxX[i] - minX[i] + 1;
            obj.height = maxY[i] - minY[i] + 1;
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

    public String[] run(String p, float s, int c, int l, int o) throws IOException{
        BufferedImage image = ImageIO.read(new File(p));

        long startTime = System.currentTimeMillis();
        
        List<GDObject> gdObjects = convertToGDObjects(image, l, o);
        //gdObjects = removeHiddenObjects2(gdObjects, image.getWidth(), image.getHeight());
        //int[][] owner = buildOwnerMap(gdObjects, image.getWidth(), image.getHeight());
        //shrinkToFit(gdObjects, owner, image.getWidth(), image.getHeight());

        long endTime = System.currentTimeMillis();
        float measuredTime = (endTime - startTime) / 1000.0f;
        //System.out.println("time taken: " + measuredTime);

        StringBuilder sb = new StringBuilder("");

        for (GDObject obj : gdObjects) {
            sb.append(obj.toString(s));
        }

        for(int i = 0; i < uniqueColors.size(); i++){
            sb.append(colorToTrigger(i+1, uniqueColors.get(i)));
        }

        String finishedLevelString = sb.toString();
        String[] arr = {String.valueOf(gdObjects.size()), finishedLevelString, Float.toString(measuredTime)};
        return arr;
    }
}
