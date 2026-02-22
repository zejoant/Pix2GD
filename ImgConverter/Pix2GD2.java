package ImgConverter;

import javax.imageio.ImageIO;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

public class Pix2GD2 {
    static List<Color> uniqueColors = new ArrayList<>();

    public static List<GDObject> convertToGDObjectsTiled(BufferedImage image, int zLayer, int startingZOrder, int tileWidth, int tileHeight, int maxLinked) {
        List<GDObject> gdObjects = new ArrayList<>();
        int imgWidth  = image.getWidth();
        int imgHeight = image.getHeight();
        boolean[][] processed = new boolean[imgHeight][imgWidth];
        int[][] cov = new int[imgHeight][imgWidth];

        if(tileHeight == 0) tileHeight = imgHeight;
        if(tileWidth == 0) tileWidth = imgWidth;

        for(int tx = 0; tx < (float)imgWidth/(float)tileWidth; tx++){
            for(int ty = 0; ty < (float)imgHeight/(float)tileHeight; ty++){
                int tWidth = tx*tileWidth+tileWidth;
                int tHeight = ty*tileHeight+tileHeight;
                maxLinked += 1;
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

                        int revertCount = 0;
                        boolean usefulCheck;

                        int minInsertIndex = 0;

                        if(cov[y][x] > minInsertIndex){
                            minInsertIndex = cov[y][x];
                        }

                        //Expand Right (only necessary overlap)
                        while (canExpandRight && x + rectWidth < imgWidth && x + rectWidth < tWidth) {
                            usefulCheck = false;
                            int nx = x + rectWidth;
                            for (int i = 0; i < rectHeight; i++) {
                                int ny = y + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandRight = false;
                                    if(cov[ny][nx] > minInsertIndex){
                                        minInsertIndex = cov[ny][nx];
                                    }
                                    break;
                                }
                                else if (!processed[ny][nx] && rgb == baseRGB) {
                                    usefulCheck = true;
                                }
                            }
                            if (canExpandRight) {
                                rectWidth++;
                                if (usefulCheck) revertCount = 0;
                                else revertCount++;
                            }
                        }
                        rectWidth -= revertCount;
                        revertCount = 0;

                        //Expand Down (only necessary overlap)
                        while (canExpandDown && y + rectHeight < imgHeight && y + rectHeight < tHeight) {
                            usefulCheck = false;
                            int ny = y + rectHeight;
                            for (int i = 0; i < rectWidth; i++) {
                                int nx = x + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandDown = false;
                                    if(cov[ny][nx] > minInsertIndex){
                                        minInsertIndex = cov[ny][nx];
                                    }
                                    break;
                                }
                                else if (!processed[ny][nx] && rgb == baseRGB) {
                                    usefulCheck = true;
                                }
                            }
                            if (canExpandDown) {
                                rectHeight++;
                                if (usefulCheck) revertCount = 0;
                                else revertCount++;
                            }
                        }
                        rectHeight -= revertCount;
                        revertCount = 0;

                        //Expand Right again (only necessary overlap)
                        while (canExpandRight && x + rectWidth < imgWidth && x + rectWidth < tWidth) {
                            usefulCheck = false;
                            int nx = x + rectWidth;
                            for (int i = 0; i < rectHeight; i++) {
                                int ny = y + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0 || (processed[ny][nx] && rgb != baseRGB)) {
                                    canExpandRight = false;
                                    if(cov[ny][nx] > minInsertIndex){
                                        minInsertIndex = cov[ny][nx];
                                    }
                                    break;
                                }
                                else if (!processed[ny][nx] && rgb == baseRGB) {
                                    usefulCheck = true;
                                }
                            }
                            if (canExpandRight) {
                                rectWidth++;
                                if (usefulCheck) revertCount = 0;
                                else revertCount++;
                            }
                        }
                        rectWidth -= revertCount;
                        revertCount = 0;

                        //Expand Left
                        int insertIndex = gdObjects.size() + 1; //index the object will be inserted at
                        while (canExpandLeft && x > tx*tileWidth) {
                            int nx = x - 1;
                            //check every pixel in the rects height
                            for (int i = 0; i < rectHeight; i++) {
                                int ny = y + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0) {
                                    canExpandLeft = false;
                                }
                                else if(processed[ny][nx] && rgb != baseRGB){
                                    if(minInsertIndex >= cov[ny][nx]){
                                        canExpandLeft = false;
                                    }
                                    else if(cov[ny][nx] < insertIndex){
                                        insertIndex = cov[ny][nx];
                                    }
                                }
                                else if(rgb == baseRGB){
                                    if(cov[ny][nx] > minInsertIndex){
                                        minInsertIndex = cov[ny][nx]; 
                                    }
                                }
                            }
                            if (canExpandLeft) {
                                rectWidth++;
                                x--;
                            }
                        }

                        /*if(gdObjects.size() == 6){
                            System.out.println(minInsertIndex + ", " + insertIndex);
                        }*/

                        //Expand Up
                        while (canExpandUp && y > ty*tileHeight) {
                            int ny = y - 1;
                            for (int i = 0; i < rectWidth; i++) {
                                int nx = x + i;
                                int p = image.getRGB(nx, ny);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;

                                if (alpha == 0) {
                                    canExpandUp = false;
                                }
                                else if(processed[ny][nx] && rgb != baseRGB){
                                    if(minInsertIndex >= cov[ny][nx]){
                                        canExpandUp = false;
                                    }
                                    else if(cov[ny][nx] < insertIndex){
                                        insertIndex = cov[ny][nx];
                                    }
                                }
                                else if(rgb == baseRGB){
                                    if(cov[ny][nx] > minInsertIndex){
                                        minInsertIndex = cov[ny][nx];
                                    }
                                }
                            }
                            if (canExpandUp) {
                                rectHeight++;
                                y--;
                            }
                        }

                        /*if(gdObjects.size() == 6){
                            System.out.println(minInsertIndex + ", " + insertIndex);
                        }*/
                        
                        for(int i = 0; i < cov.length; i++){
                           for(int j = 0; j < cov[i].length; j++){
                                if(cov[i][j] >= insertIndex){
                                    cov[i][j]++;
                                }
                            } 
                        }
                        //Mark Processed
                        for (int dy = 0; dy < rectHeight; dy++) {
                            for (int dx = 0; dx < rectWidth; dx++) {
                                int p = image.getRGB(x + dx, y + dy);
                                int alpha = (p >>> 24) & 0xFF;
                                int rgb   = p & 0xFFFFFF;
                                if (alpha != 0 && rgb == baseRGB && insertIndex > cov[y + dy][x + dx]) {
                                    processed[y + dy][x + dx] = true;
                                    //if(gdObjects.size() == 6){
                                    //    System.out.print("#  ");
                                    //}
                                }
                                //else if(gdObjects.size() == 6){
                                //    System.out.print("-  ");
                                //}
                                if(insertIndex > cov[y + dy][x + dx]){
                                    cov[y + dy][x + dx] = insertIndex;

                                }
                            }
                            //if(gdObjects.size() == 6){
                            //    System.out.print("\n");
                            //}
                        }


                        // Convert RGB back to Color
                        Color col = new Color(baseRGB);
                        if (!uniqueColors.contains(col)) {
                            uniqueColors.add(col);
                        }

                        int colorId = uniqueColors.indexOf(col);
                        
                        //Create Object
                        gdObjects.add(insertIndex-1, new GDObject(x, y, rectWidth, rectHeight, colorId, zLayer, startingZOrder, maxLinked));
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

    public String[] run(String p, float s, int c, int l, int o, int tileWidth, int tileHeight) throws Exception{
        BufferedImage image = ImageIO.read(new File(p));

        long startTime = System.currentTimeMillis();
        
        int maxLinked = GDSave.ReadFromGD.getMaxLinked();
        List<GDObject> gdObjects = convertToGDObjectsTiled(image, l, o, tileWidth, tileHeight, maxLinked);
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
