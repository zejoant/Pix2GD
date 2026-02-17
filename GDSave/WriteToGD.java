package GDSave;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

public class WriteToGD {

    // Same XOR as before
    public static byte[] xorEncrypt(byte[] data) {
        byte[] out = new byte[data.length];

        for (int i = 0; i < data.length; i++) {
            out[i] = (byte) (data[i] ^ 0x0B);
        }

        return out;
    }

    // Compress + Base64 encode level
    public static String encodeLevel(String levelText) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        try (GZIPOutputStream gzip = new GZIPOutputStream(baos)) {
            gzip.write(levelText.getBytes(StandardCharsets.UTF_8));
        }

        byte[] compressed = baos.toByteArray();

        return Base64.getUrlEncoder().encodeToString(compressed);
        //return Base64.getUrlEncoder().withoutPadding().encodeToString(compressed);
    }

    // Compress + encrypt save file
    public static byte[] encodeSaveFile(String xml) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        try (GZIPOutputStream gzip = new GZIPOutputStream(baos)) {
            gzip.write(xml.getBytes(StandardCharsets.UTF_8));
        }

        byte[] compressed = baos.toByteArray();

        String base64 = Base64.getUrlEncoder().encodeToString(compressed);
        //String base64 = Base64.getUrlEncoder().withoutPadding().encodeToString(compressed);

        return xorEncrypt(base64.getBytes(StandardCharsets.UTF_8));
    }

    public static void writeToGD(String objects) throws Exception{
        Path filePath = Path.of(System.getenv("LOCALAPPDATA"), "GeometryDash", "CCLocalLevels.dat");
        Path backupFilePath = Path.of(System.getenv("LOCALAPPDATA"), "GeometryDash", "CCLocalLevels2.dat");

        // Read file
        byte[] data = Files.readAllBytes(filePath);

        // Decode file data
        String xml = ReadFromGD.decodeSaveFile(data);
        
        //get only the top level
        String levelBase64 = ReadFromGD.extractLevel(xml);

        //decode level data
        String decoded = ReadFromGD.decodeLevel(levelBase64, false);
        
        //add new objects to level
        String newLevelBase64 = encodeLevel(decoded + objects);

        //insert in xml
        String newXml = xml.replaceFirst("<k>k4</k>\\s*<s>.*?</s>", "<k>k4</k><s>" + newLevelBase64 + "</s>");

        //encode
        byte[] newSave = encodeSaveFile(newXml);

        // Create backup
        System.out.println("Creating backup: CCLocalLevels2.dat");
        Files.copy(filePath, backupFilePath, StandardCopyOption.REPLACE_EXISTING);
        
        // Write new save
        System.out.println("Writing to the level " + ReadFromGD.extractLevelName(newXml));
        Files.write(filePath, newSave);
    }
}
