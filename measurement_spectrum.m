clear; clc; close all hidden  % Reset workspace and close figures

% Read hex data from file
filePath = "measurement_data.txt";
fileID = fopen(filePath, 'r');
rawData = fscanf(fileID, '%c');  % Read the entire file as text
fclose(fileID);

% Convert the text hex data into decimal values
hexValues = sscanf(rawData, '%2x');  % Each 2 hex characters = 1 byte

% Skip the first 10 bytes (first 5 values)
dataAfter10 = hexValues(6:end);  

% Number of 16-bit intensity values (4096 bytes → 2048 values)
numPairs = floor(length(dataAfter10) / 2);  
pairedData = zeros(1, numPairs);  % Pre-allocate for efficiency

% Convert every two bytes into a single 16-bit intensity value
for i = 1:numPairs
    pairedData(i) = dataAfter10(2*i-1) + dataAfter10(2*i) * 256;
end

% X-axis: Simple pixel index (1 to 2048)
x = 1:numPairs;

% Plot intensity values
figure;
plot(x, pairedData, 'b-', 'LineWidth', 1.5);
xlabel('Pixel Index');
ylabel('Intensity Value');
title('Intensity Values from Measurement Data');
grid on;
