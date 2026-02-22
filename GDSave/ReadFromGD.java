package GDSave;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.GZIPInputStream;

public class ReadFromGD {
    
    public static String decodeLevel(String levelData, boolean isOfficial) throws IOException {

        if (isOfficial) {
            levelData = "H4sIAAAAAAAAA" + levelData;
        }

        // Base64 decode
        byte[] compressed = Base64.getUrlDecoder().decode(levelData);

        // GZIP decompress
        ByteArrayInputStream bais = new ByteArrayInputStream(compressed);
        GZIPInputStream gzip = new GZIPInputStream(bais);

        InputStreamReader reader = new InputStreamReader(gzip);
        BufferedReader br = new BufferedReader(reader);

        StringBuilder sb = new StringBuilder();
        String line;

        while ((line = br.readLine()) != null) {
            sb.append(line);
        }

        return sb.toString();
    }

    public static List<Map<String, String>> parseObjects(String levelData) {

        List<Map<String, String>> objects = new ArrayList<>();

        String[] rawObjects = levelData.split(";");

        for (String obj : rawObjects) {
            if (obj.isEmpty()) continue;

            String[] tokens = obj.split(",");

            Map<String, String> properties = new LinkedHashMap<>();

            for (int i = 0; i < tokens.length - 1; i += 2) {
                String key = tokens[i];
                String value = tokens[i + 1];

                properties.put(key, value);
            }

            objects.add(properties);
        }

        return objects;
    }


    public static byte[] xorDecrypt(byte[] data) {
        byte[] out = new byte[data.length];

        for (int i = 0; i < data.length; i++) {
            out[i] = (byte) (data[i] ^ 0x0B);
        }

        return out;
    }

    public static String extractLevel(String xml) {

        //System.out.println("Reading from the level " + extractLevelName(xml));

        // Match <k>k4</k><s>BASE64</s>, allowing whitespace/newlines
        Pattern pattern = Pattern.compile(
            "<k>k4</k>\\s*<s>(.*?)</s>",
            Pattern.DOTALL
        );

        Matcher matcher = pattern.matcher(xml);

        if (matcher.find()) {
            return matcher.group(1).trim();
        }

        return null;
    }

    public static String extractLevelName(String xml) {
        Pattern pattern = Pattern.compile(
            "<k>k2</k>\\s*<s>(.*?)</s>",
            Pattern.DOTALL
        );

        Matcher matcher = pattern.matcher(xml);

        if (matcher.find()) {
            return matcher.group(1).trim();
        }

        return null;
    }

    public static String decodeSaveFile(byte[] data) throws Exception {

        // XOR decrypt
        byte[] decrypted = xorDecrypt(data);

        // Convert to string
        String base64 = new String(decrypted, StandardCharsets.UTF_8).trim();

        // Remove line breaks
        base64 = base64.replace("\n", "").replace("\r", "");

        // URL-safe Base64 decode
        byte[] compressed = Base64.getUrlDecoder().decode(base64);

        // GZIP decompress
        ByteArrayInputStream bais = new ByteArrayInputStream(compressed);
        GZIPInputStream gzip = new GZIPInputStream(bais);

        return new String(gzip.readAllBytes(), StandardCharsets.UTF_8);
    }

    public static int getMaxLinked() throws Exception {
        int maxLinkID = 0;
        String data = readFromGD();
        String[] objects = data.split(";");
        for (int i = 1; i < objects.length; i++) {
            String[] props = objects[i].split(",");
            for (int j = 0; j + 1 < props.length; j += 2) {
                int key = Integer.parseInt(props[j]);
                if (key == 108) {
                    int value = Integer.parseInt(props[j + 1]);
                    if (value > maxLinkID) {
                        maxLinkID = value;
                    }
                    break;
                }
            }
        }
        return maxLinkID;
    }

    public static String readFromGD() throws Exception{
        Path filePath = Path.of(System.getenv("LOCALAPPDATA"), "GeometryDash", "CCLocalLevels.dat");

        // Read file
        byte[] data = Files.readAllBytes(filePath);

        // Decode file data
        String xml = decodeSaveFile(data);

        //System.out.println("Reading from the level " + ReadFromGD.extractLevelName(xml));
        
        //get only the top level
        String levelBase64 = ReadFromGD.extractLevel(xml);

        //decode level data
        String decoded = decodeLevel(levelBase64, false);

        //System.out.println(decoded);
        return decoded;
    }
}
