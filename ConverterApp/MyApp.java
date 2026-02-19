package ConverterApp;

import ImgConverter.Pix2GD;

import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.io.*;
import java.util.Scanner;

public class MyApp {
    // App State
    private static String filePath = "";
    private static String directoryPath = System.getProperty("user.dir");

    private static float scale = 1.0f;
    private static int zLayer = 7;
    private static int startZOrder = 1;
    private static int startColor = 1;

    private static int tileWidth = 0;
    private static int tileHeight = 0;  

    private int lastZOrderValue = startZOrder;

    private static String[] convertedData;

    // UI
    private JFrame frame;

    private JButton browseBtn;
    private JButton runBtn;
    private JButton exportBtn;

    private JTextField scaleField;

    private JSpinner colorSpinner;
    private JSpinner zLayerSpinner;
    private JSpinner zOrderSpinner;

    private JSpinner tileWidthSpinner;
    private JSpinner tileHeightSpinner;

    private JLabel imageLabel;
    private JLabel statusLabel;

    private JFileChooser fileChooser;

    private BufferedImage loadedImage;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(MyApp::new);
    }

    public MyApp() {
        readSave();
        initFrame();
        initFileChooser();
        buildUI();

        // Auto-load saved image (if exists)
        if (!filePath.isEmpty() && new File(filePath).exists()) {
            loadImage();
            runBtn.setEnabled(true);
            exportBtn.setEnabled(false);
            statusLabel.setText("Loaded: " + new File(filePath).getName());
        }

        frame.setVisible(true);
    }

    private void initFrame() {
        frame = new JFrame("Pix2GD");
        frame.setSize(1000, 650);
        frame.setLocationRelativeTo(null);
        frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

        frame.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                saveSettings();
                System.exit(0);
            }
        });
    }

    private void initFileChooser() {
        fileChooser = new JFileChooser(directoryPath);
        fileChooser.setFileFilter(new FileNameExtensionFilter("Image Files", "jpg", "jpeg", "png", "gif"));
    }

    private void buildUI() {
        frame.setLayout(new BorderLayout(10, 10));
        frame.add(createLeftPanel(), BorderLayout.WEST);
        frame.add(createImagePanel(), BorderLayout.CENTER);
        frame.add(createBottomPanel(), BorderLayout.SOUTH);
    }

    private JPanel createLeftPanel() {
        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBorder(new EmptyBorder(15, 15, 15, 15));
        panel.setPreferredSize(new Dimension(280, 0));

        /* File */
        browseBtn = new JButton("Choose Image");
        browseBtn.addActionListener(e -> chooseImage());
        panel.add(browseBtn);
        panel.add(Box.createVerticalStrut(15));

        /* Settings */
        panel.add(createSectionLabel("Settings"));
        panel.add(Box.createVerticalStrut(10));
        panel.add(createRow("Scale:", scaleField = smallText(String.valueOf(scale))));
        panel.add(createRow("Start Color:", colorSpinner = spinner(startColor, 1, 9999, 1)));
        panel.add(createRow("Z-Layer:", zLayerSpinner = spinner(zLayer, -5, 9, 2)));
        panel.add(createRow("Start Z-Order:", zOrderSpinner = spinner(startZOrder, -100, 100, 1)));

        tileWidthSpinner = spinner(tileWidth, 0, Integer.MAX_VALUE, 8);
        tileWidthSpinner.setToolTipText("Image will be processed in separate tiles of this width. Keep at 0 for no horizontal tiling");
        panel.add(createRow("Tile Width:", tileWidthSpinner));

        tileHeightSpinner = spinner(tileHeight, 0, Integer.MAX_VALUE, 8);
        tileHeightSpinner.setToolTipText("Image will be processed in separate tiles of this height. Keep at 0 for no vertical tiling");
        panel.add(createRow("Tile Height:", tileHeightSpinner));

        zOrderSpinner.addChangeListener(e -> preventZero(e));
        panel.add(Box.createVerticalStrut(25));

        /* Actions */
        panel.add(createSectionLabel("Actions"));
        panel.add(Box.createVerticalStrut(10));
        runBtn = new JButton("Run Conversion");
        exportBtn = new JButton("Export to GD");
        runBtn.setEnabled(false);
        exportBtn.setEnabled(false);
        runBtn.addActionListener(e -> runConversion());
        exportBtn.addActionListener(e -> exportToGD());
        panel.add(runBtn);
        panel.add(Box.createVerticalStrut(8));
        panel.add(exportBtn);
        panel.add(Box.createVerticalGlue());

        return panel;
    }

    private JPanel createImagePanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(new EmptyBorder(10, 10, 10, 10));

        imageLabel = new JLabel("No image selected", JLabel.CENTER);
        imageLabel.setFont(new Font("Arial", Font.PLAIN, 18));

        panel.add(new JScrollPane(imageLabel), BorderLayout.CENTER);

        return panel;
    }

    private JPanel createBottomPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(new EmptyBorder(8, 15, 8, 15));
        statusLabel = new JLabel("Ready.");

        panel.add(statusLabel, BorderLayout.WEST);

        return panel;
    }

    private JLabel createSectionLabel(String text) {
        JLabel label = new JLabel(text);
        label.setFont(new Font("Arial", Font.BOLD, 14));

        return label;
    }

    private JPanel createRow(String label, JComponent field) {
        JPanel row = new JPanel(new BorderLayout(5, 0));
        JLabel l = new JLabel(label);
        l.setToolTipText(field.getToolTipText());
        l.setPreferredSize(new Dimension(110, 25));

        row.add(l, BorderLayout.WEST);
        row.add(field, BorderLayout.CENTER);

        row.setMaximumSize(new Dimension(Integer.MAX_VALUE, 30));

        return row;
    }

    private JTextField smallText(String text) {
        JTextField field = new JTextField(text);
        field.setMaximumSize(new Dimension(80, 25));
        return field;
    }

    private JSpinner spinner(int val, int min, int max, int step) {
        SpinnerModel model = new SpinnerNumberModel(val, min, max, step);
        JSpinner spinner = new JSpinner(model);
        spinner.setMaximumSize(new Dimension(80, 25));
        return spinner;
    }

    private void chooseImage() {
        int result = fileChooser.showOpenDialog(frame);

        if (result != JFileChooser.APPROVE_OPTION){
            return;
        }

        File file = fileChooser.getSelectedFile();
        filePath = file.getAbsolutePath();
        directoryPath = file.getParent();

        loadImage();

        runBtn.setEnabled(true);
        exportBtn.setEnabled(false);
        statusLabel.setText("Loaded: " + file.getName());
    }

    private void preventZero(ChangeEvent e){
        JSpinner spinner = (JSpinner) e.getSource();
        int value = (int)spinner.getValue();
        if(value == 0){
            if(lastZOrderValue > value){
                zOrderSpinner.setValue(-1);
                lastZOrderValue = -2;
                return;
            }
            else if(lastZOrderValue < value){
                zOrderSpinner.setValue(1);
                lastZOrderValue = 2;
                return;
            }
        }

        lastZOrderValue = value;
    }

    private void loadImage() {
        try {
            loadedImage = ImageIO.read(new File(filePath));
            updateImagePreview();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void updateImagePreview() {
        if (loadedImage == null){
            return;
        }

        int maxW = 650;
        int maxH = 500;

        int w = loadedImage.getWidth();
        int h = loadedImage.getHeight();
        double scale = Math.min((double) maxW / w, (double) maxH / h);

        int newW = (int) (w * scale);
        int newH = (int) (h * scale);
        Image scaled = loadedImage.getScaledInstance(newW, newH, Image.SCALE_SMOOTH);

        imageLabel.setIcon(new ImageIcon(scaled));
        imageLabel.setText(null);
    }

    private void runConversion() {
        try {
            float scaleVal = Float.parseFloat(scaleField.getText());
            int startColorVal = (int) colorSpinner.getValue();
            int zLayerVal = (int) zLayerSpinner.getValue();
            int zOrderVal = (int) zOrderSpinner.getValue();
            int tileWidthVal = (int) tileWidthSpinner.getValue();
            int tileHeightVal = (int) tileHeightSpinner.getValue();
            Pix2GD converter = new Pix2GD();

            //convertedData = converter.run(filePath, scaleVal, startColorVal, zLayerVal, zOrderVal);
            convertedData = converter.run(filePath, scaleVal, startColorVal, zLayerVal, zOrderVal, tileWidthVal, tileHeightVal);
            statusLabel.setText("Converted to " + convertedData[0] + " objects in " + convertedData[2] + " sec");
            exportBtn.setEnabled(true);
        }
        catch (Exception e) {
            e.printStackTrace();
            statusLabel.setText("Conversion failed.");
        }
    }

    private void exportToGD() {
        if (convertedData == null){
            return;
        }

        try {
            GDSave.WriteToGD.writeToGD(convertedData[1]);
            statusLabel.setText("Exported to GD successfully.");
        }
        catch (Exception e) {
            e.printStackTrace();
            statusLabel.setText("Export failed.");
        }
    }

    private void saveSettings() {
        try {
            FileWriter writer = new FileWriter(System.getProperty("user.dir") + "/ConverterApp/settings.txt");

            writer.write(filePath + ",");
            writer.write(scaleField.getText() + ",");
            writer.write(colorSpinner.getValue() + ",");
            writer.write(zLayerSpinner.getValue() + ",");
            writer.write(zOrderSpinner.getValue() + ",");
            writer.write(directoryPath + ",");
            writer.write(tileWidthSpinner.getValue() + ",");
            writer.write(tileHeightSpinner.getValue() + ",");

            writer.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void readSave() {
        File file = new File(System.getProperty("user.dir") + "/ConverterApp/settings.txt");
        if (!file.exists()){
            return;
        }

        try (Scanner sc = new Scanner(file)) {
            if (!sc.hasNextLine()){
                return;
            }

            String[] data = sc.nextLine().split(",");

            filePath = data[0];
            scale = Float.parseFloat(data[1]);
            startColor = Integer.parseInt(data[2]);
            zLayer = Integer.parseInt(data[3]);
            startZOrder = Integer.parseInt(data[4]);
            directoryPath = data[5];
            tileWidth = Integer.parseInt(data[6]);
            tileHeight = Integer.parseInt(data[7]);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
}
