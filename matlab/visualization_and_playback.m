%% This script is for visualization/testing purposes. 
%  You need matlab to run the code below

clear all

% Replace below with file path to wav file
file = "../test.wav";
[Y, fs] = audioread(file);

%% Viz waveform
subplot(2,1,1);
y1 = Y(:, 1);

N = length(y1); % #samples

dt = 1/fs;
t1 = 0:dt:(N*dt - dt);
plot(t1, y1);
title('Channel 1');

xlabel('seconds');
ylabel('amplitude');

% In our case (with an unsophisticated setup), channel 1 = channel 2
subplot(2,1,2);
y2 = Y(:, 2);
t2 = 0:dt:(N*dt - dt);
plot(t2, y2);
title('Channel 2'); 

xlabel('seconds');
ylabel('amplitude');

%% Playback
sound(Y, fs);
